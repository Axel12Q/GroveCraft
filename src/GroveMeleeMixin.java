package dev.libertycraft.mixin;
import dev.libertycraft.GroveMeleeRules;
import dev.libertycraft.combat.HostActorEntity;
import dev.libertycraft.world.HostDrive;
import dev.libertycraft.world.HostPath;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.entity.Mob;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/** Use the same body reach for navigation and damage; otherwise mobs stop too far away. */
@Mixin(Mob.class)
public abstract class GroveMeleeMixin {
    @Inject(method="isWithinMeleeAttackRange",at=@At("HEAD"),cancellable=true)
    private void groveReach(LivingEntity target,CallbackInfoReturnable<Boolean> result){
        var mob=(Mob)(Object)this;
        if(!HostPath.applies(mob.level()))return;
        if(target instanceof HostActorEntity || target instanceof ServerPlayer player && HostDrive.drivenByHost(player))
            result.setReturnValue(GroveMeleeRules.reachable(mob.getBoundingBox(),target.getBoundingBox()));
    }
}
