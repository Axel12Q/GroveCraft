package dev.libertycraft.mixin;
import dev.libertycraft.GroveVillageEconomy;
import net.minecraft.world.entity.npc.villager.*;
import net.minecraft.world.item.trading.MerchantOffers;
import org.spongepowered.asm.mixin.*;
import org.spongepowered.asm.mixin.injection.*;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/** Losing/changing a vanilla job must not destroy a GroveCraft shop's trade history. */
@Mixin(Villager.class)
public class GroveMarketProfessionMixin {
    @Unique private MerchantOffers groveSavedMarket;
    @Inject(method="setVillagerData",at=@At("HEAD"))
    private void groveKeepMarket(VillagerData data,CallbackInfo ci){
        var v=(Villager)(Object)this;
        groveSavedMarket=v.entityTags().contains("grovecraft_settler")&&v.entityTags().contains(GroveVillageEconomy.MARKET)?v.getOffers():null;
    }
    @Inject(method="setVillagerData",at=@At("RETURN"))
    private void groveRestoreMarket(VillagerData data,CallbackInfo ci){
        // Merchant.overrideOffers is a server-side no-op; restore the actual stored list.
        if(groveSavedMarket!=null){((GroveMarketOffersAccessor)this).groveRestoreOffers(groveSavedMarket);groveSavedMarket=null;}
    }
}
