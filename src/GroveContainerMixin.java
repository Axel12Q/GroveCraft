package dev.libertycraft.client.mixin;

import net.minecraft.client.gui.screens.inventory.AbstractContainerScreen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

@Mixin(AbstractContainerScreen.class)
public interface GroveContainerMixin {
    @Accessor("leftPos") int groveLeft();
    @Accessor("topPos") int groveTop();
    @Accessor("imageWidth") int groveWidth();
    @Accessor("imageHeight") int groveHeight();
}
