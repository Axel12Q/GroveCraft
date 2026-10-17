package dev.libertycraft;
import dev.libertycraft.link.*;
import dev.libertycraft.world.HostClip;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.minecraft.core.BlockPos;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.npc.villager.Villager;
import net.minecraft.world.level.ClipContext;
import net.minecraft.world.level.block.*;
import net.minecraft.world.phys.*;

/** Read-only discovery/UI; J still revalidates reach, world, visibility and permissions server-side. */
public final class GroveInteractionHints {
    private GroveInteractionHints(){}
    static int kind(Block block){
        if(block instanceof BedBlock)return 2;
        if(block instanceof CraftingTableBlock)return 3;
        if(block instanceof AbstractFurnaceBlock)return 4;
        if(block instanceof ChestBlock||block instanceof BarrelBlock)return 5;
        return 0;
    }
    static boolean visible(ServerPlayer p,Vec3 point){
        Vec3 from=p.getEyePosition();double length=from.distanceTo(point);
        var nativeHit=HostClip.cast(from,point);
        if(nativeHit!=null&&nativeHit.t()*length<length-.15)return false;
        var hit=p.level().clip(new ClipContext(from,point,ClipContext.Block.COLLIDER,ClipContext.Fluid.NONE,p));
        return hit.getType()==HitResult.Type.MISS||hit.getLocation().distanceTo(point)<.9;
    }
    public static void register(){
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(server.getTickCount()%4!=0||!Link.active()||Link.hostSuspended()||server.getPlayerList().getPlayers().isEmpty())return;
            var p=server.getPlayerList().getPlayers().getFirst();var sky=new Link.SkyState();
            if(!Link.readSkyState(sky)||sky.worldId!=0||!p.isAlive()||p.isSleeping()
                ||(sky.flags&Proto.SKY_IN_GAME)==0||(sky.flags&(Proto.SKY_LOADING|Proto.SKY_MENU_OPEN|(1<<16)|(1<<19)|(1<<21)|(1<<22)))!=0){clear();return;}
            try{if(GroveTownEconomy.enabled()){
                var s=GroveTownEconomy.state(p);
                if(s.job==null)Link.pushEvent(51,0,0,0,0,0,0);
                else{
                    var m=s.markets.get(s.job.target);float height=m==null?(float)sky.y:(float)m.y;
                    int logs=Math.min(16,GroveTownEconomy.count(p,net.minecraft.world.item.Items.OAK_LOG));
                    int wheat=s.job.kind==0?Math.min(8,GroveTownEconomy.count(p,net.minecraft.world.item.Items.WHEAT)):0;
                    int progress=logs|(wheat<<8)|(s.pending!=null?1<<16:0);
                    Link.pushEvent(51,s.job.kind+1,(float)s.job.x,height,(float)s.job.z,0,progress);
                }
            }
            }catch(RuntimeException invalidLedger){Link.pushEvent(51,0,0,0,0,0,0);}
            if((sky.flags&Proto.SKY_IN_VEHICLE)!=0){Link.pushEvent(50,0,0,0,0,0,0);return;}
            var block=GroveInteraction.blockTarget(p);int k=block==null?0:kind(p.level().getBlockState(block.getBlockPos()).getBlock());
            if(k!=0){Vec3 point=Vec3.atCenterOf(block.getBlockPos());Link.pushEvent(50,k,(float)point.x,(float)point.y,(float)point.z,0,0);return;}
            if(GroveTownEconomy.enabled()){
                var villagers=p.level().getEntitiesOfClass(Villager.class,p.getBoundingBox().inflate(6),v->v.isAlive()&&v.entityTags().contains("grovecraft_settler")&&visible(p,v.getBoundingBox().getCenter()));
                var v=villagers.stream().min(java.util.Comparator.comparingDouble(p::distanceToSqr)).orElse(null);
                if(v!=null){Link.pushEvent(50,1,(float)v.getX(),(float)(v.getY()+v.getBbHeight()+.3),(float)v.getZ(),p.distanceToSqr(v)<9?1:0,v.getId());return;}
            }
            Link.pushEvent(50,0,0,0,0,0,0);
        });
    }
    private static void clear(){Link.pushEvent(50,0,0,0,0,0,0);Link.pushEvent(51,0,0,0,0,0,0);}
    public static boolean merchant(ServerPlayer p,int id){
        var e=p.level().getEntity(id);
        return e instanceof Villager v&&p.distanceToSqr(v)<9&&visible(p,v.getBoundingBox().getCenter())&&GroveTownEconomy.open(p,v);
    }
    public static boolean useBlock(ServerPlayer p,BlockPos pos){
        var sky=new Link.SkyState();
        if(!p.isAlive()||!Link.readSkyState(sky)||sky.worldId!=0||Link.hostSuspended()||!GroveCargoRules.safeFlags(sky.flags,false))return false;
        if(!p.level().hasChunkAt(pos)||kind(p.level().getBlockState(pos).getBlock())==0||p.getEyePosition().distanceToSqr(Vec3.atCenterOf(pos))>20.25||!visible(p,Vec3.atCenterOf(pos)))return false;
        p.gameMode.useItemOn(p,p.level(),p.getMainHandItem(),InteractionHand.MAIN_HAND,new BlockHitResult(Vec3.atCenterOf(pos),net.minecraft.core.Direction.UP,pos,false));return true;
    }
}
