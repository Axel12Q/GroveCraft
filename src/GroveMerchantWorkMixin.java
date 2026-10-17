package dev.libertycraft.mixin;
import dev.libertycraft.GroveVillageEconomy;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.entity.ai.behavior.WorkAtPoi;
import net.minecraft.world.entity.npc.villager.Villager;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.*;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
/** Settler work reach on quantized ground, with optional read-only test observation. */
@Mixin(WorkAtPoi.class)
public class GroveMerchantWorkMixin {
    private static double reach(double original,Villager v){
        // GTA ground is quantized in eighths, rather than integer vanilla floors.
        return v.entityTags().contains("grovecraft_settler")?original+.17:original;
    }
    @ModifyConstant(method="checkExtraStartConditions(Lnet/minecraft/server/level/ServerLevel;Lnet/minecraft/world/entity/npc/villager/Villager;)Z",constant=@Constant(doubleValue=1.73))
    private double groveStartReach(double original,ServerLevel level,Villager v){return reach(original,v);}
    @ModifyConstant(method="canStillUse(Lnet/minecraft/server/level/ServerLevel;Lnet/minecraft/world/entity/npc/villager/Villager;J)Z",constant=@Constant(doubleValue=1.73))
    private double groveContinuedReach(double original,ServerLevel level,Villager v,long time){return reach(original,v);}
    @Inject(method="start(Lnet/minecraft/server/level/ServerLevel;Lnet/minecraft/world/entity/npc/villager/Villager;J)V",at=@At("RETURN"))
    private void groveObserveActualWork(ServerLevel level,Villager v,long time,CallbackInfo ci){
        GroveVillageEconomy.observeWork(v,time);
    }
}
