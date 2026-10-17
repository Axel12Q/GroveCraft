package dev.libertycraft;

import java.util.*;

/** Saved inside the same anchor chunk as the successful generation ledger. */
public final class GroveStructureMap {
    public static final int LIMIT=64;
    public static final String PREFIX="map-v1:";
    public record Point(String key,int kind,int x,int y,int z) {}
    private final Map<String,Point> points=new HashMap<>();
    public static int kind(GroveEcologyRules.Kind kind){return switch(kind){
        case HAMLET->1;case OUTPOST->2;case WAYSTATION->3;case FARM->4;case CAMP->5;case RUIN->6;default->0;
    };}
    public static String encode(Point p){
        if(!valid(p))throw new IllegalArgumentException("Invalid structure marker");
        return PREFIX+p.key()+"\t"+p.kind()+"\t"+p.x()+"\t"+p.y()+"\t"+p.z();
    }
    public static Optional<Point> decode(String text){
        if(!text.startsWith(PREFIX))return Optional.empty();
        try{var a=text.substring(PREFIX.length()).split("\t",-1);if(a.length!=5)return Optional.empty();
            var p=new Point(a[0],Integer.parseInt(a[1]),Integer.parseInt(a[2]),Integer.parseInt(a[3]),Integer.parseInt(a[4]));
            return valid(p)?Optional.of(p):Optional.empty();
        }catch(RuntimeException e){return Optional.empty();}
    }
    private static boolean valid(Point p){return p.key().matches("[A-Za-z0-9:_-]{1,100}")&&p.kind()>=1&&p.kind()<=6
        &&Math.abs((long)p.x())<=4000&&Math.abs((long)p.z())<=4000&&p.y()>=-60&&p.y()<=1000;}
    public void remember(Point p){if(valid(p))points.put(p.key(),p);}
    /** Resolved atlas parcels are destinations before their chunks are visited.
     * Saved actual positions win, including relocated structures in old worlds. */
    public void addPlanned(GroveWorldPlan plan){
        if(plan==null)return;
        for(var owner:plan.all()){
            int k=kind(owner.kind());if(k==0||points.containsKey(owner.key()))continue;
            var p=plan.placement(owner);
            if(p!=null&&Double.isFinite(p.height()))remember(new Point(owner.key(),k,p.site().x(),(int)Math.floor(p.height()),p.site().z()));
        }
    }
    public void restore(List<String> ledger){if(ledger!=null)for(var row:ledger)decode(row).ifPresent(this::remember);}
    public boolean contains(String key){return points.containsKey(key);}
    public int size(){return points.size();}
    public List<String> ledger(){return points.values().stream().sorted(Comparator.comparing(Point::key)).map(GroveStructureMap::encode).toList();}
    public List<Point> nearest(double x,double z){
        return points.values().stream().sorted(Comparator.<Point>comparingDouble(p->Math.hypot(p.x()-x,p.z()-z))
            .thenComparing(Point::key)).limit(LIMIT).sorted(Comparator.comparing(Point::key)).toList();
    }
}
