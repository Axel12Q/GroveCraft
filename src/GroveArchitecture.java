package dev.libertycraft;
import dev.libertycraft.link.Link;
import dev.libertycraft.world.HostDig;
import net.minecraft.core.BlockPos;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.LevelEvent;
/** Surface-only cuts: no procedural stone interiors or neighbour reveal. */
public final class GroveArchitecture {
    public static final int FLAG=256;
    public static boolean cut(Level level,int world,BlockPos pos){return HostDig.isDug(level,world+512,pos);}
    public static int entryFace(Level level,int world,BlockPos pos){
        for(int face=0;face<6;face++)if(HostDig.isDug(level,world+768+face*256,pos))return face;
        return -1;
    }
    public static void open(ServerPlayer player,int world,BlockPos pos,int encoded){
        int material=encoded&255;var sky=new Link.SkyState();
        int entry=(encoded>>>9)&7;
        if((encoded&~4095)!=0||entry>6||material<1||material>32||GroveNativeTrees.wood(material)
            ||world<0||world>255||!Link.readSkyState(sky)||world!=sky.worldId
            ||!GroveEcologyRules.safe(sky.flags,sky.worldId,sky.inGame(),sky.loading()))return;
        var level=player.level();if(!HostDig.markDug(level,world+512,pos))return;
        if(entry!=0)HostDig.markDug(level,world+768+(entry-1)*256,pos);
        var state=HostDig.materialState(material);
        if(!player.isCreative()){
            var tool=player.getMainHandItem();var used=tool.copy();boolean harvest=player.hasCorrectToolForDrops(state);
            tool.mineBlock(level,state,pos,player);
            if(harvest)state.getBlock().playerDestroy(level,player,pos,state,null,used);
        }
        for(var drop:level.getEntitiesOfClass(net.minecraft.world.entity.item.ItemEntity.class,new net.minecraft.world.phys.AABB(pos).inflate(1)))
            GroveGameplay.preserveNativeDrop(drop,Math.min(pos.getY(),sky.y)+.1);
        level.levelEvent(LevelEvent.PARTICLES_AND_SOUND_DESTROY_BLOCK,pos,Block.getId(state));
        Link.pushEvent(45,world,pos.getX()+.5f,pos.getY()+.5f,pos.getZ()+.5f,0,0);
        LibertyCraft.LOG.info("[GroveCraft] native architecture cut {} material {} (one cell, no reveal)",pos,material);
    }
}
