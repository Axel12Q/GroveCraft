package dev.libertycraft;

import dev.libertycraft.mixin.*;
import java.util.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.*;
import net.minecraft.core.BlockPos;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.entity.*;
import net.minecraft.world.entity.item.ItemEntity;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.level.block.state.BlockState;
import org.joml.Matrix4f;

/** Persistent display entities rotate one tree as a unit; logs become loot only on landing.
 * Save data belongs to each display, so unload/restart resumes without replaying a drop. */
public final class GroveTreeFalls {
    private static final String TAG="grovecraft_treefall_v1:";
    private record Fall(Display.BlockDisplay entity,int x,int y,int z,double dx,double dz,long start,boolean loot) {}
    private static final Map<UUID,Fall> falls=new LinkedHashMap<>();
    public static void register() {
        ServerEntityEvents.ENTITY_LOAD.register((entity,level)->{
            if(!(entity instanceof Display.BlockDisplay display))return;
            for(String tag:entity.entityTags())if(tag.startsWith(TAG))try{
                String[] p=tag.substring(TAG.length()).split(":");
                falls.put(entity.getUUID(),new Fall(display,Integer.parseInt(p[0]),Integer.parseInt(p[1]),Integer.parseInt(p[2]),
                    Double.valueOf(p[3]),Double.valueOf(p[4]),Long.parseLong(p[5]),Boolean.parseBoolean(p[6])));
            }catch(RuntimeException e){LibertyCraft.LOG.warn("Invalid saved tree fall",e);}
        });
        ServerEntityEvents.ENTITY_UNLOAD.register((entity,level)->falls.remove(entity.getUUID()));
        ServerLifecycleEvents.SERVER_STOPPED.register(server->falls.clear());
        ServerTickEvents.END_SERVER_TICK.register(server->{
            // Snapshot: discard triggers ENTITY_UNLOAD and removes from the original map.
            for(var f:List.copyOf(falls.values())) {
                var entity=f.entity();if(entity.isRemoved()||!(entity.level() instanceof ServerLevel level))continue;
                double t=Math.clamp((level.getGameTime()-f.start())/44.0,0,1),angle=t*t*(3-2*t)*Math.PI/2;
                Matrix4f matrix=new Matrix4f().rotate((float)angle,(float)f.dz(),0,(float)-f.dx())
                    .translate(f.x()-.5f,f.y(),f.z()-.5f);
                ((GroveTreeTransformMixin)entity).groveTransform(new com.mojang.math.Transformation(matrix));
                if(t<1)continue;
                BlockState state=((GroveTreeDisplayMixin)entity).groveBlock();
                if(f.loot()&&state.is(net.minecraft.tags.BlockTags.LOGS)) {
                    double x=entity.getX()+f.dx()*f.y()+f.x()*.2,z=entity.getZ()+f.dz()*f.y()+f.z()*.2;
                    double floor=entity.getY();
                    var ground=GroveEcology.wildlifeHeight((int)Math.floor(x),(int)Math.floor(z),floor);
                    if(ground.isPresent())floor=ground.getAsDouble();
                    var item=new ItemEntity(level,x,floor+.3,z,new ItemStack(state.getBlock()));
                    item.setDefaultPickUpDelay();GroveGameplay.preserveNativeDrop(item,floor);level.addFreshEntity(item);
                }
                entity.discard();
            }
        });
    }
    public static Display.BlockDisplay prepare(ServerLevel level,BlockPos root,int x,int y,int z,BlockState state,double dx,double dz,boolean loot) {
        if(falls.size()>2048)return null;
        var display=EntityTypes.BLOCK_DISPLAY.create(level,EntitySpawnReason.EVENT);if(display==null)return null;
        display.setPos(root.getX()+.5,root.getY(),root.getZ()+.5);
        ((GroveTreeDisplayMixin)display).groveSetBlock(state);
        ((GroveTreeTransformMixin)display).groveTransform(new com.mojang.math.Transformation(new Matrix4f().translate(x-.5f,y,z-.5f)));
        display.addTag(TAG+x+":"+y+":"+z+":"+Double.toString(dx)+":"+Double.toString(dz)+":"+level.getGameTime()+":"+loot);
        return display;
    }
}
