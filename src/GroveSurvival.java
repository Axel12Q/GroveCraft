package dev.libertycraft;

import dev.libertycraft.link.Link;
import dev.libertycraft.link.Proto;
import net.fabricmc.fabric.api.entity.event.v1.ServerLivingEntityEvents;
import net.fabricmc.fabric.api.event.Event;
import net.minecraft.resources.Identifier;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.damagesource.DamageTypes;
import net.minecraft.world.entity.Mob;
import net.minecraft.world.entity.projectile.Projectile;

/** Vanilla FoodData schedules starvation; GTA owns CJ's actual health. */
public final class GroveSurvival {
    private static boolean blocked;
    public static void register(){
        net.fabricmc.fabric.api.event.lifecycle.v1.ServerLifecycleEvents.SERVER_STARTED.register(server->blocked=false);
        net.fabricmc.fabric.api.event.lifecycle.v1.ServerLifecycleEvents.SERVER_STOPPED.register(server->blocked=false);
        net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents.END_SERVER_TICK.register(server->{
            var sky=new Link.SkyState();if(!Link.active()||!Link.readSkyState(sky))return;
            boolean next=(sky.flags&(1<<21))!=0;
            if(blocked&&!next&&(sky.flags&(1<<22))!=0&&sky.inGame()&&!sky.loading())for(var p:server.getPlayerList().getPlayers()){
                p.getFoodData().setFoodLevel(20);p.getFoodData().setSaturation(5);p.stopUsingItem();
            }
            blocked=next;
        });
        var phase=Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"grove_survival");
        ServerLivingEntityEvents.ALLOW_DAMAGE.addPhaseOrdering(phase,Event.DEFAULT_PHASE);
        ServerLivingEntityEvents.ALLOW_DAMAGE.addPhaseOrdering(phase,Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"player_vehicle_hits"));
        ServerLivingEntityEvents.ALLOW_DAMAGE.register(phase,(entity,source,amount)->{
            if(!Link.active()||!(entity instanceof ServerPlayer player))return true;
            var sky=new Link.SkyState();
            if(!Link.readSkyState(sky)||!sky.inGame()||sky.loading()||sky.menuOpen()||(sky.flags&(1<<21))!=0)return false;
            if(source.is(DamageTypes.STARVE)){
                if(player.getFoodData().getFoodLevel()==0&&!player.isCreative()&&!player.isSpectator()){
                    int floor=switch(player.level().getDifficulty()){case EASY->10;case NORMAL->1;default->0;};
                    Link.pushEvent(41,0,1,floor,0,0,0);
                }
                return false;
            }
            if(source.getEntity() instanceof Mob mob && !(source.getDirectEntity() instanceof Projectile)){
                // Reach uses actual body bounds, never a host's stale centre or follow range.
                var a=mob.getBoundingBox();var b=player.getBoundingBox();
                double dx=Math.max(0,Math.max(a.minX-b.maxX,b.minX-a.maxX));
                double dz=Math.max(0,Math.max(a.minZ-b.maxZ,b.minZ-a.maxZ));
                double dy=Math.max(0,Math.max(a.minY-b.maxY,b.minY-a.maxY));
                if(dx*dx+dz*dz>.75*.75||dy>.5)return false;
            }
            return true;
        });
    }
}
