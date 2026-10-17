package dev.libertycraft;
import dev.libertycraft.link.Link;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.entity.ai.attributes.Attributes;
import net.minecraft.world.damagesource.*;
import net.minecraft.world.item.enchantment.EnchantmentHelper;
import net.minecraft.core.registries.Registries;

/** Cached vanilla protection for synchronous native hits; wear returns to the owning server. */
public final class GroveArmorState {
    public static void register(){
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(!Link.active()||server.getTickCount()%4!=0)return;
            for(var player:server.getPlayerList().getPlayers()){
                var arrow=new DamageSource(player.registryAccess().lookupOrThrow(Registries.DAMAGE_TYPE).getOrThrow(DamageTypes.ARROW));
                Link.pushEvent(40,0,player.getArmorValue(),(float)player.getAttributeValue(Attributes.ARMOR_TOUGHNESS),
                    EnchantmentHelper.getDamageProtection(player.level(),player,player.damageSources().mobAttack(player)),
                    EnchantmentHelper.getDamageProtection(player.level(),player,arrow),0);
            }
        });
    }
    public static void nativeWear(ServerPlayer player,int scaled,int weapon){
        if(scaled<=0||scaled>25600||weapon<0||weapon>34||!player.isAlive()||player.isCreative()||player.isSpectator())return;
        ((dev.libertycraft.mixin.GroveArmorMixin)(Object)player).groveWear(player.damageSources().mobAttack(player),scaled/256f);
    }
}
