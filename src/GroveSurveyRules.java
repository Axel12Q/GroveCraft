package dev.libertycraft;
import java.util.*;
/** Prepare visible-distance plots first, with a nearby fallback every third slot.
 * This orders bounded surveys; it never declares unknown geometry clear. */
public final class GroveSurveyRules {
    private GroveSurveyRules(){}
    /** Keep fast vehicles' preparation ahead, but ignore teleport-sized jumps. */
    public static double[] lookAhead(double x,double z){
        double distance=Math.hypot(x,z);
        if(!Double.isFinite(distance)||distance>900)return new double[]{0,0};
        double scale=distance>160?160/distance:1;
        return new double[]{x*scale,z*scale};
    }
    public static double rank(double dx,double dz,double leadX,double leadZ){
        double distance=Math.hypot(dx,dz),ahead=Math.hypot(dx-leadX,dz-leadZ);
        return Math.hypot(leadX,leadZ)>12?ahead+(distance<48?100:0):Math.abs(distance-104);
    }
    public static List<GroveEcologyRules.Site> order(List<GroveEcologyRules.Site> sites,double x,double z,double leadX,double leadZ){
        return order(sites,x,z,leadX,leadZ,java.util.function.Function.identity());
    }
    public static List<GroveEcologyRules.Site> order(List<GroveEcologyRules.Site> sites,double x,double z,double leadX,double leadZ,
            java.util.function.Function<GroveEcologyRules.Site,GroveEcologyRules.Site> location){
        var far=new ArrayList<>(sites);var near=new ArrayList<>(sites);
        far.sort(Comparator.comparingDouble(s->{var p=location.apply(s);return rank(p.x()-x,p.z()-z,leadX,leadZ);}));
        near.sort(Comparator.comparingDouble(s->{var p=location.apply(s);return Math.hypot(p.x()-x,p.z()-z);}));
        var result=new ArrayList<GroveEcologyRules.Site>();var seen=new HashSet<String>();int f=0,n=0;
        while(f<far.size()||n<near.size()){
            for(int i=0;i<2&&f<far.size();i++){var s=far.get(f++);if(seen.add(s.key()))result.add(s);}
            if(n<near.size()){var s=near.get(n++);if(seen.add(s.key()))result.add(s);}
        }
        return result;
    }
}
