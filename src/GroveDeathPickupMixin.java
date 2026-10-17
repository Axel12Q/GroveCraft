package dev.libertycraft.mixin;
import dev.libertycraft.link.Link;
import dev.libertycraft.world.city.BlockyCity;
import net.minecraft.world.entity.item.ItemEntity;
import net.minecraft.world.entity.player.Player;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.*;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
/** The mirror stays alive during GTA's Wasted; it must not re-pick its death loot. */
@Mixin(ItemEntity.class)
public abstract class GroveDeathPickupMixin {
    @Inject(method="playerTouch",at=@At("HEAD"),cancellable=true)
    private void groveWaitForNativeRespawn(Player player,CallbackInfo ci){
        var item=(ItemEntity)(Object)this;
        if(!item.entityTags().contains("grovecraft_death_drop")||!Link.active()||!BlockyCity.isMirror(player.level()))return;
        var sky=new Link.SkyState();
        if(!Link.readSkyState(sky)||!sky.inGame()||sky.loading()||sky.menuOpen()||Link.hostSuspended()
            ||(sky.flags&((1<<21)|(1<<22)|(1<<24)))!=0)ci.cancel();
    }
}
