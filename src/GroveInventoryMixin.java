package dev.libertycraft.client.mixin;

import dev.libertycraft.link.Link;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.screens.inventory.InventoryScreen;
import net.minecraft.world.entity.LivingEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/** CJ's native geometry replaces only our player's doll, never other GUI entities. */
@Mixin(InventoryScreen.class)
public class GroveInventoryMixin {
    @Inject(method="extractEntityInInventoryFollowsMouse", at=@At("HEAD"), cancellable=true)
    private static void groveCJ(GuiGraphicsExtractor gui, int left, int top, int right, int bottom,
            int size, float offset, float mouseX, float mouseY, LivingEntity entity, CallbackInfo ci) {
        var mc=Minecraft.getInstance();
        if (!Link.active() || entity != mc.player) return;
        int scale=mc.getWindow().getGuiScale();
        Link.pushEvent(39, 1, left*scale, top*scale, (right-left)*scale, (bottom-top)*scale, 0);
        ci.cancel();
    }
}
