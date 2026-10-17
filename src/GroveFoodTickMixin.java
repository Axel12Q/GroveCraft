package dev.libertycraft.mixin;
import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import com.llamalad7.mixinextras.injector.wrapoperation.WrapOperation;
import dev.libertycraft.link.Link;
import dev.libertycraft.world.city.BlockyCity;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.food.FoodData;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;

/** CJ regenerates once in the host, with one corresponding exhaustion payment.
 * Vanilla starvation, eating and saturation stay authoritative. */
@Mixin(FoodData.class)
public class GroveFoodTickMixin {
    @WrapOperation(method="tick",at=@At(value="INVOKE",target="Lnet/minecraft/server/level/ServerPlayer;isHurt()Z"))
    private boolean groveHostRegeneration(ServerPlayer player,Operation<Boolean> original){
        if(Link.active()&&BlockyCity.isMirror(player.level()))return false;
        return original.call(player);
    }
}
