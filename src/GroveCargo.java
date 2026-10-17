package dev.libertycraft;

import java.util.*;
import dev.libertycraft.link.*;
import net.fabricmc.fabric.api.attachment.v1.*;
import net.minecraft.resources.Identifier;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.SimpleContainer;
import net.minecraft.world.SimpleMenuProvider;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.inventory.ChestMenu;
import net.minecraft.world.inventory.CraftingMenu;
import net.minecraft.world.inventory.ContainerLevelAccess;
import net.minecraft.world.item.Items;
import net.minecraft.world.item.ItemStack;
import net.minecraft.network.chat.Component;

/** CJ's expedition luggage: one persistent inventory per player/world, accessed at a parked car.
 * Never keyed by a recyclable GTA pool pointer. Switching cars cannot clone or lose the luggage. */
public final class GroveCargo {
    private GroveCargo() {}
    static final int READY=1<<20;
    static final AttachmentType<List<ItemStack>> CARGO=AttachmentRegistry.<List<ItemStack>>builder()
        .persistent(ItemStack.OPTIONAL_CODEC.listOf()).copyOnDeath()
        .syncWith(ItemStack.OPTIONAL_LIST_STREAM_CODEC,AttachmentSyncPredicate.all())
        .buildAndRegister(Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"expedition_cargo_v1"));
    public static List<ItemStack> visualItems(Player player){
        var cargo=player.getAttached(CARGO);
        return cargo==null?List.of():cargo.stream().filter(s->!s.isEmpty()).limit(6).map(ItemStack::copy).toList();
    }
    public static void register() {
        net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents.END_SERVER_TICK.register(server->{
            if(!Link.active()||server.getTickCount()%4!=0)return;
            for(var p:server.getPlayerList().getPlayers()){
                var cargo=p.getAttached(CARGO);boolean any=cargo!=null&&cargo.stream().anyMatch(stack->!stack.isEmpty());
                Link.pushEvent(49,(hasWorkbench(p)?1:0)|(any?2:0),0,0,0,0,0);
            }
        });
    }
    static boolean eligible(ServerPlayer player,int id) {
        Link.SkyState sky=new Link.SkyState();Link.readSkyState(sky);
        if(!Link.active()||!GroveCargoRules.safeFlags(sky.flags,true)||!player.isAlive()||id<=0)return false;
        var actors=new ArrayList<Link.Actor>();long[] stamp={0};
        if(!Link.readActors(actors,stamp)||stamp[0]<=0)return false;
        long now=Link.qpc(),frequency=Link.qpcFrequency();
        double age=frequency>0?(double)now/frequency-stamp[0]/10_000_000.0:Double.NaN;
        if(!Double.isFinite(age)||age<-.1||age>.5)return false;
        for(var a:actors)if((a.formId()&0xff000000)==0x56000000&&((a.formId()&0x00fffff0)>>>4)==id
            &&(a.flags()&(READY|Proto.ACTOR_VEHICLE))==(READY|Proto.ACTOR_VEHICLE)
            &&(a.flags()&(Proto.ACTOR_MISSION|Proto.ACTOR_OCCUPIED|Proto.ACTOR_DEAD))==0
            &&player.position().distanceToSqr(new net.minecraft.world.phys.Vec3(a.x(),a.y(),a.z()))<49)return true;
        return false;
    }
    static final class Luggage extends SimpleContainer {
        private final ServerPlayer owner;private final int vehicle;private boolean loaded;
        Luggage(ServerPlayer owner,int vehicle) {
            super(27);this.owner=owner;this.vehicle=vehicle;
            List<ItemStack> saved=owner.getAttached(CARGO);
            if(saved!=null)for(int i=0;i<Math.min(27,saved.size());i++)super.setItem(i,saved.get(i).copy());
            loaded=true;
        }
        @Override public void setChanged() {
            super.setChanged();if(!loaded)return;
            var saved=new ArrayList<ItemStack>(27);
            for(int i=0;i<27;i++)saved.add(getItem(i).copy());
            owner.setAttached(CARGO,List.copyOf(saved));
        }
        @Override public boolean stillValid(Player player) {return player==owner&&eligible(owner,vehicle);}
    }
    public static boolean open(ServerPlayer player,int id) {
        Link.SkyState sky=new Link.SkyState();Link.readSkyState(sky);
        if(!GroveCargoRules.safeFlags(sky.flags,false))return false;
        if(!eligible(player,id))return false;
        // A new snapshot must never coexist with an old menu's live inventory.
        player.closeContainer();
        var luggage=new Luggage(player,id);
        player.openMenu(new SimpleMenuProvider((menu,inv,p)->ChestMenu.threeRows(menu,inv,luggage),
            Component.literal("Equipaje de expedición")));
        LibertyCraft.LOG.info("[GroveCraft] Cargo opened for vehicle lease {}",id);
        return true;
    }
    static boolean hasWorkbench(ServerPlayer player){
        var cargo=player.getAttached(CARGO);
        return cargo!=null&&cargo.stream().anyMatch(stack->stack.is(Items.CRAFTING_TABLE)&&!stack.isEmpty());
    }
    public static boolean workshop(ServerPlayer player,int id){
        Link.SkyState sky=new Link.SkyState();Link.readSkyState(sky);
        if(!GroveCargoRules.safeFlags(sky.flags,false)||!eligible(player,id))return false;
        if(!hasWorkbench(player)){
            player.sendSystemMessage(Component.literal("Guarda una mesa de crafteo en el equipaje (G) para usar el taller móvil."));return false;
        }
        player.closeContainer();
        // A real level access lets vanilla resolve recipes and return partial inputs
        // on close; eligibility replaces its stationary crafting-table block check.
        player.openMenu(new SimpleMenuProvider((menu,inv,p)->new CraftingMenu(menu,inv,ContainerLevelAccess.create(player.level(),player.blockPosition())){
            @Override public boolean stillValid(Player p){return p==player&&eligible(player,id)&&hasWorkbench(player);}
        },Component.literal("Taller móvil · crafteo 3 × 3")));
        LibertyCraft.LOG.info("[GroveCraft] Mobile workshop opened for vehicle lease {}",id);return true;
    }
}
