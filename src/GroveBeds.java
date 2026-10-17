package dev.libertycraft;

import com.mojang.serialization.Codec;
import com.google.gson.*;
import dev.libertycraft.link.Link;
import dev.libertycraft.world.city.BlockyCity;
import net.fabricmc.fabric.api.attachment.v1.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.*;
import net.minecraft.core.BlockPos;
import net.minecraft.resources.Identifier;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.entity.EquipmentSlot;
import net.minecraft.world.entity.item.ItemEntity;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.level.block.AbstractBedBlock;
import net.minecraft.world.level.block.state.properties.BedPart;
import net.minecraft.world.phys.Vec3;
import java.util.*;

/** Persist real sleep locations and vanilla recoverable items; GTA owns death and CJ. */
public final class GroveBeds {
    private static final AttachmentType<String> SPAWN=AttachmentRegistry.<String>builder().persistent(Codec.STRING)
        .buildAndRegister(Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"bed_spawn_v1"));
    private static final AttachmentType<Boolean> DEATH=AttachmentRegistry.<Boolean>builder().persistent(Codec.BOOL)
        .buildAndRegister(Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"death_drops_v1"));
    public record SavedBed(int x,int y,int z,double sx,double sy,double sz){}
    private static boolean sleeping,requested;
    private static Vec3 lastGround;
    private static int groundTick;
    public static SavedBed saved(ServerPlayer p){
        try{return new Gson().fromJson(p.getAttached(SPAWN),SavedBed.class);}catch(RuntimeException e){return null;}
    }
    private static void reset(){sleeping=false;requested=false;lastGround=null;groundTick=0;}
    /** Called only after vanilla accepts enough deeply sleeping players.
     * Mirror ADVANCE_TIME is disabled because GTA owns the clock, but a bed is
     * an intentional time change, like /time, and must reach GTA through HostSky. */
    public static void finishNight(net.minecraft.server.level.ServerLevel level){
        if(!BlockyCity.isMirror(level)||!Link.active()||Link.hostSuspended()
            ||level.getGameRules().get(net.minecraft.world.level.gamerules.GameRules.ADVANCE_TIME))return;
        if(level.players().stream().noneMatch(p->p.isSleepingLongEnough()&&saved(p)!=null))return;
        level.dimensionType().defaultClock().ifPresent(clock->level.getServer().clockManager()
            .moveToTimeMarker(clock,net.minecraft.world.clock.ClockTimeMarkers.WAKE_UP_FROM_SLEEP));
    }
    public static void register(){
        ServerLifecycleEvents.SERVER_STARTED.register(s->reset());
        ServerLifecycleEvents.SERVER_STOPPED.register(s->reset());
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(!Link.active()||Link.hostSuspended()||server.getPlayerList().getPlayers().isEmpty())return;
            var p=server.getPlayerList().getPlayers().getFirst();var sky=new Link.SkyState();
            if(!BlockyCity.isMirror(p.level())||!Link.readSkyState(sky)||!sky.inGame()||sky.loading()||sky.menuOpen())return;
            boolean dying=(sky.flags&(1<<22))!=0,blocked=(sky.flags&(1<<21))!=0;
            if(!dying&&!blocked&&(sky.flags&(1<<17))!=0){lastGround=new Vec3(sky.x,sky.y,sky.z);groundTick=server.getTickCount();}
            if(p.isSleeping()&&!sleeping&&!dying&&sky.worldId==0){
                var pos=p.getSleepingPos().orElse(null);
                if(pos!=null&&lastGround!=null&&lastGround.distanceToSqr(Vec3.atCenterOf(pos))<36){
                    var value=new SavedBed(pos.getX(),pos.getY(),pos.getZ(),lastGround.x,lastGround.y,lastGround.z);
                    p.setAttached(SPAWN,new Gson().toJson(value));
                    p.sendSystemMessage(net.minecraft.network.chat.Component.literal("Cama guardada: reaparecerás aquí."));
                    LibertyCraft.LOG.info("[GroveCraft] bed spawn saved {}",value);
                }
            }
            sleeping=p.isSleeping();
            if(dying&&!Boolean.TRUE.equals(p.getAttached(DEATH))){
                if(dropInventory(p,lastGround!=null&&server.getTickCount()-groundTick<100?lastGround:null)){
                    p.setAttached(DEATH,true);requested=false;
                }
            }
            if(dying&&!blocked&&!requested&&Boolean.TRUE.equals(p.getAttached(DEATH))){
                requested=true;var bed=saved(p);
                if(bed!=null&&valid(p,bed)){
                    p.closeContainer();p.stopUsingItem();
                    Link.pushEvent(47,sky.teleportSeq,(float)bed.sx,(float)bed.sy,(float)bed.sz,0,0);
                    LibertyCraft.LOG.info("[GroveCraft] bed respawn requested {}",bed);
                }else if(bed!=null)p.sendSystemMessage(net.minecraft.network.chat.Component.literal("La cama está rota o bloqueada: reapareces en el hospital."));
            }
            if(!dying&&!blocked&&(sky.flags&(1<<24))==0){
                if(Boolean.TRUE.equals(p.getAttached(DEATH)))p.setAttached(DEATH,false);
                requested=false;
            }
        });
    }
    public static boolean valid(ServerPlayer p,SavedBed bed){
        if(!Double.isFinite(bed.sx+bed.sy+bed.sz)||Math.abs(bed.sx)>6000||Math.abs(bed.sz)>6000||bed.sy< -60||bed.sy>1000)return false;
        var pos=new BlockPos(bed.x,bed.y,bed.z);var level=p.level();
        // At most the two bed chunks and saved standing chunk, rather than forcing a region.
        level.getChunkAt(pos);var state=level.getBlockState(pos);
        if(!(state.getBlock() instanceof AbstractBedBlock))return false;
        var direction=state.getValue(AbstractBedBlock.FACING);
        var other=pos.relative(state.getValue(AbstractBedBlock.PART)==BedPart.HEAD?direction.getOpposite():direction);
        level.getChunkAt(other);var second=level.getBlockState(other);
        if(second.getBlock()!=state.getBlock()||second.getValue(AbstractBedBlock.PART)==state.getValue(AbstractBedBlock.PART)
            ||second.getValue(AbstractBedBlock.FACING)!=direction)return false;
        if(new Vec3(bed.sx,bed.sy,bed.sz).distanceToSqr(Vec3.atCenterOf(pos))>36)return false;
        var feet=BlockPos.containing(bed.sx,bed.sy,bed.sz);level.getChunkAt(feet);
        for(int y=0;y<=1;y++)if(!level.getBlockState(feet.above(y)).getCollisionShape(level,feet.above(y)).isEmpty())return false;
        return true;
    }
    public static boolean dropInventory(ServerPlayer p,Vec3 ground){
        p.closeContainer();p.stopUsingItem();if(p.isSleeping())p.stopSleepInBed(true,true);
        if(p.isCreative()||p.isSpectator())return true;
        var items=new ArrayList<ItemStack>();
        for(var stack:p.getInventory().getNonEquipmentItems())if(!stack.isEmpty())items.add(stack.copy());
        // MAINHAND aliases the selected inventory slot. Never drop it twice.
        for(var slot:EquipmentSlot.values())if(slot!=EquipmentSlot.MAINHAND){var s=p.getItemBySlot(slot);if(!s.isEmpty())items.add(s.copy());}
        var added=new ArrayList<ItemEntity>();
        for(int i=0;i<items.size();i++){
            double angle=i*2.399963;var e=new ItemEntity(p.level(),p.getX()+Math.cos(angle)*.3,p.getY()+.35,p.getZ()+Math.sin(angle)*.3,items.get(i));
            e.setDefaultPickUpDelay();e.setTarget(p.getUUID());e.setDeltaMovement(Math.cos(angle)*.08,.12,Math.sin(angle)*.08);
            e.addTag("grovecraft_death_drop");
            if(!p.level().addFreshEntity(e)){for(var a:added)a.discard();LibertyCraft.LOG.error("Death drops refused; inventory preserved");return false;}
            added.add(e);
        }
        // Inventory owns a cached selected-hand entry in EntityEquipment too.
        // Clearing the backing list alone leaves CJ holding a retained stack.
        p.getInventory().clearContent();
        for(var e:added)if(ground!=null&&ground.distanceToSqr(p.position())<9)GroveGameplay.preserveNativeDrop(e,ground.y);
        p.getInventory().setChanged();
        LibertyCraft.LOG.info("[GroveCraft] death inventory dropped: {} stacks at {}",added.size(),p.position());return true;
    }
}
