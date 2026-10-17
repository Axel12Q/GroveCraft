"""Reproducible visual-only patch; masks, liquids and excavation stay exact."""
from pathlib import Path
import shutil
root=Path(__file__).resolve().parents[1]
guest=root/'build/guest'
for name,side in [('GroveFoliageLod','main'),('GroveFoliageLodTest','test')]:
    shutil.copyfile(root/'src'/(name+'.java'),guest/('src/'+side+'/java/dev/libertycraft')/(name+'.java'))
p=guest/'src/client/java/dev/libertycraft/client/render/WorldExporter.java'
s=p.read_text(encoding='utf-8')
if 'groveLodModes' not in s:
    s=s.replace('private static final int SECTIONS_PER_FRAME', '''private static final java.util.Map<Long,Boolean> groveLodModes=new java.util.HashMap<>();
    private static long groveLodDue;
    private static double groveDistance(long key){
        var player=Minecraft.getInstance().player;if(player==null)return 0;
        return Math.hypot(SectionPos.x(key)*16+8-player.getX(),SectionPos.z(key)*16+8-player.getZ());
    }
    private static void groveRefreshLod(){
        long now=System.nanoTime();if(now<groveLodDue)return;groveLodDue=now+250_000_000L;
        int queued=0;
        for(long key:SENT.toLongArray()){
            boolean old=groveLodModes.getOrDefault(key,false);
            if(old!=dev.libertycraft.GroveFoliageLod.coarse(groveDistance(key),old)){
                // Approaching detail is prioritized; at most eight replacements per scan.
                if(old)markDirtyNow(SectionPos.x(key),SectionPos.y(key),SectionPos.z(key));
                else markDirty(SectionPos.x(key),SectionPos.y(key),SectionPos.z(key));
                if(++queued>=8)break;
            }
        }
    }
    private static final int SECTIONS_PER_FRAME''',1)
    s=s.replace('meshDirtySections(level);','groveRefreshLod();\n\t\tmeshDirtySections(level);',1)
    s=s.replace('SENT.clear();','SENT.clear();groveLodModes.clear();groveLodDue=0;',1)
    s=s.replace('SENT.remove(key);','SENT.remove(key);groveLodModes.remove(key);')
    marker='\t\tMESH.reset();'
    assert marker in s
    s=s.replace(marker,'''        boolean groveCoarse=dev.libertycraft.GroveFoliageLod.coarse(groveDistance(key),groveLodModes.getOrDefault(key,false));
        var groveMerged=groveCoarse&&!empty?dev.libertycraft.GroveFoliageLod.merged(i->{
            var state=section.getBlockState(i&15,i>>8,(i>>4)&15);
            return state.is(net.minecraft.tags.BlockTags.LEAVES)&&state.getFluidState().isEmpty()?net.minecraft.core.registries.BuiltInRegistries.BLOCK.getId(state.getBlock())+1:0;
        }):new java.util.BitSet();
'''+marker,1)
    marker='\t\t\t\t\t\t\tblockRenderer.tesselateBlock(MESH, x, y, z, level, pos.immutable(), state, model, state.getSeed(pos));'
    assert marker in s
    s=s.replace(marker,'''                            float[] groveUv=groveMerged.get(x+16*z+256*y)?cubeFaces(Minecraft.getInstance(),state):null;
                            if(groveUv!=null){
                                if(((x|y|z)&1)==0){
                                    int rgba=cubeTint(Minecraft.getInstance(),state);
                                    int argb=rgba==0?0xffffffff:0xff000000|((rgba&255)<<16)|(rgba&0xff00)|((rgba>>16)&255);
                                    MESH.groveLeafCube(x,y,z,groveUv,argb,(level.getBrightness(net.minecraft.world.level.LightLayer.SKY,pos)<<20)|(level.getBrightness(net.minecraft.world.level.LightLayer.BLOCK,pos)<<4));
                                }
                            }else{
'''+marker+'\n                            }',1)
    marker='if (GroveArchitectureLining.publish(sx, sy, sz, header, MESH.bytes(), terrainPackets)) {'
    assert marker in s
    s=s.replace(marker,marker+'\n            groveLodModes.put(key,groveCoarse);',1)
    marker='\t\t/** A dug hole\'s wall (DigWalls): an untinted opaque quad. */'
    assert marker in s
    s=s.replace(marker,'''        void groveLeafCube(float x,float y,float z,float[] uv,int tint,int light){
            float[][] faces={
                {0,0,0, 2,0,0, 2,0,2, 0,0,2},
                {0,2,2, 2,2,2, 2,2,0, 0,2,0},
                {2,0,0, 0,0,0, 0,2,0, 2,2,0},
                {0,0,2, 2,0,2, 2,2,2, 0,2,2},
                {0,0,0, 0,0,2, 0,2,2, 0,2,0},
                {2,0,2, 2,0,0, 2,2,0, 2,2,2}};
            ensure(36*Proto.REN_VERTEX_BYTES);
            for(Direction dir:Direction.values()){
                int f=dir.ordinal(),offset=(dir==Direction.UP?1:dir==Direction.DOWN?2:0)*4;
                float u0=uv[offset],v0=uv[offset+1],u1=uv[offset+2],v1=uv[offset+3];
                for(int k:new int[]{0,1,2,0,2,3})vertex(x+faces[f][k*3],y+faces[f][k*3+1],z+faces[f][k*3+2],
                    k==0||k==3?u0:u1,k<2?v1:v0,tint,light,flags(false,dir));
            }
        }
'''+marker,1)
if 'groveMergedPublications' not in s:
    s=s.replace('private static long groveLodDue;', 'private static long groveLodDue;\n    private static int groveMergedPublications;')
    s=s.replace('groveLodModes.put(key,groveCoarse);', '''groveLodModes.put(key,groveCoarse);
            if(!groveMerged.isEmpty()&&++groveMergedPublications<=16)
                LibertyCraft.LOG.info("[GroveCraft] foliage LOD section {} {} {}: {} leaf cells in {} coarse cubes at {}m",
                    sx,sy,sz,groveMerged.cardinality(),groveMerged.cardinality()/8,(int)groveDistance(key));''')
p.write_text(s,encoding='utf-8')
print('Foliage LOD patched; collision and save data unchanged.')
