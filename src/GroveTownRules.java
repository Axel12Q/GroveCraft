package dev.libertycraft;
/** Pure bounds for shared town income and progression. */
public final class GroveTownRules {
    private GroveTownRules(){}
    public static final int DAILY_INCOME=1000;
    public static boolean canEarn(int earned,int reward){return earned>=0&&reward>0&&reward<=5000&&(long)earned+reward<=DAILY_INCOME;}
    public static int price(int base,int reputation){return reputation>=3?Math.max(1,base*9/10):base;}
    public static boolean route(double ax,double az,double bx,double bz){
        double distance=Math.hypot(ax-bx,az-bz);return Double.isFinite(distance)&&distance>=300&&distance<=2500;
    }
    public static boolean safe(int flags,boolean ownMenu){return GroveCargoRules.safeFlags(flags,ownMenu)
        &&(flags&((1<<21)|(1<<22)|(1<<23)|(1<<24)))==0;}
}
