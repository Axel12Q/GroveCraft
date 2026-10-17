package dev.libertycraft.link;

import java.lang.foreign.MemorySegment;
import java.lang.invoke.VarHandle;
import java.nio.ByteBuffer;
import java.util.List;
import static java.lang.foreign.ValueLayout.*;
import static dev.libertycraft.link.Proto.*;

/** Publish a section and its cut bounds at one release boundary. A full ring
 * leaves the previous complete section intact, rather than exposing half an update. */
public final class GroveRenderBatch {
    private static final int BEGIN=0x47434231,END=0x47434232;
    public record Packet(int type,ByteBuffer header,ByteBuffer body) {
        long size(){return (8L+header.remaining()+(body==null?0:body.remaining())+7)&~7L;}
    }
    private static final VarHandle LONG=JAVA_LONG.varHandle();
    private GroveRenderBatch(){}
    public static Packet maskPacket(int type,int x,int y,int z,int world,long[] bits){
        if(type!=REN_SOLIDS&&type!=REN_DUG)throw new IllegalArgumentException("Mask packet type");
        if(bits!=null&&bits.length!=64)throw new IllegalArgumentException("Section mask length");
        int count=0;if(bits!=null)for(long b:bits)count+=Long.bitCount(b);
        var header=ByteBuffer.allocate(type==REN_DUG?24:16).order(java.nio.ByteOrder.LITTLE_ENDIAN)
            .putInt(x).putInt(y).putInt(z).putInt(count);
        if(type==REN_DUG)header.putInt(world).putInt(0);
        var data=ByteBuffer.allocate(count==0?0:512).order(java.nio.ByteOrder.LITTLE_ENDIAN);
        if(count>0)for(long b:bits)data.putLong(b);
        return new Packet(type,header.flip(),data.flip());
    }
    public static long required(long head,long capacity,List<Packet> packets){
        long end=head;
        for(var p:packets){
            long size=p.size();if(size>capacity/2)return -1;
            long pos=end%capacity;if(pos+size>capacity)end+=capacity-pos;
            end+=size;
        }
        return end-head;
    }
    public static boolean publish(List<Packet> packets){
        synchronized(Link.class){
            if(Boolean.getBoolean("grovecraft.atomicSections")){
                if(packets.isEmpty()||packets.size()>32)return false;
                var framed=new java.util.ArrayList<Packet>();
                var count=ByteBuffer.allocate(4).order(java.nio.ByteOrder.LITTLE_ENDIAN).putInt(packets.size()).flip();
                framed.add(new Packet(BEGIN,count,null));framed.addAll(packets);
                framed.add(new Packet(END,count.duplicate(),null));packets=framed;
            }
            var s=Link.segment();if(s==null)return false;
            long base=OFF_RENDER_RING,head=s.get(JAVA_LONG,base+RR_HEAD);
            long tail=(long)LONG.getAcquire(s,base+RR_TAIL);
            long needed=required(head,RR_DATA_BYTES,packets);
            if(needed<0||needed>2*1024*1024||head<tail||head-tail>RR_DATA_BYTES||needed>RR_DATA_BYTES-(head-tail))return false;
            long end=head;
            for(var p:packets){
                long pos=end%RR_DATA_BYTES;
                if(pos+p.size()>RR_DATA_BYTES){
                    s.set(JAVA_INT,base+RR_DATA+pos,REN_PAD);
                    s.set(JAVA_INT,base+RR_DATA+pos+4,0);
                    end+=RR_DATA_BYTES-pos;pos=0;
                }
                long at=base+RR_DATA+pos;
                int bytes=p.header.remaining()+(p.body==null?0:p.body.remaining());
                s.set(JAVA_INT,at,p.type);s.set(JAVA_INT,at+4,bytes);
                MemorySegment.copy(MemorySegment.ofBuffer(p.header),0,s,at+8,p.header.remaining());
                if(p.body!=null&&p.body.hasRemaining())MemorySegment.copy(MemorySegment.ofBuffer(p.body),0,s,at+8+p.header.remaining(),p.body.remaining());
                end+=p.size();
            }
            LONG.setRelease(s,base+RR_HEAD,end);
            return true;
        }
    }
}
