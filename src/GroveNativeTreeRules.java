package dev.libertycraft;

/** Pure wire/harvest policy. Seven elapsed Minecraft days, paused/saved with the world. */
public final class GroveNativeTreeRules {
    public static final long REGROW_TICKS=7L*24000;
    public static int count(int packed){return (packed>>>16)&31;}
    public static double length(int packed){return ((packed>>>29)&7)*4.;}
    public static double heading(int packed){return ((packed>>>21)&255)*Math.PI*2/256;}
    public static boolean regrow(long now,long harvested,double distanceSquared){
        return now>=harvested&&now-harvested>=REGROW_TICKS&&Double.isFinite(distanceSquared)&&distanceSquared>96*96;
    }
    public static double along(int index,int count,double length){return length*(index+.5)/count;}
}
