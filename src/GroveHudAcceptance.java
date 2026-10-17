package dev.libertycraft;
import dev.libertycraft.link.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.minecraft.world.entity.*;
import net.minecraft.world.entity.npc.villager.Villager;
import net.minecraft.world.item.*;
import net.minecraft.world.level.storage.LevelResource;
/** Opt-in setup for a single private native HUD field world. Never runs in user campaigns. */
public final class GroveHudAcceptance {
    private static boolean ready;
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.hudAcceptance"))return;
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(ready||server.getPlayerList().getPlayers().isEmpty()||!java.util.Set.of("GroveCraftGtaTestHud20261013","GroveCraftGtaTestHudFinal20261013").contains(server.getWorldPath(LevelResource.ROOT).normalize().getFileName().toString()))return;
            var p=server.getPlayerList().getPlayers().getFirst();var sky=new Link.SkyState();
            if(!Link.active()||!Link.readSkyState(sky)||Link.hostSuspended()||!GroveCargoRules.safeFlags(sky.flags,false)
                ||p.position().distanceToSqr(new net.minecraft.world.phys.Vec3(sky.x,sky.y,sky.z))>.1)return;
            var v=EntityTypes.VILLAGER.create(p.level(),EntitySpawnReason.EVENT);if(v==null)return;
            v.snapTo(p.getX()+2.2,p.getY(),p.getZ(),180,0);
            v.setNoAi(true);v.setNoGravity(true);v.setPersistenceRequired();v.entityTags().add("grovecraft_settler");v.entityTags().add("grove_hud_probe");p.level().addFreshEntity(v);
            p.getInventory().add(new ItemStack(Items.OAK_LOG,8));p.getInventory().add(new ItemStack(Items.WHEAT,4));p.setAttached(GroveCargo.CARGO,java.util.List.of(new ItemStack(Items.CRAFTING_TABLE)));
            if(!GroveTownEconomy.open(p,v))throw new IllegalStateException("Private HUD merchant open failed");
            var state=GroveTownEconomy.state(p);var market=state.markets.values().iterator().next();GroveTownEconomy.choose(p,v,market,20);p.closeContainer();
            ready=true;LibertyCraft.LOG.info("[GroveCraft] HUD acceptance setup merchant {} at {} {} {}, job {}",v.getId(),v.getX(),v.getY(),v.getZ(),GroveTownEconomy.state(p).job!=null);
        });
    }
}
