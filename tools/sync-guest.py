"""Apply our owned source patches to the existing generated guest without replacing its world."""
from pathlib import Path
import shutil
import json
root = Path(__file__).resolve().parents[1]
shutil.copyfile(root/'src/GroveWorkshopAcceptance.java',root/'build/guest/src/main/java/dev/libertycraft/GroveWorkshopAcceptance.java')
workshop_init=root/'build/guest/src/main/java/dev/libertycraft/LibertyCraft.java'
if 'GroveWorkshopAcceptance.register();' not in workshop_init.read_text():
    workshop_init.write_text(workshop_init.read_text().replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveWorkshopAcceptance.register();'))
shutil.copyfile(root/'src/GroveLightingAcceptance.java',root/'build/guest/src/main/java/dev/libertycraft/GroveLightingAcceptance.java')
lighting_init=root/'build/guest/src/main/java/dev/libertycraft/LibertyCraft.java'
if 'GroveLightingAcceptance.register();' not in lighting_init.read_text():
    lighting_init.write_text(lighting_init.read_text().replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveLightingAcceptance.register();'))
for name in ('GroveStructureMap','GroveWorldPlan','GroveVillageEconomy','GroveSurveyRules','GroveCoexistenceAcceptance','GroveMarketWorkAcceptance','GroveBeds','GroveBedAcceptance','GroveCombatFieldAcceptance','GroveTownEconomy','GroveTownRules','GroveTownFieldAcceptance'):
    shutil.copyfile(root/'src'/(name+'.java'),root/'build/guest/src/main/java/dev/libertycraft'/(name+'.java'))
shutil.copyfile(root/'src/GroveSurveyRulesTest.java',root/'build/guest/src/test/java/dev/libertycraft/GroveSurveyRulesTest.java')
shutil.copyfile(root/'src/GroveStructureMapTest.java',root/'build/guest/src/test/java/dev/libertycraft/GroveStructureMapTest.java')
init_market=root/'build/guest/src/main/java/dev/libertycraft/LibertyCraft.java'
shutil.copyfile(root/'src/GroveTownRulesTest.java',root/'build/guest/src/test/java/dev/libertycraft/GroveTownRulesTest.java')
if 'GroveTownEconomy.register();' not in init_market.read_text():
    init_market.write_text(init_market.read_text().replace('GroveGameplay.register();','GroveTownEconomy.register();\n        GroveGameplay.register();'))
if 'GroveTownFieldAcceptance.register();' not in init_market.read_text():
    init_market.write_text(init_market.read_text().replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveTownFieldAcceptance.register();'))
if 'GroveCombatFieldAcceptance.register();' not in init_market.read_text():
    init_market.write_text(init_market.read_text().replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveCombatFieldAcceptance.register();'))
if 'GroveBedAcceptance.register();' not in init_market.read_text():
    init_market.write_text(init_market.read_text().replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveBedAcceptance.register();'))
if 'GroveBeds.register();' not in init_market.read_text():
    init_market.write_text(init_market.read_text().replace('GroveGameplay.register();','GroveBeds.register();\n        GroveGameplay.register();'))
if 'GroveVillageEconomy.register();' not in init_market.read_text():
    init_market.write_text(init_market.read_text().replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveVillageEconomy.register();\n        GroveCoexistenceAcceptance.register();'))
if 'GroveMarketWorkAcceptance.register();' not in init_market.read_text():
    init_market.write_text(init_market.read_text().replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveMarketWorkAcceptance.register();'))
shutil.copyfile(root/'src/GroveHamletPlan.java',root/'build/guest/src/main/java/dev/libertycraft/GroveHamletPlan.java')
shutil.copyfile(root/'src/GroveHamletPlanTest.java',root/'build/guest/src/test/java/dev/libertycraft/GroveHamletPlanTest.java')
shutil.copyfile(root/'src/GroveEconomyAcceptance.java',root/'build/guest/src/main/java/dev/libertycraft/GroveEconomyAcceptance.java')
shutil.copyfile(root/'src/GroveArchitecture.java',root/'build/guest/src/main/java/dev/libertycraft/GroveArchitecture.java')
shutil.copyfile(root/'src/GroveNativeTrees.java',root/'build/guest/src/main/java/dev/libertycraft/GroveNativeTrees.java')
shutil.copyfile(root/'src/GroveNativeTreeRules.java',root/'build/guest/src/main/java/dev/libertycraft/GroveNativeTreeRules.java')
shutil.copyfile(root/'src/GroveNativeTreeRulesTest.java',root/'build/guest/src/test/java/dev/libertycraft/GroveNativeTreeRulesTest.java')
shutil.copyfile(root/'src/GroveNativeTreesClient.java',root/'build/guest/src/client/java/dev/libertycraft/client/GroveNativeTreesClient.java')
import runpy
shutil.copyfile(root/'src/GroveReleasePaths.java',root/'build/guest/src/main/java/dev/libertycraft/GroveReleasePaths.java')
shutil.copyfile(root/'src/GroveReleasePathsTest.java',root/'build/guest/src/test/java/dev/libertycraft/GroveReleasePathsTest.java')
init_trees=root/'build/guest/src/main/java/dev/libertycraft/LibertyCraft.java'
if 'GroveEconomyAcceptance.register();' not in init_trees.read_text():
    init_trees.write_text(init_trees.read_text().replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveEconomyAcceptance.register();'))
if 'GroveNativeTrees.register();' not in init_trees.read_text():
    init_trees.write_text(init_trees.read_text().replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveNativeTrees.register();'))
shutil.copyfile(root / 'src/GroveSurvival.java', root / 'build/guest/src/main/java/dev/libertycraft/GroveSurvival.java')
shutil.copyfile(root / 'src/GroveBugAcceptance.java', root / 'build/guest/src/main/java/dev/libertycraft/GroveBugAcceptance.java')
shutil.copyfile(root / 'src/GroveClockRules.java', root / 'build/guest/src/main/java/dev/libertycraft/GroveClockRules.java')
shutil.copyfile(root / 'src/GroveMeleeRules.java', root / 'build/guest/src/main/java/dev/libertycraft/GroveMeleeRules.java')
init_survival=root / 'build/guest/src/main/java/dev/libertycraft/LibertyCraft.java'
if 'GroveNativeFellingTest.register();' not in init_survival.read_text():
    init_survival.write_text(init_survival.read_text().replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveNativeFellingTest.register();'))
if 'GroveSurvival.register();' not in init_survival.read_text():
    init_survival.write_text(init_survival.read_text().replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveSurvival.register();'))
if 'GroveBugAcceptance.register();' not in init_survival.read_text():
    init_survival.write_text(init_survival.read_text().replace('GroveSurvival.register();','GroveSurvival.register();\n        GroveBugAcceptance.register();'))
runpy.run_path(str(root / 'tools/sync-mobs.py'))
runpy.run_path(str(root / 'tools/sync-excavation.py'))
runpy.run_path(str(root / 'tools/sync-water.py'))
guest = root / 'build/guest'
shutil.copyfile(root / 'src/GroveFoodMixin.java', guest / 'src/main/java/dev/libertycraft/mixin/GroveFoodMixin.java')
mixins = guest / 'src/main/resources/libertycraft.mixins.json'
config = json.loads(mixins.read_text())
if 'GroveFoodMixin' not in config['mixins']:
    config['mixins'].append('GroveFoodMixin')
    mixins.write_text(json.dumps(config, indent=2) + '\n')
for name in ('GroveNativeFellingTest.java','GroveProbe.java', 'GroveGameplay.java', 'GroveAcceptance.java', 'GroveBucketTarget.java', 'GroveFluidWake.java', 'GroveInteraction.java', 'GroveInteractionAcceptance.java', 'GroveEcology.java', 'GroveEcologyRules.java', 'GroveEcologyAcceptance.java', 'GroveFieldGuide.java', 'GroveCargo.java', 'GroveCargoAcceptance.java'):
    shutil.copyfile(root / 'src' / name, guest / 'src/main/java/dev/libertycraft' / name)
shutil.copyfile(root / 'src/GroveCargoRules.java',guest / 'src/main/java/dev/libertycraft/GroveCargoRules.java')
shutil.copyfile(root / 'src/GroveBucketMixin.java', guest / 'src/main/java/dev/libertycraft/mixin/GroveBucketMixin.java')
shutil.copyfile(root / 'src/GroveUseMixin.java', guest / 'src/main/java/dev/libertycraft/mixin/GroveUseMixin.java')
config = json.loads(mixins.read_text())
if 'GroveBucketMixin' not in config['mixins']:
    config['mixins'].append('GroveBucketMixin')
    mixins.write_text(json.dumps(config, indent=2) + '\n')
if 'GroveUseMixin' not in config['mixins']:
    config['mixins'].append('GroveUseMixin')
    mixins.write_text(json.dumps(config, indent=2) + '\n')
shutil.copyfile(root / 'src/GroveClientUse.java', guest / 'src/client/java/dev/libertycraft/client/GroveClientUse.java')
shutil.copyfile(root / 'src/GroveInterface.java', guest / 'src/client/java/dev/libertycraft/client/GroveInterface.java')
shutil.copyfile(root / 'src/GroveContainerMixin.java', guest / 'src/client/java/dev/libertycraft/client/mixin/GroveContainerMixin.java')
shutil.copyfile(root / 'src/GroveBookMixin.java', guest / 'src/client/java/dev/libertycraft/client/mixin/GroveBookMixin.java')
shutil.copyfile(root/'src/GroveTownMenuMixin.java',guest/'src/client/java/dev/libertycraft/client/mixin/GroveTownMenuMixin.java')
client_mixins = guest / 'src/client/resources/libertycraft.client.mixins.json'
if not client_mixins.exists():
    client_mixins = guest / 'src/main/resources/libertycraft.client.mixins.json'
config_client = json.loads(client_mixins.read_text())
if 'GroveTownMenuMixin' not in config_client['client']:
    config_client['client'].append('GroveTownMenuMixin')
    client_mixins.write_text(json.dumps(config_client,indent=2)+'\n')
if 'GroveContainerMixin' not in config_client['client']:
    config_client['client'].append('GroveContainerMixin')
    client_mixins.write_text(json.dumps(config_client, indent=2) + '\n')
if 'GroveBookMixin' not in config_client['client']:
    config_client['client'].append('GroveBookMixin')
    client_mixins.write_text(json.dumps(config_client, indent=2) + '\n')
p = guest / 'src/client/java/dev/libertycraft/client/InputBridge.java'
s = p.read_text(encoding='utf-8')
marker = '\t\t\tcase Proto.IN_KEY ->'
patch = '''\t\t\tcase 32 -> {
                var life = new dev.libertycraft.link.Link.SkyState();
                if (dev.libertycraft.link.Link.readSkyState(life) && (life.flags & (1 << 21)) != 0
                    && code != 9 && code != 16 && code != 17 && code != 501 && code != 502 && code != 503) break;
                if (code == 11) { GroveExcavation.target(a, b, c); break; }
                if (code >= 6000 && code < 6384) { GroveExcavation.nativeWood(code,a,b,c); break; }
                if (code >= 7000 && code < 7006) { GroveExcavation.nativeCell(code,a,b,c); break; }
                if (code == 7100) { GroveExcavation.nativePoint(a,b,c); break; }
                if (code == 9 || code == 2) GroveExcavation.stop();
                if (code == 10) { GroveInterface.inventory(minecraft); break; }
                if (code == 4) GroveClientUse.start(minecraft, a);
                if (code == 15 || code == 25 || (code >= 400 && code <= 405) || (code >= 410 && code <= 415)) GroveClientUse.press(minecraft);
                if (code == 16 || code == 17) GroveClientUse.release(minecraft);
                var server = minecraft.getSingleplayerServer();
                if (server != null && minecraft.player != null) {
                    var uuid = minecraft.player.getUUID();
                    server.execute(() -> {
                        var player = server.getPlayerList().getPlayer(uuid);
                        if (player != null) dev.libertycraft.GroveGameplay.act(player, code, a, b, c);
                    });
                }
            }
'''
if 'case 32 ->' in s:
    start = s.index('\t\t\tcase 32 -> {')
    end = s.index(marker, start)
    s = s[:start] + patch + s[end:]
else:
    assert marker in s
    s = s.replace(marker, patch + marker)
if 'GroveClientUse.frame(minecraft);' not in s:
    s = s.replace('public static void drain(Minecraft minecraft) {', 'public static void drain(Minecraft minecraft) {\n\t\tGroveClientUse.frame(minecraft);')
p.write_text(s, encoding='utf-8')
if 'GroveExcavation.frame(minecraft);' not in s:
    s = s.replace('GroveClientUse.frame(minecraft);', 'GroveClientUse.frame(minecraft);\n\t\tGroveExcavation.frame(minecraft);')
    p.write_text(s, encoding='utf-8')
host = guest / 'src/client/java/dev/libertycraft/client/HostClient.java'
text = host.read_text()
if 'GroveInterface.frame(minecraft);' not in text:
    text = text.replace('FrameExporter.capture(minecraft);', 'GroveInterface.frame(minecraft);\n\t\t\tFrameExporter.capture(minecraft);')
    host.write_text(text)
print('Owned GroveCraft patches applied; worlds preserved.')

# Limit GPU readback/shared-memory copies, not host rendering or input polling.
# The linked client can otherwise ship an entire 800x600 frame at 80+ FPS.
frame = guest / 'src/client/java/dev/libertycraft/client/FrameExporter.java'
text = frame.read_text()
if 'groveNextCapture' not in text:
    text = text.replace('private static long nextFrameId = 1;',
        'private static long nextFrameId = 1;\n\tprivate static long groveNextCapture;')
    text = text.replace('shipReadyFrames();', '''shipReadyFrames();
        long groveNow = System.nanoTime();
        if (groveNow < groveNextCapture) return;
        groveNextCapture = groveNow + 33_333_333L;''', 1)
    frame.write_text(text)

# CJ supplies the body; Minecraft supplies real worn equipment and its rules.
shutil.copyfile(root / 'src/GroveArmorAcceptance.java',guest / 'src/main/java/dev/libertycraft/GroveArmorAcceptance.java')
shutil.copyfile(root / 'src/GroveArmorState.java',guest / 'src/main/java/dev/libertycraft/GroveArmorState.java')
init=guest / 'src/main/java/dev/libertycraft/LibertyCraft.java'
if 'GroveArmorAcceptance.register();' not in init.read_text():
    init.write_text(init.read_text().replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveArmorAcceptance.register();'))
if 'GroveArmorState.register();' not in init.read_text():
    init.write_text(init.read_text().replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveArmorState.register();'))
for name, side in (('GroveInventoryMixin','client'),('GroveArmorMixin','main'),('GroveMeleeMixin','main')):
    dest=guest / ('src/'+side+'/java/dev/libertycraft/'+('client/' if side=='client' else '')+'mixin/'+name+'.java')
    shutil.copyfile(root / ('src/'+name+'.java'),dest)
    cfg=client_mixins if side=='client' else mixins
    obj=json.loads(cfg.read_text()); key='client' if side=='client' else 'mixins'
    if name not in obj[key]:obj[key].append(name)
    cfg.write_text(json.dumps(obj,indent=2)+'\n')
p=guest / 'src/client/java/dev/libertycraft/client/render/AvatarExporter.java'
s=p.read_text()
s=s.replace('SCENE_MAX_ENTITIES = 48;', 'SCENE_MAX_ENTITIES = 128;') # one whole falling tree plus nearby mobs; packet vertex cap still applies
s=s.replace('SCENE_RANGE = 64.0;', 'SCENE_RANGE = 96.0;')
needle='Identifier texture = ((TextureBindingAccessor) binding).libertycraft$location();'
if 'groveEquipmentOnly' not in s:
    s=s.replace(needle,needle+'\n        // groveEquipmentOnly: only actual vanilla equipment, never the Minecraft skin.\n        if (this == RAGDOLL && !texture.getPath().contains("/equipment/")) return null;')
    start=s.index('private void sendParts(int message)')
    pos=s.index('if (groups.isEmpty()) {',start)
    s=s[:pos]+s[pos:].replace('if (groups.isEmpty()) {\n\t\t\treturn;', 'if (groups.isEmpty()) {\n            Link.tryWriteRender(message, ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN), null);\n\t\t\treturn;',1)
p.write_text(s)

if 'groveSkipHeld' not in s:
    marker='\t\tfor (Batch b : this.batches.values()) {'
    start=s.index('private void sendParts(int message)')
    pos=s.index(marker,start)
    s=s[:pos]+s[pos:].replace(marker,marker+'\n            // groveSkipHeld: item atlas batches belong to the separate hand renderer.\n            if (this == RAGDOLL && b.texture == 0) continue;',1)
    p.write_text(s)

# Forward only the unblocked part of Minecraft mob damage to CJ. Use vanilla's
# own item-blocking calculation (angle, delay, bypass tags, durability and disable).
p = guest / 'src/main/java/dev/libertycraft/combat/PlayerVehicleHits.java'
s = p.read_text(encoding='utf-8')
needle = 'hitNiko(player, source, amount);'
if 'groveVehicleFlag' not in s:
    s=s.replace('if (HostDrive.isMount(player.getVehicle())) {', '''var groveVehicleFlag=new Link.SkyState();
            if (HostDrive.isMount(player.getVehicle()) || (Link.readSkyState(groveVehicleFlag) && groveVehicleFlag.inGame() && !groveVehicleFlag.loading() && (groveVehicleFlag.flags & Proto.SKY_IN_VEHICLE)!=0)) {''')
    p.write_text(s,encoding='utf-8')
if needle in s:
    s = s.replace(needle, 'float unblocked = Math.max(0, amount - player.applyItemBlocking(player.level(), source, amount));\n\t\t\t\t\thitNiko(player, source, unblocked);')
    p.write_text(s, encoding='utf-8')
# The server excludes the miner from destruction broadcasts; export our authoritative progress too.
armor_hits=guest / 'src/main/java/dev/libertycraft/combat/PlayerVehicleHits.java'
armor_source=armor_hits.read_text()
needle='hitNiko(player, source, unblocked);'
if needle in armor_source:
    armor_source=armor_source.replace(needle, 'var protection=(dev.libertycraft.mixin.GroveArmorMixin)(Object)player;\n                    hitNiko(player, source, protection.groveMagic(source, protection.groveArmor(source, unblocked)));')
    armor_hits.write_text(armor_source)
p = guest / 'src/client/java/dev/libertycraft/client/render/WorldExporter.java'
s = p.read_text(encoding='utf-8')
held_marker = '\t\taddCracks(level);'
held_export = '''
        if (!minecraft.player.getMainHandItem().isEmpty()) {
            int before = ENTITIES.size();
            addItem(minecraft, level, minecraft.player, minecraft.player.getMainHandItem(), Vec3.ZERO, 0);
            if (ENTITIES.size() > before) {
                var held = ENTITIES.removeLast();
                ENTITIES.add(new Link.WorldEntity(held.kind() == Proto.WE_BLOCK ? 100 : 101, -1,
                    0,0,0,0,0,held.kind() == Proto.WE_BLOCK ? .24f : .48f,
                    null,held.uv(),held.tint()));
            }
        }
'''
if 'groveShieldUv' not in s and 'int before = ENTITIES.size();' not in s:
    s = s.replace(held_marker, held_export + held_marker)
s = s.replace('ItemDisplayContext.GROUND, level, null, 0', 'ItemDisplayContext.GROUND, level, minecraft.player, 0')
start = s.index('\tprivate static void addCracks(ClientLevel level) {')
end = s.index('\n\t\tfor (BlockDestructionProgress', start)
patch = '''
        BlockPos own = dev.libertycraft.GroveGameplay.miningPosition();
        float fraction = dev.libertycraft.GroveGameplay.miningFraction();
        if (own != null && fraction > 0 && ENTITIES.size() < Proto.MAX_WORLD_ENTITIES) {
            var shape = level.getBlockState(own).getShape(level, own);
            if(shape.isEmpty() && dev.libertycraft.GroveGameplay.virtualMining())shape=net.minecraft.world.phys.shapes.Shapes.block();
            if (!shape.isEmpty()) {
                AABB box = shape.bounds().move(own).inflate(.004);
                ENTITIES.add(new Link.WorldEntity(Proto.WE_CRACK, own.hashCode(), (float)box.minX,
                    (float)box.minY, (float)box.minZ, 0, 0, 1,
                    new float[]{(float)box.getXsize(), (float)box.getYsize(), (float)box.getZsize()},
                    atlas.crackUv(Math.min(9, (int)(fraction * 10))), 0));
            }
        }
        var terrainHit = dev.libertycraft.client.GroveExcavation.hit();
        float terrainFraction = dev.libertycraft.client.HostDigClient.groveFraction();
        if (terrainHit != null && terrainFraction > 0 && ENTITIES.size() < Proto.MAX_WORLD_ENTITIES) {
            ENTITIES.add(new Link.WorldEntity(102, 0, (float)terrainHit.x(), (float)terrainHit.y(),
                (float)terrainHit.z(), 0, 0, 1,
                new float[]{(float)terrainHit.nx(), (float)terrainHit.ny(), (float)terrainHit.nz()},
                atlas.crackUv(Math.min(9, (int)(terrainFraction * 10))), 0));
        }'''
s = s[:start] + '\tprivate static void addCracks(ClientLevel level) {' + patch + s[end:]
p.write_text(s, encoding='utf-8')
p = guest / 'src/main/java/dev/libertycraft/LibertyCraft.java'
s = p.read_text(encoding='utf-8')
original_world = 'WORLD_NAME = "GroveCraftLab"'
if original_world in s:
    s = s.replace(original_world, 'WORLD_NAME = Boolean.getBoolean("grovecraft.acceptance") ? "GroveCraftAcceptance" : "GroveCraftLab"')
    p.write_text(s, encoding='utf-8')
if 'GroveGameplay.register();' not in s:
    assert 'GroveProbe.register();' in s
    p.write_text(s.replace('GroveProbe.register();', 'GroveProbe.register();\n\t\tGroveGameplay.register();'), encoding='utf-8')
s = p.read_text(encoding='utf-8')
if 'GroveAcceptance.register();' not in s:
    p.write_text(s.replace('GroveGameplay.register();', 'GroveGameplay.register();\n\t\tGroveAcceptance.register();'), encoding='utf-8')
s = p.read_text(encoding='utf-8')
if 'GroveInteractionAcceptance.register();' not in s:
    p.write_text(s.replace('GroveGameplay.register();', 'GroveGameplay.register();\n\t\tGroveInteractionAcceptance.register();'), encoding='utf-8')
s = p.read_text(encoding='utf-8')
if 'GroveEcology.register();' not in s:
    p.write_text(s.replace('GroveGameplay.register();', 'GroveGameplay.register();\n\t\tGroveEcology.register();\n\t\tGroveEcologyAcceptance.register();'), encoding='utf-8')
shutil.copyfile(root / 'src/GroveEcologyRulesTest.java', guest / 'src/test/java/dev/libertycraft/GroveEcologyRulesTest.java')
shutil.copyfile(root / 'src/GroveCargoTest.java', guest / 'src/test/java/dev/libertycraft/GroveCargoTest.java')
shutil.copyfile(root / 'src/GroveRunOverRegressionTest.java', guest / 'src/test/java/dev/libertycraft/GroveRunOverRegressionTest.java')
shutil.copyfile(root / 'src/GroveClockRulesTest.java', guest / 'src/test/java/dev/libertycraft/GroveClockRulesTest.java')
shutil.copyfile(root / 'src/GroveMeleeRulesTest.java', guest / 'src/test/java/dev/libertycraft/GroveMeleeRulesTest.java')
s = p.read_text(encoding='utf-8')
if 'GroveFieldGuide.register();' not in s:
    p.write_text(s.replace('GroveEcology.register();', 'GroveEcology.register();\n\t\tGroveFieldGuide.register();'), encoding='utf-8')
if 'GroveCargo.register();' not in p.read_text():
    p.write_text(p.read_text().replace('GroveEcology.register();','GroveEcology.register();\n\t\tGroveCargo.register();'),encoding='utf-8')
if 'GroveCargoAcceptance.register();' not in p.read_text():
    p.write_text(p.read_text().replace('GroveCargo.register();','GroveCargo.register();\n\t\tGroveCargoAcceptance.register();'),encoding='utf-8')
# Optional fresh survival profile: explicit name, separate world, no creative laboratory kit.
p = guest / 'src/main/java/dev/libertycraft/kit/KitGiver.java'
s = p.read_text(encoding='utf-8')
if 'grovecraft.expedition' not in s:
    s = s.replace('loadConfig();', 'loadConfig();\n\t\tif (Boolean.getBoolean("grovecraft.expedition")) enabled = false;')
    s = s.replace('Given given = give(player, context.getSource().getServer());',
                  'if (Boolean.getBoolean("grovecraft.expedition")) { context.getSource().sendFailure(Component.literal("El kit de laboratorio está desactivado en Expedición.")); return 0; }\n\t\tGiven given = give(player, context.getSource().getServer());')
    p.write_text(s, encoding='utf-8')
p = guest / 'src/main/java/dev/libertycraft/LibertyCraft.java'
s = p.read_text(encoding='utf-8')
needle = 'WORLD_NAME = Boolean.getBoolean("grovecraft.acceptance")'
if needle in s:
    s = s.replace(needle, 'WORLD_NAME = Boolean.getBoolean("grovecraft.interactionAcceptance") ? "GroveCraftInteractionAcceptance" : Boolean.getBoolean("grovecraft.acceptance")')
    p.write_text(s, encoding='utf-8')
# Explicit local test-world override; normal launches always retain GroveCraftLab.
# Apply to the source, never replace the user's save directory.
p = guest / 'src/main/java/dev/libertycraft/LibertyCraft.java'
s = p.read_text(encoding='utf-8')
if 'System.getProperty("grovecraft.testWorld"' not in s:
    start = s.index('WORLD_NAME = ') + len('WORLD_NAME = ')
    end = s.index(';', start)
    s = s[:start] + 'System.getProperty("grovecraft.testWorld", ' + s[start:end] + ')' + s[end:]
    p.write_text(s, encoding='utf-8')
p = guest / 'build.gradle'
s = p.read_text(encoding='utf-8')
p.write_text(s.replace('-Dlibertycraft.quitWithHost=false', '-Dlibertycraft.quitWithHost=true'), encoding='utf-8')
# GTA owns CJ physics. The mirror server must not fight his position with void gravity/collision.
p = guest / 'src/main/java/dev/libertycraft/world/HostDrive.java'
s = p.read_text(encoding='utf-8')
if 'groveOriginalNoPhysics' not in s:
    s = s.replace('private static final class State {', 'private static final class State {\n\t\tboolean groveOriginalNoPhysics, groveOriginalNoGravity;')
    s = s.replace('st = new State();', 'st = new State();\n\t\t\tst.groveOriginalNoPhysics = player.noPhysics;\n\t\t\tst.groveOriginalNoGravity = player.isNoGravity();')
    s = s.replace('st.lastTick = server != null ? server.getTickCount() : 0;', 'st.lastTick = server != null ? server.getTickCount() : 0;\n\t\tplayer.noPhysics = true;\n\t\tplayer.setNoGravity(true);')
    marker = '\t\tif (type == MountType.NONE) {'
    assert marker in s
    s = s.replace(marker, marker + '\n\t\t\tplayer.setPos(x, y, z);\n\t\t\tplayer.setDeltaMovement(Vec3.ZERO);\n\t\t\tplayer.connection.resetPosition();')
    s = s.replace('boolean hadMount = st.mount != null;', 'player.noPhysics = st.groveOriginalNoPhysics;\n\t\tplayer.setNoGravity(st.groveOriginalNoGravity);\n\t\tboolean hadMount = st.mount != null;')
    marker = '\t\t\tServerPlayer player = server.getPlayerList().getPlayer(e.getKey());\n\t\t\tif (player != null && e.getValue().mount != null'
    assert marker in s
    s = s.replace(marker, '\t\t\tServerPlayer player = server.getPlayerList().getPlayer(e.getKey());\n\t\t\tif (player != null) { player.noPhysics = e.getValue().groveOriginalNoPhysics; player.setNoGravity(e.getValue().groveOriginalNoGravity); }\n\t\t\tif (player != null && e.getValue().mount != null')
    p.write_text(s, encoding='utf-8')
runpy.run_path(str(root / 'tools/sync-lifecycle.py'))
drive=guest/'src/main/java/dev/libertycraft/world/HostDrive.java'
s=drive.read_text(encoding='utf-8')
if 'if (player.isSleeping()) return; // Keep the real bed pose.' not in s:
    s=s.replace('st.lastTick = server != null ? server.getTickCount() : 0;','st.lastTick = server != null ? server.getTickCount() : 0;\n        if (player.isSleeping()) return; // Keep the real bed pose.')
drive.write_text(s,encoding='utf-8')
drive_client=guest/'src/client/java/dev/libertycraft/client/HostDriveClient.java'
s=drive_client.read_text(encoding='utf-8')
if 'if (player.isSleeping()) return; // Keep the real bed pose.' not in s:
    s=s.replace('private static void follow(LocalPlayer player, Link.SkyState sky) {','private static void follow(LocalPlayer player, Link.SkyState sky) {\n        if (player.isSleeping()) return; // Keep the real bed pose.')
drive_client.write_text(s,encoding='utf-8')

# Persistent coherent natural tree fall visuals and private accessors.
for name in ('GroveTreeFalls.java','GroveRuntimeAcceptance.java','GroveClientLease.java'):
    shutil.copyfile(root/'src'/name, guest/'src/main/java/dev/libertycraft'/name)
shutil.copyfile(root/'src/GroveClientLeaseTest.java',guest/'src/test/java/dev/libertycraft/GroveClientLeaseTest.java')
init=guest/'src/main/java/dev/libertycraft/LibertyCraft.java'
s=init.read_text()
if 'GroveRuntimeAcceptance.register();' not in s:
    init.write_text(s.replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveRuntimeAcceptance.register();'))
config=json.loads(mixins.read_text())
for name in ('GroveTreeDisplayMixin','GroveTreeTransformMixin','GroveFoodTickMixin','GroveMovementFoodMixin','GroveMarketProfessionMixin','GroveMarketOffersAccessor','GroveMerchantWorkMixin','GroveBedNightMixin','GroveDeathPickupMixin'):
    shutil.copyfile(root/'src'/(name+'.java'),guest/'src/main/java/dev/libertycraft/mixin'/(name+'.java'))
    if name not in config['mixins']:config['mixins'].append(name)
mixins.write_text(json.dumps(config,indent=2)+'\n')
# Match countryside look-ahead; entity cap remains 128, regardless of distance.
p=guest/'src/client/java/dev/libertycraft/client/render/AvatarExporter.java'
s=p.read_text(encoding='utf-8').replace('private static final double SCENE_RANGE = 96.0;','private static final double SCENE_RANGE = 160.0;')
p.write_text(s,encoding='utf-8')

runpy.run_path(str(root/'tools/patch-held-items.py'))

# Campaign-owned world selection and durable paired-save requests run even
# while GTA's own save/loading menu has paused the simulation.
shutil.copyfile(root/'src/GroveCampaignFiles.java',guest/'src/main/java/dev/libertycraft/GroveCampaignFiles.java')
shutil.copyfile(root/'src/GroveCampaignAcceptance.java',guest/'src/main/java/dev/libertycraft/GroveCampaignAcceptance.java')
shutil.copyfile(root/'src/GroveCampaignFilesTest.java',guest/'src/test/java/dev/libertycraft/GroveCampaignFilesTest.java')
shutil.copyfile(root/'src/GroveCampaignClient.java',guest/'src/client/java/dev/libertycraft/client/GroveCampaignClient.java')
p=guest/'src/main/java/dev/libertycraft/LibertyCraft.java'
s=p.read_text().replace('public static final String WORLD_NAME =','public static volatile String WORLD_NAME =')
if 'GroveCampaignAcceptance.register();' not in s:s=s.replace('GroveGameplay.register();','GroveGameplay.register();\n        GroveCampaignAcceptance.register();')
p.write_text(s)
p=guest/'src/client/java/dev/libertycraft/client/HostClient.java';s=p.read_text()
if 'GroveCampaignClient.frame(minecraft);' not in s:
    s=s.replace('MirrorWorld.openWhenReady(minecraft);','GroveCampaignClient.frame(minecraft);\n\t\tMirrorWorld.openWhenReady(minecraft);')
p.write_text(s)
p=guest/'src/client/java/dev/libertycraft/client/MirrorWorld.java';s=p.read_text()
if 'campaignReset()' not in s:
    s=s.replace('public static void openWhenReady(Minecraft minecraft) {','public static void campaignReset() { attempted=false; }\n\tpublic static void openWhenReady(Minecraft minecraft) {\n\t\tif (!GroveCampaignClient.mayOpen()) return;')
p.write_text(s)
p=guest/'src/main/java/dev/libertycraft/link/Link.java';s=p.read_text()
if '0x900L' not in s:
    s=s.replace('return (int) INT.getAcquire(s, 0x20L) != 0 || presented', 'return (int) INT.getAcquire(s, 0x900L) != 0 || (int) INT.getAcquire(s, 0x20L) != 0 || presented')
p.write_text(s)

# Context hints and tracked village deliveries, registered once.
shutil.copyfile(root/'src/GroveInteractionHints.java',guest/'src/main/java/dev/libertycraft/GroveInteractionHints.java')
shutil.copyfile(root/'src/GroveRoadside.java',guest/'src/main/java/dev/libertycraft/GroveRoadside.java')
p=guest/'src/main/java/dev/libertycraft/LibertyCraft.java';s=p.read_text(encoding='utf-8')
if 'GroveRoadside.register();' not in s:
    s=s.replace('GroveCargo.register();','GroveCargo.register();\n        GroveRoadside.register();')
if 'GroveInteractionHints.register();' not in s:
    s=s.replace('GroveCargo.register();','GroveCargo.register();\n        GroveInteractionHints.register();')
p.write_text(s,encoding='utf-8')

shutil.copyfile(root/'src/GroveHudAcceptance.java',guest/'src/main/java/dev/libertycraft/GroveHudAcceptance.java')
p=guest/'src/main/java/dev/libertycraft/LibertyCraft.java';s=p.read_text(encoding='utf-8')
if 'GroveHudAcceptance.register();' not in s:
    p.write_text(s.replace('GroveCargo.register();','GroveCargo.register();\n        GroveHudAcceptance.register();'),encoding='utf-8')

shutil.copyfile(root/"src/GroveWorldPlanTest.java",guest/"src/test/java/dev/libertycraft/GroveWorldPlanTest.java")

shutil.copyfile(root/'src/GroveRecipeBookMixin.java',guest/'src/client/java/dev/libertycraft/client/mixin/GroveRecipeBookMixin.java')
p=guest/'src/main/resources/libertycraft.client.mixins.json'
if not p.exists():p=guest/'src/client/resources/libertycraft.client.mixins.json'
if not p.exists():p=guest/'src/main/resources/libertycraft.mixins.json'
config=json.loads(p.read_text(encoding='utf-8'))
key='client' if 'client' in config else 'mixins'
if 'GroveRecipeBookMixin' not in config[key]:config[key].append('GroveRecipeBookMixin')
p.write_text(json.dumps(config,indent=2),encoding='utf-8')

shutil.copyfile(root/"src/GroveOreRules.java",guest/"src/main/java/dev/libertycraft/GroveOreRules.java")
shutil.copyfile(root/"src/GroveOreRulesTest.java",guest/"src/test/java/dev/libertycraft/GroveOreRulesTest.java")

p=guest/'src/client/java/dev/libertycraft/client/render/WorldExporter.java'
s=p.read_text(encoding='utf-8')
line='if(cargoBox!=null)ENTITIES.add(new Link.WorldEntity(104,2,0,0,0,0,0,.32f,null,cargoBox,0));'
replacement='if(Boolean.getBoolean("grovecraft.cargoMaterials")){\n        int cargoSlot=0;\n        for(var cargoStack:dev.libertycraft.GroveCargo.visualItems(minecraft.player)){\n            int before=ENTITIES.size();addItem(minecraft,level,minecraft.player,cargoStack,Vec3.ZERO,0);\n            if(ENTITIES.size()>before){var display=ENTITIES.removeLast();\n                ENTITIES.add(new Link.WorldEntity(display.kind()==Proto.WE_BLOCK?104:105,3+cargoSlot++,0,0,0,0,0,\n                    display.kind()==Proto.WE_BLOCK?.22f:.23f,null,display.uv(),display.tint()));}\n        }\n        }else '+line
if 'grovecraft.cargoMaterials' not in s:
    legacyMaterials=replacement.split('grovecraft.cargoMaterials")){\n        ',1)[1].rsplit('\n        }else ',1)[0]
    s=s.replace(legacyMaterials,replacement) if legacyMaterials in s else s.replace(line,replacement)
p.write_text(s,encoding='utf-8')

shutil.copyfile(root/"src/GroveSurfaceAtlas.java",guest/"src/main/java/dev/libertycraft/GroveSurfaceAtlas.java")

shutil.copyfile(root/"src/GroveSurfaceAtlasTest.java",guest/"src/test/java/dev/libertycraft/GroveSurfaceAtlasTest.java")

shutil.copyfile(root/"src/GroveStaticGround.java",guest/"src/main/java/dev/libertycraft/GroveStaticGround.java")
p=guest/'src/main/java/dev/libertycraft/mixin/BlockCollisionsMixin.java'
s=p.read_text(encoding='utf-8')
if 'GroveStaticGround.shape' not in s:
    s=s.replace('VoxelShape sky = HostCollision.shapeAt(pos);','VoxelShape sky = HostCollision.shapeAt(pos);\n        if(level instanceof net.minecraft.world.level.Level actual&&dev.libertycraft.world.city.BlockyCity.isMirror(actual)){\n            var atlasSky=new dev.libertycraft.link.Link.SkyState();\n            if(dev.libertycraft.link.Link.readSkyState(atlasSky)&&dev.libertycraft.world.HostDig.isDug(actual,atlasSky.worldId,pos))sky=null;\n            else if(sky==null)sky=dev.libertycraft.GroveStaticGround.shape(actual,pos);\n        }')
p.write_text(s,encoding='utf-8')
p=guest/'src/main/java/dev/libertycraft/world/MobHold.java'
s=p.read_text(encoding='utf-8').replace('if (!HostCollision.active()) return true;', 'if (!HostCollision.active()&&!dev.libertycraft.GroveStaticGround.available()) return true;')
s=s.replace('if (!groundUnknown(level, mob.blockPosition())) {','if (dev.libertycraft.GroveStaticGround.supported(level,mob.blockPosition()) || !groundUnknown(level, mob.blockPosition())) {')
p.write_text(s,encoding='utf-8')
p=guest/'src/main/java/dev/libertycraft/world/HostPath.java'
s=p.read_text(encoding='utf-8')
s=s.replace('&& HostCollision.active();','&& (HostCollision.active()||dev.libertycraft.GroveStaticGround.available());')
s=s.replace('return HostCollision.hasGeometry(pos) && HostCollision.groundTop(pos) >= SOLID_FROM;', 'return dev.libertycraft.GroveStaticGround.top(pos)>=SOLID_FROM || HostCollision.hasGeometry(pos) && HostCollision.groundTop(pos) >= SOLID_FROM;')
s=s.replace('return !solid(pos) && HostCollision.supportsFromBelow(pos);','return !solid(pos) && (HostCollision.supportsFromBelow(pos)||dev.libertycraft.GroveStaticGround.top(pos)>0||dev.libertycraft.GroveStaticGround.top(pos.below())>=SOLID_FROM);')
p.write_text(s,encoding='utf-8')

p=guest/'src/main/java/dev/libertycraft/world/HostPath.java'
s=p.read_text(encoding='utf-8')
if 'if(dev.libertycraft.GroveStaticGround.dug(pos))return false;' not in s:
    s=s.replace('public static boolean solid(BlockPos pos) {','public static boolean solid(BlockPos pos) {\n        if(dev.libertycraft.GroveStaticGround.dug(pos))return false;')
    s=s.replace('HostCollision.supportsFromBelow(pos)||', '(!dev.libertycraft.GroveStaticGround.dug(pos)&&!dev.libertycraft.GroveStaticGround.dug(pos.below())&&HostCollision.supportsFromBelow(pos))||')
p.write_text(s,encoding='utf-8')

p=guest/'src/main/java/dev/libertycraft/mixin/BlockCollisionsMixin.java'
s=p.read_text(encoding='utf-8').replace('var state=new dev.libertycraft.link.Link.SkyState();','var atlasSky=new dev.libertycraft.link.Link.SkyState();').replace('Link.readSkyState(state)&&dev.libertycraft.world.HostDig.isDug(actual,state.worldId,pos)','Link.readSkyState(atlasSky)&&dev.libertycraft.world.HostDig.isDug(actual,atlasSky.worldId,pos)')
p.write_text(s,encoding='utf-8')
runpy.run_path(str(root/'tools/sync-foliage-lod.py'))

# An installed release uses the official authenticated Fabric launcher, with
# shared local settings instead of a developer shell's environment.
p=guest/'src/main/java/dev/libertycraft/LibertyCraft.java'
s=p.read_text(encoding='utf-8')
if '// GroveCraft release survival defaults' not in s:
    s=s.replace('public void onInitialize() {','''public void onInitialize() {
        // GroveCraft release survival defaults
        if (GroveReleasePaths.installedHome()!=null && System.getProperty("grovecraft.testWorld")==null
            && System.getProperty("grovecraft.expedition")==null) System.setProperty("grovecraft.expedition","true");''',1)
p.write_text(s,encoding='utf-8')
p=guest/'src/main/resources/fabric.mod.json';metadata=json.loads(p.read_text(encoding='utf-8'))
if 'Axel12Q / GroveCraft contributors' not in metadata['authors']:metadata['authors'].append('Axel12Q / GroveCraft contributors')
if 'Codex (GPT-6), development assistance' not in metadata['contributors']:metadata['contributors'].append('Codex (GPT-6), development assistance')
p.write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
# Resolve a fixed implementation module, not a moving plugin snapshot marker.
p=guest/'gradle.properties';s=p.read_text().replace('loom_version=1.18-SNAPSHOT','loom_version=1.18.4');p.write_text(s)
p=guest/'settings.gradle';s=p.read_text()
if '// GroveCraft pinned Loom module' not in s:
    s=s.replace('pluginManagement {','''pluginManagement {
    // GroveCraft pinned Loom module
    resolutionStrategy {
        eachPlugin {
            if (requested.id.id == 'net.fabricmc.fabric-loom') useModule('net.fabricmc:fabric-loom:1.18.4')
        }
    }''',1)
p.write_text(s)

# Earlier incremental patches can run before GroveGameplay exists on a fresh
# checkout. Normalize our bootstrap at the end, preserving the verified event
# registration order rather than relying on how often sync has been run.
p=guest/'src/main/java/dev/libertycraft/LibertyCraft.java'
s=p.read_text(encoding='utf-8')
bootstrap=[
    'GroveProbe','GroveBeds','GroveTownEconomy','GroveGameplay',
    'GroveWorkshopAcceptance','GroveLightingAcceptance','GroveCampaignAcceptance',
    'GroveTownFieldAcceptance','GroveCombatFieldAcceptance','GroveBedAcceptance',
    'GroveMarketWorkAcceptance','GroveVillageEconomy','GroveCoexistenceAcceptance',
    'GroveRuntimeAcceptance','GroveEconomyAcceptance','GroveNativeTrees',
    'GroveNativeFellingTest','GroveSurvival','GroveBugAcceptance','GroveArmorState',
    'GroveArmorAcceptance','GroveEcology','GroveCargo','GroveRoadside',
    'GroveHudAcceptance','GroveInteractionHints','GroveCargoAcceptance',
    'GroveFieldGuide','GroveEcologyAcceptance','GroveInteractionAcceptance','GroveAcceptance',
]
begin=s.index('GroveProbe.register();')
end=s.index('HostCombat.init();',begin)
s=s[:begin]+('\n        '.join(name+'.register();' for name in bootstrap))+'\n        '+s[end:]
p.write_text(s,encoding='utf-8')
config=json.loads(client_mixins.read_text(encoding='utf-8'))
owned=['GroveContainerMixin','GroveBookMixin','GroveInventoryMixin','GroveTownMenuMixin','GroveRecipeBookMixin']
config['client']=[name for name in config['client'] if name not in owned]+owned
client_mixins.write_text(json.dumps(config,indent=2)+'\n',encoding='utf-8')
