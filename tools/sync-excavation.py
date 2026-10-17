"""Reproducible owned adaptations of the pinned MIT digging implementation."""
from pathlib import Path
import shutil
root = Path(__file__).resolve().parents[1]
source = root / 'vendor/libertycraft/fabric/src'
guest = root / 'build/guest/src'
# Private vertex bit: identify only the already-published lining for native
# reticle picking. Existing normal/cutout bits and protocol layout stay intact.
exporter=guest/'client/java/dev/libertycraft/client/render/WorldExporter.java'
exported=exporter.read_text(encoding='utf-8')
wall='void wall(float[] xyz, float[] uv, int light, Direction normal) {'
start=exported.index(wall);end=exported.index('// ---- block quads',start)
part=exported[start:end].replace('int flags = flags(false, normal);','int flags = flags(false, normal) | 256;')
exporter.write_text(exported[:start]+part+exported[end:],encoding='utf-8')
clip=guest/'main/java/dev/libertycraft/world/HostClip.java'
clipped=clip.read_text(encoding='utf-8')
clipped=clipped.replace('Proto.TRI_DIGGABLE | (dev.libertycraft.link.Proto.DIG_STONE',
    'Proto.TRI_DIGGABLE | dev.libertycraft.link.Proto.TRI_TERRAIN | (dev.libertycraft.link.Proto.DIG_STONE')
clip.write_text(clipped,encoding='utf-8')
walls=(source/'client/java/dev/libertycraft/client/render/DigWalls.java').read_text(encoding='utf-8')
walls=walls.replace('(float) t[0]', '(float) Math.clamp(t[0], .0001, .9999)')
walls=walls.replace('(float) t[1]', '(float) Math.clamp(t[1], .0001, .9999)')
walls=walls.replace('int a = 0, b = i, c = i + 1, d = Math.min(i + 2, world.size() - 1);',
    '''int a = 0, b = i, c = i + 1, d = Math.min(i + 2, world.size() - 1);
                if(this.material == dev.libertycraft.link.Proto.DIG_OAK_LOG || this.material == dev.libertycraft.link.Proto.DIG_SPRUCE_LOG)
                    rect=8; // exposed native wood interior, not bark on the cut walls''')
(guest/'client/java/dev/libertycraft/client/render/DigWalls.java').write_text(walls,encoding='utf-8')
for relative in ('main/java/dev/libertycraft/world/HostDig.java',
                 'client/java/dev/libertycraft/client/HostDigClient.java',
                 'client/java/dev/libertycraft/client/DestructionToggle.java'):
    text = (source / relative).read_text(encoding='utf-8')
    if relative.endswith('/HostDig.java'):
        text=text.replace('public static void open(ServerPlayer player, int world, BlockPos pos, int material) {',
                          'public static void open(ServerPlayer player, int world, BlockPos pos, int material) {\n        if(dev.libertycraft.GroveRoadside.busy(player))return;')
        text=text.replace('LevelChunk chunk = level.getChunkAt(pos);\n\t\tDugColumn column = column(chunk);\n\t\tif (column.isDug(world, pos.getX(), pos.getY(), pos.getZ())) {',
            '''if ((material & dev.libertycraft.GroveArchitecture.FLAG) != 0) {
            dev.libertycraft.GroveArchitecture.open(player,world,pos,material);return;
        }
        if (dev.libertycraft.GroveNativeTrees.wood(material)) {
            dev.libertycraft.GroveNativeTrees.open(player,world,pos,material);return;
        }
        LevelChunk chunk = level.getChunkAt(pos);
        DugColumn column = column(chunk);
        if (column.isDug(world, pos.getX(), pos.getY(), pos.getZ())) {''',1)
        text=text.replace('return material == DIG_STONE || material == DIG_NONE ? stoneOrOre(level.getSeed(), pos) : materialState(material);',
            'if(material != DIG_STONE && material != DIG_NONE)return materialState(material);\n        var land=landAbove(pos.getX()+.5,pos.getY()+.5,pos.getZ()+.5);\n        double depth=land==null?Double.NaN:land.heightAt(pos.getX()+.5,pos.getZ()+.5)-(pos.getY()+.5);\n        String ore=dev.libertycraft.GroveOreRules.ore(level.getSeed(),pos.getX(),pos.getY(),pos.getZ(),depth);\n        return net.minecraft.core.registries.BuiltInRegistries.BLOCK.getValue(net.minecraft.resources.Identifier.withDefaultNamespace(ore)).defaultBlockState();')
        text = text.replace('BEDROCK_DEPTH = 5.0', 'BEDROCK_DEPTH = 40.0')
        text = text.replace('LAND_REACH = 16.0', 'LAND_REACH = 48.0')
        text = text.replace('y + 32, z + 0.01', 'y + 48, z + 0.01')
        # Above land, its outward surface participates in the nearest-surface
        # decision too. Otherwise a farther protective wall under the pavement
        # classifies outdoor air as KEEP and absorbs every explosion ray.
        text = text.replace('double best = SEARCH * SEARCH;', '''double best = SEARCH * SEARCH;
            if (!Double.isNaN(ground) && py >= ground)
                best = Math.min(best, (py - ground) * (py - ground));''')
        text = text.replace('return farFromSurfaces(px, py, pz, ground);', '''if (!Double.isNaN(ground) && py >= ground) return AIR;
                return farFromSurfaces(px, py, pz, ground);''')
        # Missing exterior samples are not evidence of buried rock. The pinned
        # interior fallback otherwise grows stone pillars beside an outdoor hole.
        text = text.replace('return above != null || landBelow ? AIR : DIG_STONE;', 'return AIR;')
        # Architecture is mined through its separate surface mask, never through
        # recursive terrain reveal. A diggable material does not imply soil.
        text = text.replace('if (!nearest.diggable) {', 'if (!nearest.diggable || !nearest.terrain) {')
        text = text.replace('if (!above.diggable) {', 'if (!above.diggable || !above.terrain) {')
        # Keep the pinned MIT closest-point routine pure: querying geometry
        # must not initialize Fabric's world-attachment registry.
        math_start = text.index('\tstatic void closestPoint(')
        geometry = 'package dev.libertycraft.world;\n\nfinal class GroveDigGeometry {\n' + text[math_start:]
        (guest / 'main/java/dev/libertycraft/world/GroveDigGeometry.java').write_text(geometry, encoding='utf-8')
        text = text.replace('closestPoint(t, px, py, pz, this.q);',
                            'GroveDigGeometry.closestPoint(t, px, py, pz, this.q);')
    elif relative.endswith('/HostDigClient.java'):
        text=text.replace('int[] cell = HostRay.surfaceCell(hit);','int[] cell = GroveExcavation.surfaceCell(hit);')
        text=text.replace('if (HostDig.isDug(minecraft.level, world(), pos)) {',
            '''int material = hit.tri().material == 0 ? DIG_STONE : hit.tri().material;
        if (HostDig.isDug(minecraft.level, world(), pos)
            || (dev.libertycraft.GroveNativeTrees.wood(material) && dev.libertycraft.GroveNativeTrees.cut(minecraft.level,world(),pos))
            || (!hit.tri().terrain && !dev.libertycraft.GroveNativeTrees.wood(material) && dev.libertycraft.GroveArchitecture.cut(minecraft.level,world(),pos))) {''')
        text=text.replace('\t\tint material = hit.tri().material == 0 ? DIG_STONE : hit.tri().material;\n\t\tBlockState state', '\t\tBlockState state')
        text=text.replace('REVEAL.put(pos.asLong(), REVEAL_TRIES); // what\'s around it: blocks, or walls',
            'if (hit.tri().terrain && !dev.libertycraft.GroveNativeTrees.wood(material)) REVEAL.put(pos.asLong(), REVEAL_TRIES); // terrain only')
        text=text.replace('new LcNet.DigOpen(world(), pos, material)',
            'new LcNet.DigOpen(world(), pos, material | (!hit.tri().terrain && !dev.libertycraft.GroveNativeTrees.wood(material) ? dev.libertycraft.GroveArchitecture.FLAG | ((HostRay.dominantFace(hit.nx(),hit.ny(),hit.nz())+1)<<9) : 0))')
        text=text.replace('ticks++;', 'ticks++;\n        GroveNativeTreesClient.tick(minecraft);',1)
        text=text.replace('public static void resendAll() {', 'public static void resendAll() {\n        GroveNativeTreesClient.reset();')
        text=text.replace('BlockPos pos = BlockPos.of(key);\n\t\t\tboolean waiting = false;', '''BlockPos pos = BlockPos.of(key);
            int origin=HostDig.classify(pos.getX(),pos.getY(),pos.getZ());
            if(origin==dev.libertycraft.link.Proto.DIG_OAK_LOG || origin==dev.libertycraft.link.Proto.DIG_SPRUCE_LOG) {
                REVEAL.remove(key);continue; // a native trunk cut never voxelizes adjacent ground
            }
\t\t\tboolean waiting = false;''')
        text=text.replace('if (material <= HostDig.AIR) {',
            'if (material <= HostDig.AIR || material == dev.libertycraft.link.Proto.DIG_OAK_LOG || material == dev.libertycraft.link.Proto.DIG_SPRUCE_LOG) {')
        text = text.replace('private static float progress;',
            'private static float progress;\n\tpublic static float groveFraction() { return progress; }\n'
            '\tpublic static BlockPos grovePosition() { return mining; }')
        text = text.replace('WorldExporter.markDirty(sx, sy, sz);',
            'WorldExporter.markDirty(sx, sy, sz);\n\t\t\t\t\t\t\tgroveQueueHoles(level, chunk, sy);')
        text = text.replace('if (dugBits(chunk, sy) != null) {',
            'if (dugBits(chunk, sy) != null || dev.libertycraft.client.render.GroveArchitectureLining.count(chunk,sy)>0) {')
        text = text.replace('SEEN.put(key, column);', '''SEEN.put(key, column);
                for (var section : column.sections()) if (section.world() == world)
                    groveQueueHoles(level, chunk, section.sectionY());''')
        marker = '\tprivate static void reveal(ClientLevel level) {'
        helper = '''
    // Replay unfinished reveals after loading a saved hole or receiving its
    // original terrain. Filled cells are excluded: never flood-fill solid land.
    private static void groveQueueHoles(ClientLevel level, LevelChunk chunk, int sy) {
        if (!HostDig.destruction) return;
        long[] bits = dugBits(chunk, sy);
        var player = net.minecraft.client.Minecraft.getInstance().player;
        if (bits == null || player == null) return;
        for (int word = 0; word < bits.length && REVEAL.size() < 512; word++) {
            long mask = bits[word];
            while (mask != 0 && REVEAL.size() < 512) {
                int bit = word * 64 + Long.numberOfTrailingZeros(mask); mask &= mask - 1;
                BlockPos pos = new BlockPos(chunk.getPos().x() * 16 + (bit & 15), sy * 16 + (bit >> 8),
                    chunk.getPos().z() * 16 + ((bit >> 4) & 15));
                if (player.blockPosition().distSqr(pos) <= REVEAL_RANGE * REVEAL_RANGE && level.getBlockState(pos).isAir())
                    REVEAL.put(pos.asLong(), REVEAL_TRIES);
            }
        }
    }
'''
        assert marker in text
        text = text.replace(marker, helper + marker)
    else:
        text = text.replace('GTA IV', 'GTA SA').replace('Liberty City', 'San Andreas')
        text = text.replace('private static void load() {',
            'private static void load() {\n\t\tHostDig.destruction = Boolean.getBoolean("grovecraft.excavation");')
        text = text.replace('props.getProperty(KEY, "false")',
            'props.getProperty(KEY, System.getProperty("grovecraft.excavation", "false"))')
    (guest / relative).write_text(text, encoding='utf-8')
shutil.copyfile(root / 'src/GroveExcavation.java', guest / 'client/java/dev/libertycraft/client/GroveExcavation.java')
shutil.copyfile(root / 'src/GroveArchitectureLining.java', guest / 'client/java/dev/libertycraft/client/render/GroveArchitectureLining.java')
shutil.copyfile(root / 'src/GroveRenderBatch.java', guest / 'main/java/dev/libertycraft/link/GroveRenderBatch.java')
shutil.copyfile(root / 'src/GroveArchitectureCap.java', guest / 'main/java/dev/libertycraft/GroveArchitectureCap.java')
p=guest/'client/java/dev/libertycraft/client/render/WorldExporter.java'
text=p.read_text(encoding='utf-8')
text=text.replace('GroveArchitectureLining.add(level, chunk, sx, sy, sz, st -> cubeFaces(Minecraft.getInstance(), st), MESH::wall);',
    'if (!GroveArchitectureLining.add(level, chunk, sx, sy, sz, st -> cubeFaces(Minecraft.getInstance(), st), MESH::wall)) { markDirty(sx, sy, sz); return false; }')
text=text.replace('            if (!GroveArchitectureLining.publish(sx, sy, sz)) { markDirty(sx, sy, sz); return false; }\n','')
if 'int liningCount =' not in text:
    text=text.replace('int dugCount = 0;', '''int liningCount = GroveArchitectureLining.count(chunk, sy);
        int dugCount = 0;''')
    text=text.replace('&& dugCount == 0', '&& dugCount == 0 && liningCount == 0')
    text=text.replace('// Holes dug into GTA IV\'s ground:', '''if (!GroveArchitectureLining.add(level, chunk, sx, sy, sz, st -> cubeFaces(Minecraft.getInstance(), st), MESH::wall)) { markDirty(sx, sy, sz); return false; }
        // Holes dug into GTA IV's ground:''')
if 'GroveArchitectureLining.publish(sx, sy, sz, header, MESH.bytes())' not in text:
    text=text.replace('if (Link.writeRender(Proto.REN_SECTION, header, MESH.bytes())) {',
        'if (GroveArchitectureLining.publish(sx, sy, sz, header, MESH.bytes())) {')
p.write_text(text,encoding='utf-8')
# Keep the explosion echo guard while letting both games carve the same world.
# Section geometry, terrain cut mask and solid collision share one reservation.
# The new host can additionally finish these packets as a bounded transaction.
p=guest/'client/java/dev/libertycraft/client/render/WorldExporter.java'
text=p.read_text(encoding='utf-8')
if 'var terrainPackets =' not in text:
    start=text.index('\t\t\tif (solidCount > 0 || SOLID.contains(key)) {')
    end=text.index('\t\t\tif (liquidCount > 0 || WET.contains(key)) {',start)
    text=text[:start]+'''            if(solidCount>0)SOLID.add(key);else SOLID.remove(key);
'''+text[end:]
    start=text.index('\t\t\tif (dugCount > 0 || DUG.contains(key)) {')
    end=text.index('\t\t\tif (++meshesSent',start)
    text=text[:start]+'''            if(dugCount>0)DUG.add(key);else DUG.remove(key);
'''+text[end:]
    marker='\t\tByteBuffer header = ByteBuffer.allocate(16).order(ByteOrder.LITTLE_ENDIAN).putInt(sx).putInt(sy).putInt(sz).putInt(MESH.vertexCount()).flip();'
    assert marker in text
    text=text.replace(marker,'''        var terrainPackets = new java.util.ArrayList<dev.libertycraft.link.GroveRenderBatch.Packet>();
        if(solidCount>0||SOLID.contains(key))terrainPackets.add(dev.libertycraft.link.GroveRenderBatch.maskPacket(
            Proto.REN_SOLIDS,sx,sy,sz,0,SOLID_BITS));
        if(dugCount>0||DUG.contains(key))terrainPackets.add(dev.libertycraft.link.GroveRenderBatch.maskPacket(
            Proto.REN_DUG,sx,sy,sz,dev.libertycraft.client.HostDigClient.world(),dug));
'''+marker)
text=text.replace('GroveArchitectureLining.publish(sx, sy, sz, header, MESH.bytes())',
    'GroveArchitectureLining.publish(sx, sy, sz, header, MESH.bytes(), terrainPackets)')
p.write_text(text,encoding='utf-8')
relative = 'main/java/dev/libertycraft/combat/GtaBlasts.java'
text = (source / relative).read_text(encoding='utf-8')
text = text.replace('false, Explosion.BlockInteraction.KEEP);',
    'false, dev.libertycraft.world.HostDig.destruction && !fire ? '
    'Explosion.BlockInteraction.DESTROY_WITH_DECAY : Explosion.BlockInteraction.KEEP);')
(guest / relative).write_text(text, encoding='utf-8')
# A refresh must not replace a sealed excavation with an empty lining while
# original neighbouring columns are still arriving. This also gates the mask.
shutil.copyfile(root/'src/GroveCutReadiness.java',guest/'main/java/dev/libertycraft/world/GroveCutReadiness.java')
shutil.copyfile(root/'src/GroveCutReadinessTest.java',guest/'test/java/dev/libertycraft/world/GroveCutReadinessTest.java')
p=guest/'main/java/dev/libertycraft/world/HostCollision.java'
text=p.read_text(encoding='utf-8')
if 'boolean liningRegionKnown(' not in text:
    text=text.replace('public static boolean isKnown(int x, int y, int z) {', '''public static boolean liningRegionKnown(int rx,int ry,int rz) {
        long key=regionKey(rx,ry,rz);
        return epoch>=0&&TRIS.containsKey(key)&&VOXELS.containsKey(key);
    }

    public static boolean isKnown(int x, int y, int z) {''')
if 'epoch = -1; // readers must not publish' not in text:
    text=text.replace('private static void clear(int newEpoch) {\n', 'private static void clear(int newEpoch) {\n        epoch = -1; // readers must not publish a partial cold refresh\n')
p.write_text(text,encoding='utf-8')
p=guest/'client/java/dev/libertycraft/client/render/WorldExporter.java'
text=p.read_text(encoding='utf-8')
marker='\t\tMESH.reset();'
if 'int groveCutEpoch =' not in text:
    text=text.replace(marker,'''        int groveCutEpoch = dev.libertycraft.world.HostCollision.epoch();
        if(dugCount>0&&!dev.libertycraft.world.GroveCutReadiness.ready(sx,sy,sz,dug,
            dev.libertycraft.world.HostCollision::liningRegionKnown)) { dev.libertycraft.world.GroveCutReadiness.request(sx,sy,sz,dev.libertycraft.client.HostDigClient.world()); markDirty(sx,sy,sz); return false; }
'''+marker)
marker='        var terrainPackets ='
if 'groveCutEpoch != dev.libertycraft.world.HostCollision.epoch()' not in text:
    text=text.replace(marker,'''        if(dugCount>0&&(groveCutEpoch<0||groveCutEpoch != dev.libertycraft.world.HostCollision.epoch()
            ||!dev.libertycraft.world.GroveCutReadiness.ready(sx,sy,sz,dug,
                dev.libertycraft.world.HostCollision::liningRegionKnown))) { dev.libertycraft.world.GroveCutReadiness.request(sx,sy,sz,dev.libertycraft.client.HostDigClient.world()); markDirty(sx,sy,sz); return false; }
'''+marker)
p.write_text(text,encoding='utf-8')
p=guest/'client/java/dev/libertycraft/client/render/WorldExporter.java'
text=p.read_text(encoding='utf-8')
for close in ('))', ')))'):
    text=text.replace('::liningRegionKnown'+close+' { markDirty(sx,sy,sz);',
        '::liningRegionKnown'+close+' { dev.libertycraft.world.GroveCutReadiness.request(sx,sy,sz,dev.libertycraft.client.HostDigClient.world()); markDirty(sx,sy,sz);')
p.write_text(text,encoding='utf-8')

test_target = guest / 'test/java/dev/libertycraft/world/GroveExcavationTest.java'
test_target.parent.mkdir(parents=True, exist_ok=True)
shutil.copyfile(root / 'src/GroveExcavationTest.java', test_target)
# Integrated-server blast diagnostics: distinguish unavailable terrain from a
# successful blast that reached no solid cells. No world mutation is added.
relative = 'main/java/dev/libertycraft/world/HostDigBlast.java'
text = (source / relative).read_text(encoding='utf-8')
text = text.replace('Vec3 c = explosion.center();', '''Vec3 c = explosion.center();
        dev.libertycraft.LibertyCraft.LOG.info("[GroveCraft] blast probe at {}: destruction={} active={} known={}",
            c, HostDig.destruction, HostCollision.active(), HostCollision.isKnown((int)Math.floor(c.x), (int)Math.floor(c.y), (int)Math.floor(c.z)));''')
text = text.replace('this.cells.defaultReturnValue(Integer.MIN_VALUE);', '''this.cells.defaultReturnValue(Integer.MIN_VALUE);
        dev.libertycraft.LibertyCraft.LOG.info("[GroveCraft] blast surfaces: land={} other={} below={}",
            this.probe.land.size(), this.probe.tris.size(), this.probe.test(center.x, center.y - 1, center.z));''')
text = text.replace('if (this.opened.isEmpty()) {', '''dev.libertycraft.LibertyCraft.LOG.info("[GroveCraft] blast opened {} native cells ({} sampled)", this.opened.size(), this.cells.size());
        if (this.opened.isEmpty()) {''')
text = text.replace('result = HostDig.KEEP;', '''if (this.cells.isEmpty() && this.probe.surface != null) {
                            var s = this.probe.surface;
                            dev.libertycraft.LibertyCraft.LOG.info("[GroveCraft] blast stopped sample={} {} {} normal={} {} {} material={} diggable={} terrain={} boundsY={}..{}",
                                pos.getX()+sx,pos.getY()+sy,pos.getZ()+sz,s.nx,s.ny,s.nz,s.material,s.diggable,s.terrain,s.minY,s.maxY);
                        }
                        result = HostDig.KEEP;''')
(guest / relative).write_text(text, encoding='utf-8')
relative = 'main/java/dev/libertycraft/mixin/ServerExplosionMixin.java'
text = (source / relative).read_text(encoding='utf-8')
text = text.replace(' || dev.libertycraft.combat.GtaBlasts.mirroring()\n\t\t\t? null : HostDigBlast.begin',
                    '\n\t\t\t? null : HostDigBlast.begin')
# The RETURN injection still suppresses EV_EXPLOSION while mirroring.
(guest / relative).write_text(text, encoding='utf-8')
