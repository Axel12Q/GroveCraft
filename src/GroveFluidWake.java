package dev.libertycraft;

import net.minecraft.core.BlockPos;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.tags.FluidTags;

/** Retry nearby water after streamed terrain arrives; never load a new chunk. */
public final class GroveFluidWake {
    private static BlockPos center;
    private static int cursor;
    private static long next;
    private GroveFluidWake() {}
    public static void tick(ServerPlayer player) {
        if (System.nanoTime() < next) return;
        var feet = player.blockPosition();
        if (center == null || center.distSqr(feet) > 256) { center = feet; cursor = 0; }
        var level = player.level();
        var pos = new BlockPos.MutableBlockPos();
        for (int n=0; n<128 && cursor<17*17*17; ++n,++cursor) {
            pos.set(center.getX()+cursor%17-8,center.getY()+cursor/289-8,center.getZ()+cursor/17%17-8);
            if (!level.hasChunkAt(pos)) continue;
            var fluid = level.getFluidState(pos);
            if (fluid.is(FluidTags.WATER)) level.scheduleTick(pos.immutable(), fluid.getType(), fluid.getType().getTickDelay(level));
        }
        if (cursor==17*17*17) { center = feet; cursor=0; next=System.nanoTime()+1_000_000_000L; }
    }
    public static void reset() { center=null; cursor=0; next=0; }
}
