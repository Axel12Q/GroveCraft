"""Owned SA fluid integration over the pinned MIT terrain-flow rules."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
guest=root/'build/guest/src/main/java/dev/libertycraft'
dig=guest/'world/HostDig.java'
s=dig.read_text(encoding='utf-8')
assert 'private static int worldFor(Level level)' in s
dig.write_text(s.replace('private static int worldFor(Level level)', 'public static int worldFor(Level level)'),encoding='utf-8')
p=root/'vendor/libertycraft/fabric/src/main/java/dev/libertycraft/mixin/FlowingFluidMixin.java'
s=p.read_text(encoding='utf-8')
marker='\t\tif (!HostCollision.isKnown(targetPos.getX(), targetPos.getY(), targetPos.getZ())) {'
assert marker in s
s=s.replace(marker,'''        if (level instanceof net.minecraft.world.level.Level actual
                && dev.libertycraft.world.HostDig.destruction && actual.hasChunkAt(targetPos)) {
            int world = dev.libertycraft.world.HostDig.worldFor(actual);
            // A dug void must accept water even under the original ground. The
            // original underground voxel remains for sampling; its mask wins.
            if (dev.libertycraft.world.HostDig.isDug(actual, world, targetPos)) return;
            int material = dev.libertycraft.world.HostDig.classify(targetPos.getX(), targetPos.getY(), targetPos.getZ());
            if (material != dev.libertycraft.world.HostDig.AIR) {
                cir.setReturnValue(false); return;
            }
        }
''' + marker)
(guest/'mixin/FlowingFluidMixin.java').write_text(s,encoding='utf-8')
print('Water uses dug voids; intact native terrain remains sealed.')

# One spare info bit identifies source cells: an empty bucket's reticle passes
# through flowing water to the source, just like vanilla SOURCE_ONLY rays.
export=root/'build/guest/src/client/java/dev/libertycraft/client/render/WorldExporter.java'
s=export.read_text(encoding='utf-8')
s=s.replace('(surface | kind << 4)', '(surface | kind << 4 | (fluid.isSource() ? 64 : 0))')
export.write_text(s,encoding='utf-8')
