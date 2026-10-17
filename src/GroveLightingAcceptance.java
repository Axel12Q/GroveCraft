package dev.libertycraft;
import dev.libertycraft.link.Link;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.HorizontalDirectionalBlock;
import net.minecraft.core.Direction;
/** A single display beside CJ, only in the explicitly named lighting test world. */
public final class GroveLightingAcceptance {
    private static boolean done;
    private static net.minecraft.core.BlockPos lamp;
    public static void register(){
        if(!"GroveCraftGtaTestLighting20261013".equals(System.getProperty("grovecraft.testWorld")))return;
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(!Link.active()||Link.hostSuspended()||server.getPlayerList().getPlayers().isEmpty())return;
            var sky=new Link.SkyState();if(!Link.readSkyState(sky)||!sky.inGame()||sky.menuOpen()||sky.loading())return;
            var p=server.getPlayerList().getPlayers().getFirst();if(p.position().distanceToSqr(new net.minecraft.world.phys.Vec3(sky.x,sky.y,sky.z))>1)return;
            var command=net.fabricmc.loader.api.FabricLoader.getInstance().getGameDir().resolve("grovecraft-lighting-lamp.txt");
            if(done&&lamp!=null&&java.nio.file.Files.exists(command)){
                try{var value=java.nio.file.Files.readString(command).trim();java.nio.file.Files.delete(command);
                    p.level().setBlock(lamp,value.equals("on")?Blocks.GLOWSTONE.defaultBlockState():Blocks.AIR.defaultBlockState(),3);
                }catch(Exception e){LibertyCraft.LOG.error("Lighting fixture",e);}
            }
            if(done)return;
            var base=p.blockPosition().offset(-6,1,-1);
            for(int x=0;x<7;x++)for(int z=0;z<3;z++)p.level().setBlock(base.offset(x,-1,z),Blocks.OAK_PLANKS.defaultBlockState(),3);
            for(int y=0;y<4;y++)p.level().setBlock(base.offset(0,y,0),Blocks.OAK_LOG.defaultBlockState(),3);
            for(int x=-1;x<=1;x++)for(int z=-1;z<=1;z++)p.level().setBlock(base.offset(x,4,z),Blocks.OAK_LEAVES.defaultBlockState(),3);
            var bed=Blocks.BED.red().defaultBlockState().setValue(HorizontalDirectionalBlock.FACING,Direction.SOUTH);
            p.level().setBlock(base.offset(2,0,0),bed,3);
            p.level().setBlock(base.offset(2,0,1),bed.setValue(net.minecraft.world.level.block.BedBlock.PART,net.minecraft.world.level.block.state.properties.BedPart.HEAD),3);
            p.level().setBlock(base.offset(4,0,0),Blocks.CRAFTING_TABLE.defaultBlockState(),3);
            p.level().setBlock(base.offset(5,0,0),Blocks.STONE.defaultBlockState(),3);
            lamp=base.offset(6,0,0);p.level().setBlock(lamp,Blocks.GLOWSTONE.defaultBlockState(),3);
            done=true;LibertyCraft.LOG.info("[GroveCraft] lighting display placed {}",base);
        });
    }
}

