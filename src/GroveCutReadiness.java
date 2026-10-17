package dev.libertycraft.world;

import java.util.HashSet;
import java.util.HashMap;
import net.minecraft.core.BlockPos;

/** Unknown original geometry is not air. Keep the last complete host mesh
 * until the small neighbourhood used by the lining probe is available. */
public final class GroveCutReadiness {
    @FunctionalInterface public interface Known { boolean region(int x,int y,int z); }
    private GroveCutReadiness(){}
    private static final HashMap<Long,Long> REQUESTS=new HashMap<>();
    public static void request(int sx,int sy,int sz,int world){
        long key=BlockPos.asLong(sx,sy,sz),now=System.nanoTime();
        if(now-REQUESTS.getOrDefault(key,now-1_000_000_001L)<1_000_000_000L)return;
        if(REQUESTS.size()>=128)REQUESTS.entrySet().removeIf(e->now-e.getValue()>2_000_000_000L);
        if(REQUESTS.size()>=128)return;
        REQUESTS.put(key,now);
        // Existing bounded survey event: ask for support around the entire
        // saved section, rather than requiring the player to look at each cut.
        dev.libertycraft.link.Link.pushEvent(46,world,sx*16+8,sy*16+8,sz*16+8,13,0,0);
    }
    public static boolean ready(int sx,int sy,int sz,long[] dug,Known known){
        if(dug==null)return true;
        var checked=new HashSet<Long>();
        for(int w=0;w<dug.length;w++)for(long mask=dug[w];mask!=0;mask&=mask-1){
            int bit=w*64+Long.numberOfTrailingZeros(mask);
            int x=sx*16+(bit&15),y=sy*16+(bit>>8),z=sz*16+((bit>>4)&15);
            for(int rx=(x-4)>>3;rx<=(x+4)>>3;rx++)
                for(int ry=(y-4)>>3;ry<=(y+4)>>3;ry++)
                    for(int rz=(z-4)>>3;rz<=(z+4)>>3;rz++){
                        long key=BlockPos.asLong(rx,ry,rz);
                        if(checked.add(key)&&!known.region(rx,ry,rz))return false;
                    }
        }
        return true;
    }
}
