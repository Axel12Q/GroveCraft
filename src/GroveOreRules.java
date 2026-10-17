package dev.libertycraft;
/** Depth below the native surface, for the compact forty-metre underground.
 * No surface sample means ordinary stone, never a guessed rich deposit. */
public final class GroveOreRules {
    private GroveOreRules(){}
    private static long mix(long x){x=(x^(x>>>30))*0xbf58476d1ce4e5b9L;x=(x^(x>>>27))*0x94d049bb133111ebL;return x^(x>>>31);}
    public static String ore(long seed,int x,int y,int z,double depth){
        if(!Double.isFinite(depth)||depth<5)return "stone";
        long vein=mix(seed^mix((long)(x>>1)*73856093L^(long)(y>>1)*19349663L^(long)(z>>1)*83492791L));
        long block=mix(vein^((x&1)|((y&1)<<1)|((z&1)<<2)));
        if((block&255)>=112)return "stone";
        int roll=(int)((vein>>>16)%10000);
        if(roll<180)return "coal_ore";
        if(depth>=10&&roll>=180&&roll<285)return "iron_ore";
        if(depth>=8&&roll>=285&&roll<355)return "copper_ore";
        if(depth>=24&&roll>=355&&roll<385)return "redstone_ore";
        if(depth>=22&&roll>=385&&roll<403)return "gold_ore";
        if(depth>=18&&roll>=403&&roll<418)return "lapis_ore";
        if(depth>=32&&roll>=418&&roll<422)return "diamond_ore";
        if(depth>=28&&roll>=422&&roll<424)return "emerald_ore";
        return "stone";
    }
}
