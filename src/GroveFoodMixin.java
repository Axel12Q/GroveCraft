package dev.libertycraft.mixin;

import dev.libertycraft.GroveGameplay;
import net.minecraft.world.entity.player.Player;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/** Explicit GroveCraft meal requests may overeat; ordinary Minecraft eating is unchanged. */
@Mixin(Player.class)
public class GroveFoodMixin {
    @Inject(method = "canEat", at = @At("HEAD"), cancellable = true)
    private void groveMeal(boolean always, CallbackInfoReturnable<Boolean> result) {
        if (GroveGameplay.canOvereat(((Player)(Object)this).getUUID())) result.setReturnValue(true);
    }
}
