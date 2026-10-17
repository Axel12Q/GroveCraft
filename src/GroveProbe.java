package dev.libertycraft;

import dev.libertycraft.link.Link;
import dev.libertycraft.link.Proto;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.minecraft.core.BlockPos;
import net.minecraft.world.level.block.Blocks;

/** Explicit lab-only marker: actual Minecraft blocks, exported by upstream's renderer. */
public final class GroveProbe {
    private static boolean placed;
    private static BlockPos previousOrigin;
    public static void register() {
        if (!Boolean.getBoolean("grovecraft.probeBlock")) return;
        ServerTickEvents.END_SERVER_TICK.register(server -> {
            if (!Link.active()) return;
            Link.SkyState state = new Link.SkyState();
            Link.readSkyState(state);
            if ((state.flags & Proto.SKY_IN_GAME) == 0 || server.getPlayerList().getPlayers().isEmpty()) return;
            if (!Double.isFinite(state.x) || !Double.isFinite(state.y) || !Double.isFinite(state.z)) return;
            if (Math.abs(state.x) > 10000 || Math.abs(state.z) > 10000 || state.y < -60 || state.y > 300) return;
            if (Math.hypot(state.x, state.z) < 100) return; // menu's diagnostic origin is not CJ
            var level = server.overworld();
            // A compact four-block marker near the diagnostic camera, not a replacement for building controls.
            BlockPos origin = BlockPos.containing(state.x + 3, state.y, state.z);
            if (placed && previousOrigin != null && previousOrigin.distSqr(origin) < 4096) return;
            level.getChunk(origin);
            level.setBlockAndUpdate(origin, Blocks.GRASS_BLOCK.defaultBlockState());
            level.setBlockAndUpdate(origin.above(), Blocks.OAK_LOG.defaultBlockState());
            level.setBlockAndUpdate(origin.east(), Blocks.CRAFTING_TABLE.defaultBlockState());
            level.setBlockAndUpdate(origin.east().above(), Blocks.GLASS.defaultBlockState());
            placed = true;
            previousOrigin = origin;
            LibertyCraft.LOG.info("[GroveCraft] REAL Minecraft probe blocks placed at {}", origin);
        });
    }
}
