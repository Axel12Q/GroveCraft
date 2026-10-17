package dev.libertycraft;

import java.nio.file.*;
import java.util.*;

/** Whole-map placement decisions, made once when the personal world starts.
 * The immutable spatial index avoids generating/sorting new candidates in travel ticks.
 * Native terrain clearance is still required before placing an unvisited site. */
public final class GroveWorldPlan {
    public static final int VERSION=1;
    private final Map<Long,List<GroveEcologyRules.Site>> regions=new HashMap<>();
    private final List<GroveEcologyRules.Site> sites;
    public final long seed;
    public record Placement(GroveEcologyRules.Site site,double height){}
    private final Map<String,Placement> placements=new HashMap<>();
    private int resolvedTotal;
    public Placement placement(GroveEcologyRules.Site owner){return placements.get(owner.key());}
    public int resolvedCount(){return resolvedTotal;}
    public void resolve(Path folder,GroveSurfaceAtlas atlas)throws java.io.IOException{
        Path file=folder.resolve("grovecraft-world-placements-v2.tsv");
        String identity="2\t"+seed+"\t"+atlas.identity();
        if(Files.exists(file)){
            var lines=Files.readAllLines(file);if(lines.isEmpty()||!lines.getFirst().equals(identity))throw new java.io.IOException("Placement identity mismatch");
            var owners=new HashMap<String,GroveEcologyRules.Site>();for(var s:sites)owners.put(s.key(),s);
            for(int i=1;i<lines.size();i++){
                var a=lines.get(i).split("\t");if(a.length!=4||placements.containsKey(a[0])||!owners.containsKey(a[0]))throw new java.io.IOException("Invalid placement entry");
                try{var owner=owners.get(a[0]);var site=new GroveEcologyRules.Site(owner.key(),Integer.parseInt(a[1]),Integer.parseInt(a[2]),owner.kind(),owner.seed());
                    placements.put(a[0],new Placement(site,Double.parseDouble(a[3])));
                }catch(RuntimeException e){throw new java.io.IOException("Invalid placement",e);}
            }
            if(placements.size()!=sites.size())throw new java.io.IOException("Incomplete placements");
            resolvedTotal=(int)placements.values().stream().filter(p->Double.isFinite(p.height)).count();reindex();return;
        }
        for(var owner:sites){
            boolean adaptive=owner.kind()==GroveEcologyRules.Kind.HAMLET||owner.kind()==GroveEcologyRules.Kind.OUTPOST||owner.kind()==GroveEcologyRules.Kind.WAYSTATION;
            Placement found=new Placement(owner,Double.NaN);
            for(int candidate=0;candidate<(adaptive?25:1);candidate++){
                var s=adaptive?GroveHamletPlan.nearby(owner,candidate):owner;
                double height=atlas.ground(s.x(),s.z());if(!Double.isFinite(height)||height<.5||height>950)continue;
                if(adaptive&&!GroveEcologyRules.countryside(s.x(),s.z(),GroveEcologyRules.radius(s.kind())+3))continue;
                boolean tree=s.kind()==GroveEcologyRules.Kind.OAK||s.kind()==GroveEcologyRules.Kind.BIRCH;
                if(s.kind()==GroveEcologyRules.Kind.HAMLET){if(!GroveHamletPlan.fit(s,height,atlas::ground).valid())continue;}
                else {
                    int radius=tree?1:GroveEcologyRules.radius(s.kind())+2;double low=height,high=height;boolean valid=true;
                    for(int dx=-radius;dx<=radius&&valid;dx++)for(int dz=-radius;dz<=radius;dz++){
                        double y=atlas.ground(s.x()+dx,s.z()+dz);if(!Double.isFinite(y)){valid=false;break;}
                        low=Math.min(low,y);high=Math.max(high,y);
                    }
                    if(!valid||high-low>(tree?3.5:adaptive?3:1.5))continue;
                }
                found=new Placement(s,height);break;
            }
            placements.put(owner.key(),found);
        }
        resolvedTotal=(int)placements.values().stream().filter(p->Double.isFinite(p.height)).count();
        reindex();
        var text=new StringBuilder(identity+"\n");
        for(var owner:sites){var p=placements.get(owner.key());text.append(owner.key()).append('\t').append(p.site.x()).append('\t').append(p.site.z()).append('\t').append(p.height).append('\n');}
        Path temp=file.resolveSibling(file.getFileName()+".tmp");Files.writeString(temp,text);Files.move(temp,file,StandardCopyOption.ATOMIC_MOVE);
    }
    private static long key(int x,int z){return ((long)x<<32)|(z&0xffffffffL);}
    private GroveWorldPlan(long seed,List<GroveEcologyRules.Site> sites){
        this.seed=seed;this.sites=List.copyOf(sites);
        reindex();
    }
    public GroveEcologyRules.Site location(GroveEcologyRules.Site owner){
        var p=placements.get(owner.key());return p==null?owner:p.site();
    }
    private void reindex(){
        regions.clear();
        for(var owner:sites){
            var p=placements.get(owner.key());if(p!=null&&!Double.isFinite(p.height()))continue;
            var s=location(owner);
            // Spatial membership follows the final parcel. Keep returning the
            // original owner, whose chunk ledger must never move or duplicate.
            regions.computeIfAbsent(key(Math.floorDiv(s.x(),192),Math.floorDiv(s.z(),192)),k->new ArrayList<>()).add(owner);
        }
        regions.replaceAll((k,v)->List.copyOf(v));
    }
    public int size(){return sites.size();}
    public List<GroveEcologyRules.Site> all(){return sites;}
    public List<GroveEcologyRules.Site> nearby(double x,double z,double radius){
        var out=new ArrayList<GroveEcologyRules.Site>();double r2=radius*radius;
        for(int rx=Math.floorDiv((int)Math.floor(x-radius),192);rx<=Math.floorDiv((int)Math.floor(x+radius),192);rx++)
            for(int rz=Math.floorDiv((int)Math.floor(z-radius),192);rz<=Math.floorDiv((int)Math.floor(z+radius),192);rz++)
                for(var s:regions.getOrDefault(key(rx,rz),List.of())){
                    var actual=location(s);double dx=actual.x()-x,dz=actual.z()-z;if(dx*dx+dz*dz<=r2)out.add(s);
                }
        return out;
    }
    public static boolean reserved(long seed,GroveEcologyRules.Site tree){
        int gx=Math.floorDiv(tree.x(),192),gz=Math.floorDiv(tree.z(),192);
        for(int dx=-1;dx<=1;dx++)for(int dz=-1;dz<=1;dz++){
            var s=GroveEcologyRules.site(seed,gx+dx,gz+dz,false);
            int r=GroveEcologyRules.radius(s.kind())+7+(s.kind()==GroveEcologyRules.Kind.HAMLET?48:0);
            if(Math.abs(tree.x()-s.x())<=r&&Math.abs(tree.z()-s.z())<=r)return true;
        }
        gx=Math.floorDiv(tree.x(),384);gz=Math.floorDiv(tree.z(),384);
        for(int dx=-1;dx<=1;dx++)for(int dz=-1;dz<=1;dz++){
            var s=GroveEcologyRules.expansionSite(seed,gx+dx,gz+dz);int r=GroveEcologyRules.radius(s.kind())+55;
            if(Math.abs(tree.x()-s.x())<=r&&Math.abs(tree.z()-s.z())<=r)return true;
        }
        return false;
    }
    public static GroveWorldPlan create(long seed){
        var sites=new ArrayList<GroveEcologyRules.Site>();
        for(int gx=-188;gx<=187;gx++)for(int gz=-188;gz<=187;gz++){
            var s=GroveEcologyRules.woodlandSite(seed,gx,gz);
            if(GroveEcologyRules.woodland(s.x(),s.z(),4)&&!reserved(seed,s))sites.add(s);
        }
        for(int gx=-16;gx<=15;gx++)for(int gz=-16;gz<=15;gz++){
            var s=GroveEcologyRules.site(seed,gx,gz,false);
            if(GroveEcologyRules.countryside(s.x(),s.z(),GroveEcologyRules.radius(s.kind())+3))sites.add(s);
        }
        for(int gx=-8;gx<=7;gx++)for(int gz=-8;gz<=7;gz++){
            var s=GroveEcologyRules.expansionSite(seed,gx,gz);
            if(GroveEcologyRules.countryside(s.x(),s.z(),GroveEcologyRules.radius(s.kind())+3))sites.add(s);
        }
        sites.sort(Comparator.comparing(GroveEcologyRules.Site::key));
        return new GroveWorldPlan(seed,sites);
    }
    public static GroveWorldPlan loadOrCreate(Path folder,long seed)throws java.io.IOException{
        Path file=folder.resolve("grovecraft-world-plan-v1.tsv");
        if(Files.exists(file)){
            var lines=Files.readAllLines(file);if(lines.isEmpty()||!lines.getFirst().equals(VERSION+"\t"+seed))throw new java.io.IOException("World plan identity mismatch");
            var sites=new ArrayList<GroveEcologyRules.Site>();var keys=new HashSet<String>();
            for(int i=1;i<lines.size();i++){
                var a=lines.get(i).split("\t");if(a.length!=5||!keys.add(a[0]))throw new java.io.IOException("Malformed/duplicate world plan entry");
                try{sites.add(new GroveEcologyRules.Site(a[0],Integer.parseInt(a[1]),Integer.parseInt(a[2]),GroveEcologyRules.Kind.valueOf(a[3]),Long.parseLong(a[4])));}
                catch(RuntimeException e){throw new java.io.IOException("Invalid world plan entry",e);}
            }
            return new GroveWorldPlan(seed,sites);
        }
        var plan=create(seed);var text=new StringBuilder(VERSION+"\t"+seed+"\n");
        for(var s:plan.sites)text.append(s.key()).append('\t').append(s.x()).append('\t').append(s.z()).append('\t').append(s.kind()).append('\t').append(s.seed()).append('\n');
        Files.createDirectories(folder);Path temp=file.resolveSibling(file.getFileName()+".tmp");Files.writeString(temp,text);
        Files.move(temp,file,StandardCopyOption.ATOMIC_MOVE);
        return plan;
    }
}
