package dev.libertycraft;

import java.util.*;

/** Three level platforms joined by a walkable path; never flattens native ground. */
public final class GroveHamletPlan {
    private GroveHamletPlan() {}
    public record Column(int x,int z) {}
    public interface Ground { double height(int x,int z); }
    public record Layout(String failure,int base,int west,int east,List<GroveEcologyRules.Cell> cells,
                         Map<Column,Double> ground,Map<Column,Integer> floors) {
        public boolean valid(){return failure.isEmpty();}
    }
    private static Layout fail(String reason){return new Layout(reason,0,0,0,List.of(),Map.of(),Map.of());}
    private static int module(Column c){return c.z()>=4?2:c.z()<=2&&c.x()<=-3?0:c.z()<=2&&c.x()>=3?1:3;}
    public static Layout fit(GroveEcologyRules.Site site,double nearY,Ground sample) {
        var original=GroveEcologyRules.plan(site);
        Map<Column,Double> ground=new LinkedHashMap<>();
        double[] lo={Double.POSITIVE_INFINITY,Double.POSITIVE_INFINITY,Double.POSITIVE_INFINITY};
        double[] hi={Double.NEGATIVE_INFINITY,Double.NEGATIVE_INFINITY,Double.NEGATIVE_INFINITY};
        double min=Double.POSITIVE_INFINITY,max=Double.NEGATIVE_INFINITY;
        for(var c:original){
            var p=new Column(c.x(),c.z());if(ground.containsKey(p))continue;
            double y=sample.height(site.x()+p.x(),site.z()+p.z());
            if(!Double.isFinite(y)||Math.abs(y-nearY)>24)return fail("suelo no apto");
            ground.put(p,y);min=Math.min(min,y);max=Math.max(max,y);
            int m=module(p);if(m<3){lo[m]=Math.min(lo[m],y);hi[m]=Math.max(hi[m],y);}
        }
        if(max-min>5)return fail("pendiente");
        int[] bases=new int[3];
        for(int i=0;i<3;i++){
            if(hi[i]-lo[i]>2)return fail("pendiente de parcela");
            bases[i]=(int)Math.floor(hi[i]);
        }
        Map<Column,Integer> floors=new LinkedHashMap<>();
        for(var e:ground.entrySet()){
            int m=module(e.getKey());floors.put(e.getKey(),m<3?bases[m]:(int)Math.floor(e.getValue()));
        }
        // Reject abrupt steps, including house entrance/path joins; no disconnected platforms.
        for(var e:floors.entrySet())for(var p:List.of(new Column(e.getKey().x()+1,e.getKey().z()),new Column(e.getKey().x(),e.getKey().z()+1))){
            Integer other=floors.get(p);if(other!=null&&Math.abs(e.getValue()-other)>1)return fail("camino discontinuo");
        }
        int base=(int)Math.floor(min);var cells=new ArrayList<GroveEcologyRules.Cell>();
        Set<Column> support=new HashSet<>();
        for(var c:original){
            var p=new Column(c.x(),c.z());int floor=floors.get(p);
            cells.add(new GroveEcologyRules.Cell(c.x(),floor-base+c.y(),c.z(),c.block()));
            if(support.add(p))for(int y=floor-1;y>=Math.floor(ground.get(p));y--)
                cells.add(new GroveEcologyRules.Cell(c.x(),y-base,c.z(),"cobblestone"));
        }
        if(cells.size()>768)return fail("presupuesto");
        return new Layout("",base,bases[0],bases[1],List.copyOf(cells),Map.copyOf(ground),Map.copyOf(floors));
    }
    /** Stable search inside the same reserved cell; original ledger key remains the owner. */
    public static GroveEcologyRules.Site nearby(GroveEcologyRules.Site site,int index){
        if(index==0)return site;
        if(index<0||index>24)throw new IllegalArgumentException("Search index");
        List<Column> offsets=new ArrayList<>();
        for(int x=-2;x<=2;x++)for(int z=-2;z<=2;z++)if(x!=0||z!=0)offsets.add(new Column(x*24,z*24));
        offsets.sort(Comparator.comparingInt(c->c.x()*c.x()+c.z()*c.z()));
        var c=offsets.get(index-1);
        return new GroveEcologyRules.Site(site.key(),site.x()+c.x(),site.z()+c.z(),site.kind(),site.seed());
    }
}
