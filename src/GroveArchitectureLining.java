package dev.libertycraft.client.render;
import dev.libertycraft.client.HostDigClient;
import dev.libertycraft.link.*;
import dev.libertycraft.world.*;
import java.util.*;
import java.nio.*;
import net.minecraft.client.multiplayer.ClientLevel;
import net.minecraft.core.*;
import net.minecraft.util.LightCoordsUtil;
import net.minecraft.world.level.LightLayer;
import net.minecraft.world.level.chunk.LevelChunk;
import net.minecraft.world.phys.AABB;

/** Closed cut chambers. Native masks are enabled only after this mesh and its
 * matching thin collision walls have reached the host. No inventory blocks. */
public final class GroveArchitectureLining {
    private record Key(int x,int y,int z){}
    private record Prepared(long[] bits,long[][] entry,Map<Integer,float[]> bounds,Map<Integer,Cell> cells){}
    private record Cell(BlockPos pos,int entry,float[] rects,int light,double[] low,double[] high){}
    private static final Map<Key,Prepared> PREPARED=new HashMap<>();
    private static int generation=-1,scope=-1;
    private static ClientLevel preparedLevel;
    private static final Set<String> DEFERRED=new HashSet<>();
    private static boolean deferred(BlockPos p,String reason){
        String key=p+" "+reason;
        if(DEFERRED.size()<128&&DEFERRED.add(key))dev.libertycraft.LibertyCraft.LOG.info("[GroveCraft] lining deferred {}: {}",p,reason);
        return false;
    }
    private GroveArchitectureLining(){}
    public static int count(LevelChunk chunk,int sy){
        var c=chunk.getAttached(HostDig.DUG);if(c==null)return 0;
        var bits=c.bits(HostDigClient.world()+512,sy);if(bits==null)return 0;
        int n=0;for(long b:bits)n+=Long.bitCount(b);return n;
    }
    static boolean add(ClientLevel level,LevelChunk chunk,int sx,int sy,int sz,DigWalls.Faces faces,DigWalls.Out out){
        if(preparedLevel!=level||generation!=Link.generation()||scope!=HostDigClient.world()){
            PREPARED.clear();DEFERRED.clear();preparedLevel=level;generation=Link.generation();scope=HostDigClient.world();
        }
        var key=new Key(sx,sy,sz);var ready=new long[64];var entries=new long[6][64];
        var previous=PREPARED.get(key);var retained=new HashMap<Integer,Cell>();
        var column=chunk.getAttached(HostDig.DUG);int world=HostDigClient.world();
        var bits=column==null?null:column.bits(world+512,sy);
        if(bits==null&&!PREPARED.containsKey(key))return true;
        var chambers=new TreeMap<Integer,float[]>();
        if(PREPARED.size()>1024)PREPARED.keySet().removeIf(k->Math.abs(k.x-sx)>32||Math.abs(k.z-sz)>32);
        if(bits==null){PREPARED.put(key,new Prepared(ready,entries,chambers,retained));return true;}
        var pending=new ArrayList<Cell>();
        for(int word=0;word<bits.length;word++)for(long mask=bits[word];mask!=0;mask&=mask-1){
            int bit=word*64+Long.numberOfTrailingZeros(mask);
            var p=new BlockPos(sx*16+(bit&15),sy*16+(bit>>8),sz*16+((bit>>4)&15));
            var old=previous==null?null:previous.cells.get(bit);
            if(old!=null){
                // Original buildings are static. A later sparse sampler column
                // can contain only a fragment of a facade, even with material>0.
                // Do not shrink a confirmed cut or replace its material during
                // that refresh. Lighting still follows the current world.
                int light=LightCoordsUtil.pack(level.getBrightness(LightLayer.BLOCK,p),level.getBrightness(LightLayer.SKY,p));
                var cell=new Cell(p,old.entry,old.rects,light,old.low,old.high);
                pending.add(cell);retained.put(bit,cell);ready[word]|=1L<<(bit&63);entries[cell.entry][word]|=1L<<(bit&63);
                float[] bounds=new float[6];for(int j=0;j<3;j++){bounds[j]=(float)cell.low[j];bounds[j+3]=(float)cell.high[j];}
                chambers.put(bit,bounds);continue;
            }
            var surfaces=new ArrayList<HostTri>();HostCollision.originalSurfacesNear(new AABB(p).inflate(.01),surfaces);
            int entry=dev.libertycraft.GroveArchitecture.entryFace(level,world,p),material=0;
            double[] low={1,1,1},high={0,0,0};
            for(var t:surfaces){
                if(t.terrain||!t.diggable||t.material<=0)continue;
                int face=entry<0?HostRay.dominantFace(t.nx,t.ny,t.nz):entry;
                if(dev.libertycraft.GroveArchitectureCap.footprint(t,p,Direction.values()[face]).isEmpty())continue;
                if(entry>=0&&face!=entry)continue;
                entry=face;material=t.material;
                for(var v:dev.libertycraft.GroveArchitectureCap.surface(t,p))for(int j=0;j<3;j++){
                    low[j]=Math.min(low[j],v[j]);high[j]=Math.max(high[j],v[j]);
                }
            }
            // New cuts wait until their original surface can be resolved.
            if(material==0||entry<0){
                return deferred(p,"surface count="+surfaces.size()+" material="+material+" entry="+entry);
            }
            var entryDirection=Direction.values()[entry];
            int entryAxis=entryDirection.getAxis()==Direction.Axis.X?0:entryDirection.getAxis()==Direction.Axis.Y?1:2;
            if(entryDirection.getAxisDirection()==Direction.AxisDirection.NEGATIVE)high[entryAxis]=1;
            else low[entryAxis]=0;
            if(high[0]-low[0]<.081||high[1]-low[1]<.081||high[2]-low[2]<.081)return deferred(p,"thin bounds "+Arrays.toString(low)+" "+Arrays.toString(high)+" entry="+entry);
            float[] bounds=new float[6];for(int j=0;j<3;j++){bounds[j]=(float)low[j];bounds[j+3]=(float)high[j];}
            var rects=faces.of(HostDig.materialState(material));if(rects==null)return false;
            int light=LightCoordsUtil.pack(level.getBrightness(LightLayer.BLOCK,p),level.getBrightness(LightLayer.SKY,p));
            ready[word]|=1L<<(bit&63);entries[entry][word]|=1L<<(bit&63);chambers.put(bit,bounds);
            var cell=new Cell(p,entry,rects,light,low,high);
            pending.add(cell);retained.put(bit,cell);
        }
        PREPARED.put(key,new Prepared(ready,entries,chambers,retained));
        for(var cell:pending){
            var p=cell.pos;int entry=cell.entry,light=cell.light;var rects=cell.rects;var low=cell.low;var high=cell.high;
            // Close visual boundaries at the exact cut edge. Collision plates
            // extend .04m into the solid side, without an unlined visual rim.
            for(var d:Direction.values()){
                if(d.ordinal()==entry)continue;
                var neighbour=p.relative(d);
                if(dev.libertycraft.GroveArchitecture.cut(level,world,neighbour)&&connected(p,neighbour,d))continue;
                int axis=d.getAxis()==Direction.Axis.X?0:d.getAxis()==Direction.Axis.Y?1:2;
                int a=(axis+1)%3,b=(axis+2)%3,rect=axis==1?(d==Direction.UP?8:4):0;
                float[] xyz=new float[12],uv=new float[8];
                double[] base={p.getX()-sx*16,p.getY()-sy*16,p.getZ()-sz*16};
                for(int k=0;k<4;k++){
                    double u=(k==1||k==2)?1:0,v=k>=2?1:0;
                    double[] q=base.clone();q[axis]+=d.getAxisDirection()==Direction.AxisDirection.POSITIVE?high[axis]:low[axis];
                    q[a]+=low[a]+u*(high[a]-low[a]);q[b]+=low[b]+v*(high[b]-low[b]);
                    for(int j=0;j<3;j++)xyz[k*3+j]=(float)q[j];
                    uv[k*2]=rects[rect]+(float)Math.clamp(u,.0001,.9999)*(rects[rect+2]-rects[rect]);
                    uv[k*2+1]=rects[rect+1]+(float)Math.clamp(v,.0001,.9999)*(rects[rect+3]-rects[rect+1]);
                }
                out.quad(xyz,uv,light,d.getOpposite());
            }
        }
        return true;
    }
    private static boolean connected(BlockPos p,BlockPos neighbour,Direction d){
        var key=new Key(p.getX()>>4,p.getY()>>4,p.getZ()>>4);
        if(!key.equals(new Key(neighbour.getX()>>4,neighbour.getY()>>4,neighbour.getZ()>>4)))return false;
        var section=PREPARED.get(key);if(section==null)return false;
        int b=(p.getX()&15)+16*(p.getZ()&15)+256*(p.getY()&15);
        int nb=(neighbour.getX()&15)+16*(neighbour.getZ()&15)+256*(neighbour.getY()&15);
        var a=section.bounds.get(b);var other=section.bounds.get(nb);if(a==null||other==null)return false;
        int axis=d.getAxis()==Direction.Axis.X?0:d.getAxis()==Direction.Axis.Y?1:2;
        boolean positive=d.getAxisDirection()==Direction.AxisDirection.POSITIVE;
        if(positive?(Math.abs(a[axis+3]-1)>.001||Math.abs(other[axis])>.001)
                   :(Math.abs(a[axis])>.001||Math.abs(other[axis+3]-1)>.001))return false;
        for(int j=0;j<3;j++)if(j!=axis&&(other[j]>a[j]+.001||other[j+3]<a[j+3]-.001))return false;
        return true;
    }
    public static boolean publish(int sx,int sy,int sz,ByteBuffer meshHeader,ByteBuffer mesh,List<GroveRenderBatch.Packet> terrain){
        var packets=new ArrayList<GroveRenderBatch.Packet>();
        packets.add(new GroveRenderBatch.Packet(Proto.REN_SECTION,meshHeader,mesh));
        packets.addAll(terrain);
        var prepared=PREPARED.get(new Key(sx,sy,sz));
        if(prepared==null)return GroveRenderBatch.publish(packets);
        for(int face=0;face<6;face++)packets.add(packet(sx,sy,sz,HostDigClient.world()+768+face*256,prepared.entry[face]));
        int count=prepared.bounds.size();
        if("GroveCraftGtaTestCuts20261014".equals(System.getProperty("grovecraft.testWorld"))){
            String payload=prepared.bounds.entrySet().stream().map(e->e.getKey()+":"+Arrays.toString(e.getValue())).collect(java.util.stream.Collectors.joining(";"));
            if(DEFERRED.size()<128&&DEFERRED.add("payload "+sx+","+sy+","+sz+" "+payload))
                dev.libertycraft.LibertyCraft.LOG.info("[GroveCraft] lining payload {},{},{} count={} {}",sx,sy,sz,count,payload);
        }
        var head=ByteBuffer.allocate(24).order(ByteOrder.LITTLE_ENDIAN).putInt(sx).putInt(sy).putInt(sz)
            .putInt(count).putInt(HostDigClient.world()+2304).putInt(count==0?0:0x47434331).flip();
        var data=ByteBuffer.allocate(count==0?0:512+count*24).order(ByteOrder.LITTLE_ENDIAN);
        if(count>0){for(long b:prepared.bits)data.putLong(b);for(var box:prepared.bounds.values())for(float f:box)data.putFloat(f);}
        packets.add(new GroveRenderBatch.Packet(Proto.REN_DUG,head,data.flip()));
        return GroveRenderBatch.publish(packets);
    }
    private static GroveRenderBatch.Packet packet(int sx,int sy,int sz,int world,long[] bits){
        int count=0;for(long b:bits)count+=Long.bitCount(b);
        var head=ByteBuffer.allocate(24).order(ByteOrder.LITTLE_ENDIAN).putInt(sx).putInt(sy).putInt(sz).putInt(count).putInt(world).putInt(0).flip();
        var data=ByteBuffer.allocate(count==0?0:512).order(ByteOrder.LITTLE_ENDIAN);
        if(count>0)for(long b:bits)data.putLong(b);
        return new GroveRenderBatch.Packet(Proto.REN_DUG,head,data.flip());
    }
}
