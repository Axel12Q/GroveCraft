package dev.libertycraft.mixin;
import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import com.llamalad7.mixinextras.injector.wrapoperation.WrapOperation;
import dev.libertycraft.link.Link;
import dev.libertycraft.world.city.BlockyCity;
import net.minecraft.server.level.ServerPlayer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;

/** The mirror follows native coordinates; its movement packets are not a second
 * physical journey. GroveGameplay charges actual CJ movement once, excluding vehicles. */
@Mixin(ServerPlayer.class)
public class GroveMovementFoodMixin {
    @WrapOperation(method={"checkMovementStatistics","jumpFromGround"},
        at=@At(value="INVOKE",target="Lnet/minecraft/server/level/ServerPlayer;causeFoodExhaustion(F)V"))
    private void groveMovementCost(ServerPlayer player,float amount,Operation<Void> original){
        if(Link.active()&&BlockyCity.isMirror(player.level()))return;
        original.call(player,amount);
    }
}
