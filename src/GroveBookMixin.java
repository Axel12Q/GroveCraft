package dev.libertycraft.client.mixin;

import net.minecraft.client.gui.screens.inventory.BookViewScreen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Invoker;

/** Use the vanilla book's actual layout for the GTA overlay and input gate. */
@Mixin(BookViewScreen.class)
public interface GroveBookMixin {
    @Invoker("backgroundLeft") int groveBookLeft();
    @Invoker("backgroundTop") int groveBookTop();
    @Invoker("menuControlsTop") int groveBookControlsTop();
}
