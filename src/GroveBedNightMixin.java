package dev.libertycraft.mixin;
import dev.libertycraft.GroveBeds;
import net.minecraft.server.level.ServerLevel;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.*;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
@Mixin(ServerLevel.class)
public abstract class GroveBedNightMixin {
    @Inject(method="tick",at=@At(value="INVOKE",target="Lnet/minecraft/server/level/ServerLevel;wakeUpAllPlayers()V"))
    private void groveSleepAdvancesNativeClock(java.util.function.BooleanSupplier keepTicking,CallbackInfo ci){
        GroveBeds.finishNight((ServerLevel)(Object)this);
    }
}
