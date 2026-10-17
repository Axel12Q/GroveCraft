package dev.libertycraft.mixin;
import net.minecraft.world.entity.Display;
import net.minecraft.world.level.block.state.BlockState;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Invoker;
@Mixin(Display.BlockDisplay.class)
public interface GroveTreeDisplayMixin {
    @Invoker("setBlockState") void groveSetBlock(BlockState state);
    @Invoker("getBlockState") BlockState groveBlock();
}
