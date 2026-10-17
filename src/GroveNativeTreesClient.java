package dev.libertycraft.client;

import dev.libertycraft.link.*;
import dev.libertycraft.world.HostDig;
import java.nio.*;
import java.util.*;
import net.minecraft.client.Minecraft;
import net.minecraft.world.level.chunk.status.ChunkStatus;

/** Wood and architecture masks, separate from the terrain/wall mesh exporter. */
public final class GroveNativeTreesClient {
    private record Key(int world,int x,int y,int z){}
    private static final Map<Key,long[]> SENT=new HashMap<>();
    private static int ticks;
    public static void reset(){SENT.clear();}
    public static void tick(Minecraft mc){
        if(mc.level==null||mc.player==null||!Link.active()){reset();return;}
        if(++ticks%5!=0)return;
        var next=new HashMap<Key,long[]>();int world=HostDigClient.world()+256;
        int cx=mc.player.getBlockX()>>4,cz=mc.player.getBlockZ()>>4,radius=mc.options.getEffectiveRenderDistance()+1;
        for(int dx=-radius;dx<=radius;dx++)for(int dz=-radius;dz<=radius;dz++){
            var chunk=mc.level.getChunkSource().getChunk(cx+dx,cz+dz,ChunkStatus.FULL,false);if(chunk==null)continue;
            var column=chunk.getAttached(HostDig.DUG);if(column==null)continue;
            for(var s:column.sections())if((s.world()==world||s.world()==world+256)&&s.bits().length==64)next.put(new Key(s.world(),cx+dx,s.sectionY(),cz+dz),s.bits());
        }
        var keys=new HashSet<>(next.keySet());keys.addAll(SENT.keySet());
        for(var key:keys){
            long[] bits=next.get(key);if(Arrays.equals(bits,SENT.get(key)))continue;
            if(key.world==HostDigClient.world()+512)
                dev.libertycraft.client.render.WorldExporter.markDirty(key.x,key.y,key.z);
            int count=0;if(bits!=null)for(long b:bits)count+=Long.bitCount(b);
            var header=ByteBuffer.allocate(24).order(ByteOrder.LITTLE_ENDIAN).putInt(key.x).putInt(key.y).putInt(key.z).putInt(count).putInt(key.world).putInt(0).flip();
            var data=ByteBuffer.allocate(count>0?512:0).order(ByteOrder.LITTLE_ENDIAN);
            if(count>0)for(long b:bits)data.putLong(b);
            if(Link.writeRender(Proto.REN_DUG,header,data.flip())){if(count>0)SENT.put(key,bits.clone());else SENT.remove(key);}
        }
    }
}
