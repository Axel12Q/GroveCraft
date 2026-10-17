package dev.libertycraft.mixin;

import dev.libertycraft.GroveBucketTarget;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.BucketItem;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.ClipContext;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.phys.BlockHitResult;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;

@Mixin(BucketItem.class)
public abstract class GroveBucketMixin extends Item {
    protected GroveBucketMixin(Properties properties) { super(properties); }
    @Redirect(method="use", at=@At(value="INVOKE", target="Lnet/minecraft/world/item/BucketItem;getPlayerPOVHitResult(Lnet/minecraft/world/level/Level;Lnet/minecraft/world/entity/player/Player;Lnet/minecraft/world/level/ClipContext$Fluid;)Lnet/minecraft/world/phys/BlockHitResult;"))
    private BlockHitResult groveReticle(Level level, Player player, ClipContext.Fluid fluid) {
        var hit = GroveBucketTarget.get();
        return hit != null ? hit : getPlayerPOVHitResult(level, player, fluid);
    }
}
