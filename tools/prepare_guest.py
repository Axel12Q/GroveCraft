"""Generate a local, attributed LibertyCraft guest fork from the pinned checkout."""
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
vendor = root/'vendor/libertycraft'
expected = '0fe0f2c9aae2ba974cb077e282b091a38f1ca48f'
actual = subprocess.check_output(['git','-C',str(vendor),'rev-parse','HEAD'],text=True).strip()
assert actual == expected, f'Unexpected upstream commit: {actual}'
target = root/'build/guest'
if target.exists():
    raise SystemExit('Generated guest already exists. Preserve edits; do not overwrite.')
shutil.copytree(vendor/'fabric',target)
shutil.copyfile(vendor/'LICENSE',target.parent/'LICENSE')
def replace(path, old, new):
    p = target/path
    s = p.read_text(encoding='utf-8')
    assert old in s, (path,old)
    p.write_text(s.replace(old,new),encoding='utf-8')
# Keep Java namespaces/mod ID so upstream mixins and registry IDs remain valid.
# Isolate transport and world from an actual LibertyCraft installation.
replace('src/main/java/dev/libertycraft/link/Proto.java','0x5954424C','0x564F5247')
replace('src/main/java/dev/libertycraft/link/Proto.java','LibertyCraft_v1','GroveCraft_v1')
replace('src/main/java/dev/libertycraft/LibertyCraft.java','WORLD_NAME = "LibertyCraft"','WORLD_NAME = "GroveCraftLab"')
replace('build.gradle','runtimeOnly "maven.modrinth:e4mc:AouleFRY"','// GroveCraft lab is local single-player; no e4mc dependency.')
replace('gradle.properties','archives_base_name=libertycraft','archives_base_name=grovecraft-guest')
replace('gradle.properties','org.gradle.jvmargs=-Xmx4G','org.gradle.jvmargs=-Xmx2G')
replace('src/main/resources/fabric.mod.json','"name": "LibertyCraft"','"name": "GroveCraft guest (LibertyCraft fork)"')
replace('src/main/resources/fabric.mod.json','Play GTA IV with Minecraft physics, inventory and viewmodels. A port of SkyCraft.','Experimental San Andreas bridge guest, derived from LibertyCraft and SkyCraft. Host integration in progress.')
replace('src/main/java/dev/libertycraft/LibertyCraft.java','public void onInitialize() {','public void onInitialize() {\n\t\tGroveProbe.register();')
shutil.copyfile(root/'src/GroveProbe.java',target/'src/main/java/dev/libertycraft/GroveProbe.java')
shutil.copyfile(root/'src/GroveWindowsLinkTest.java',target/'src/test/java/dev/libertycraft/link/GroveWindowsLinkTest.java')
print(target)
subprocess.run([__import__('sys').executable, str(root/'tools/sync-guest.py')], check=True)
