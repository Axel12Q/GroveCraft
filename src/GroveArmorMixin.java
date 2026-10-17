package dev.libertycraft.mixin;

import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.damagesource.DamageSource;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Invoker;

/** Call vanilla protection and wear exactly once before forwarding a mob's hit to GTA. */
@Mixin(LivingEntity.class)
public interface GroveArmorMixin {
    @Invoker("getDamageAfterArmorAbsorb") float groveArmor(DamageSource source, float amount);
    @Invoker("getDamageAfterMagicAbsorb") float groveMagic(DamageSource source, float amount);
    @Invoker("hurtArmor") void groveWear(DamageSource source, float amount);
}
