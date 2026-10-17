package dev.libertycraft;

import net.minecraft.world.phys.BlockHitResult;

/** Scoped server-side reticle hit, consumed by vanilla bucket use. */
public final class GroveBucketTarget {
    private static final ThreadLocal<BlockHitResult> HIT = new ThreadLocal<>();
    public static BlockHitResult get() { return HIT.get(); }
    public static void set(BlockHitResult hit) { HIT.set(hit); }
    public static void clear() { HIT.remove(); }
}
