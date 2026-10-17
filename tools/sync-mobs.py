"""Reproducible SA spawner adaptation of LibertyCraft's MIT source; no world writes."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = root / 'vendor/libertycraft/fabric/src/main/java/dev/libertycraft/world/MobSpawner.java'
s = source.read_text(encoding='utf-8')
s = s.replace('private static final String FILE = "libertycraft_mobs.properties";',
              'private static final String FILE = "grovecraft_mobs.properties";')
s = s.replace('p.getProperty("spawning", "false")',
              'p.getProperty("spawning", Boolean.toString(Boolean.getBoolean("grovecraft.dynamicMobs")))')
# Fewer encounters in a populated city; properties remain launcher-configurable.
s = s.replace('private static final int EVERY = 20;',
              'private static final int EVERY = 20;')
s = s.replace('SpawnRules.HOSTILES, SpawnRules.HOSTILE_CAP, true',
              'CITY_HOSTILES, Math.max(1, Math.min(24, Integer.getInteger("grovecraft.hostileCap", 12))), true')
s = s.replace('private static final double CAP_RANGE = 128.0;', '''private static final double CAP_RANGE = 128.0;
    private static final List<SpawnRules.Kind> CITY_HOSTILES = List.of(
        new SpawnRules.Kind("minecraft:zombie",95,1,2),
        new SpawnRules.Kind("minecraft:skeleton",60,1,2),
        new SpawnRules.Kind("minecraft:spider",50,1,2),
        new SpawnRules.Kind("minecraft:creeper",Math.max(0, Integer.getInteger("grovecraft.creeperWeight",15)),1,1),
        new SpawnRules.Kind("minecraft:enderman",5,1,1),
        new SpawnRules.Kind("minecraft:witch",2,1,1));''')
s = s.replace('!SKY.inGame() || SKY.loading()',
              '!SKY.inGame() || SKY.loading() || SKY.menuOpen() || SKY.worldId != 0 || (SKY.flags & ((1 << 19) | (1 << 21))) != 0')
s = s.replace('if (night) {', 'if (night && level.getDifficulty() != net.minecraft.world.Difficulty.PEACEFUL) {\n            if (server.getTickCount() % Math.max(20, Integer.getInteger("grovecraft.mobInterval",80)) != 0) return;')
s = s.replace('py.isPresent() && farFromPlayers(level, px + 0.5, pz + 0.5) && spawn(',
              'py.isPresent() && farFromPlayers(level, px + 0.5, pz + 0.5)\n'
              '\t\t\t\t\t&& (hostile || onGrass(px + 0.5, py.getAsDouble(), pz + 0.5)) && spawn(')
marker = '\t\tif (!level.noCollision(type.getSpawnAABB(x, y, z))) {'
assert marker in s
s = s.replace('private static int spawned;', 'private static int spawned, lightRejected;')
s = s.replace(marker, '''
        BlockPos feet = BlockPos.containing(x, y, z);
        boolean hostile = type.getCategory() == net.minecraft.world.entity.MobCategory.MONSTER;
        // Modern vanilla monsters require zero emitted block light. GTA's time is
        // authoritative; lamps represented as Minecraft lights keep streets safe.
        int blockLight = level.getBrightness(net.minecraft.world.level.LightLayer.BLOCK, feet);
        if (hostile && blockLight > 0) {
            if (lightRejected++ < 8) LibertyCraft.LOG.info("[GroveCraft] hostile spawn blocked by Minecraft block light {} at {}", blockLight, feet);
            return false;
        }
        if (hostile && (!SpawnRules.isNight(SKY.gameHour)
                || level.getDifficulty() == net.minecraft.world.Difficulty.PEACEFUL)) return false;
''' + marker)
# Mountain pasture uses the observed native surface, not the old +/-4m street band.
s = s.replace('private static final int ANIMALS_EVERY = 400;', 'private static final int ANIMALS_EVERY = 40;')
s = s.replace('private static final int TRIES = 6;', 'private static final int TRIES = 16;')
s=s.replace('private static int spawned, lightRejected;', '''private static int spawned, lightRejected;
    private static double lastX,lastZ,leadX,leadZ;
    private static int lastTick;''')
marker='ServerPlayer player = players.getFirst();'
assert marker in s
s=s.replace(marker,marker+'''
        int sampleTick=server.getTickCount();
        leadX=leadZ=0;
        if(lastTick>0 && sampleTick>lastTick && (SKY.flags & (1<<4))!=0) {
            double scale=40.0/(sampleTick-lastTick); // two seconds of observed travel
            double dx=(player.getX()-lastX)*scale,dz=(player.getZ()-lastZ)*scale;
            double speed=Math.hypot(dx,dz);
            if(speed<240) {double scaleAhead=Math.min(1,96/Math.max(1,speed));leadX=dx*scaleAhead;leadZ=dz*scaleAhead;}
        }
        lastTick=sampleTick;lastX=player.getX();lastZ=player.getZ();''')
s=s.replace('player.getX() + Math.cos(angle) * dist','player.getX() + leadX + Math.cos(angle) * dist')
s=s.replace('player.getZ() + Math.sin(angle) * dist','player.getZ() + leadZ + Math.sin(angle) * dist')
s = s.replace('SpawnRules.surface(GROUND, x, z, player.getY(), type.getHeight())',
    '(hostile ? SpawnRules.surface(GROUND, x, z, player.getY(), type.getHeight()) : dev.libertycraft.GroveEcology.wildlifeHeight(x,z,player.getY()))')
s = s.replace('SpawnRules.surface(GROUND, px, pz, player.getY(), type.getHeight())',
    '(hostile ? SpawnRules.surface(GROUND, px, pz, player.getY(), type.getHeight()) : dev.libertycraft.GroveEcology.wildlifeHeight(px,pz,player.getY()))')
s = s.replace('t.walkable && t.gtaMaterial >= 0 && SpawnRules.isGrass(CityCells.materialName(t.gtaMaterial))',
    't.terrain && (t.gtaMaterial == 37 || t.material == dev.libertycraft.link.Proto.DIG_DIRT) && Math.abs(t.ny) > .65')
s = s.replace('y - 0.6, z - 0.3', 'y - 1.4, z - 0.3')
s = s.replace("GTA IV", "San Andreas").replace('LibertyCraft mob spawning', 'GroveCraft mob spawning')
s = '// GroveCraft adaptation of LibertyCraft (MIT); see vendor/libertycraft/LICENSE.\n' + s
target = root / 'build/guest/src/main/java/dev/libertycraft/world/MobSpawner.java'
target.write_text(s, encoding='utf-8')
print('Dynamic GroveCraft mobs: SA terrain, vanilla block light, caps and campaign/interior gates.')

# Reloaded animals must not fall before the first native collision region arrives.
hold = (source.parent / 'MobHold.java').read_text(encoding='utf-8')
hold = hold.replace('if (!HostPath.applies(level) ||',
                    'if (!dev.libertycraft.world.city.BlockyCity.isMirror(level) || !dev.libertycraft.link.Link.active() || level.isClientSide() ||')
hold = hold.replace('if (mob.onGround()) {',
                    'if (!HostCollision.active()) return true;\n\t\tif (mob.onGround()) {')
(target.parent / 'MobHold.java').write_text(hold, encoding='utf-8')

# Refresh from pinned source every time: use swept search bounds, not just the
# car's final position (a fast frame can pass completely beyond a cow).
combat=source.parents[1] / 'combat'
runover=(combat/'VehicleRunOver.java').read_text(encoding='utf-8')
runover=runover.replace('dt < 0.3','dt < 1.0')
runover=runover.replace('new AABB(now.x() - reach, now.bottom() - 1.0, now.z() - reach, now.x() + reach, now.top() + 1.0, now.z() + reach)',
    'new AABB(Math.min(now.x(),before.x()) - reach, Math.min(now.bottom(),before.bottom()) - 1.0, Math.min(now.z(),before.z()) - reach, Math.max(now.x(),before.x()) + reach, Math.max(now.top(),before.top()) + 1.0, Math.max(now.z(),before.z()) + reach)')
(target.parents[1]/'combat/VehicleRunOver.java').write_text(runover,encoding='utf-8')

# Search bounds alone cannot detect a car that already passed the animal.
# Sample the swept body at <=0.5m intervals, bounded by the teleport speed cap.
math_source=(combat/'RunOver.java').read_text(encoding='utf-8')
signature='public static Contact contact(Pose now, Pose before, double dt, double mx, double mz, double px, double pz, double r, double minY, double maxY) {'
assert signature in math_source
math_source=math_source.replace(signature,signature+'''
        Contact finalContact=instantContact(now,before,dt,mx,mz,px,pz,r,minY,maxY);
        if(finalContact.touching() || !(dt>1.0E-3 && dt<1)) return finalContact;
        double distance=Math.hypot(now.x()-before.x(),now.z()-before.z());
        if(!Double.isFinite(distance) || distance/dt>MAX_SPEED) return finalContact;
        int steps=Math.min(140,Math.max(1,(int)Math.ceil(distance/.5)));
        double turn=Math.IEEEremainder(now.yaw()-before.yaw(),360);
        for(int i=1;i<steps;i++) {
            double t=(double)i/steps;
            Pose sample=new Pose(before.x()+(now.x()-before.x())*t,before.z()+(now.z()-before.z())*t,
                before.bottom()+(now.bottom()-before.bottom())*t,before.top()+(now.top()-before.top())*t,
                before.yaw()+turn*t,now.halfLength(),now.halfWidth());
            Contact c=instantContact(sample,before,dt*t,px+(mx-px)*t,pz+(mz-pz)*t,px,pz,r,minY,maxY);
            if(c.hit()) return c;
        }
        return finalContact;
    }
    private static Contact instantContact(Pose now, Pose before, double dt, double mx, double mz, double px, double pz, double r, double minY, double maxY) {''')
(target.parents[1]/'combat/RunOver.java').write_text(math_source,encoding='utf-8')

# Melee on large native stand-ins must actually reach their body, not only
# satisfy vanilla's width-expanded target radius. Projectile hits remain vanilla.
p=target.parents[1]/'combat/HostActorEntity.java'
s=(combat/'HostActorEntity.java').read_text(encoding='utf-8')
s=s.replace('public boolean hurtServer(ServerLevel level, DamageSource source, float amount) {', """public boolean hurtServer(ServerLevel level, DamageSource source, float amount) {
        if (source.getEntity() instanceof net.minecraft.world.entity.Mob mob
            && !(source.getDirectEntity() instanceof net.minecraft.world.entity.projectile.Projectile)) {
            var a=mob.getBoundingBox();var b=this.getBoundingBox();
            double dx=Math.max(0,Math.max(a.minX-b.maxX,b.minX-a.maxX));
            double dz=Math.max(0,Math.max(a.minZ-b.maxZ,b.minZ-a.maxZ));
            double dy=Math.max(0,Math.max(a.minY-b.maxY,b.minY-a.maxY));
            if(dx*dx+dz*dz>.75*.75 || dy>.5)return false;
        }""")
p.write_text(s,encoding='utf-8')

# Passive herds are discovered at visible distance. Hostile safety/light rules
# keep their existing range; a forward herd counts against the entire local cap.
p=target
s=p.read_text(encoding='utf-8')
s=s.replace('AABB around = player.getBoundingBox().inflate(CAP_RANGE);',
    'AABB around = player.getBoundingBox().inflate(hostile ? CAP_RANGE : 160);')
s=s.replace('double dist = SpawnRules.MIN_DISTANCE + random.nextDouble() * (SpawnRules.MAX_DISTANCE - SpawnRules.MIN_DISTANCE);',
    'double dist = hostile ? SpawnRules.MIN_DISTANCE + random.nextDouble() * (SpawnRules.MAX_DISTANCE - SpawnRules.MIN_DISTANCE) : 48 + random.nextDouble()*80;')
needle='int x = (int) Math.floor(player.getX() + leadX + Math.cos(angle) * dist), z = (int) Math.floor(player.getZ() + leadZ + Math.sin(angle) * dist);'
if 'groveVisibleHerd' not in s:
    s=s.replace(needle,needle+'\n            double groveVisibleHerd = Math.hypot(x-player.getX(),z-player.getZ());\n            if (!hostile && (groveVisibleHerd < 48 || groveVisibleHerd > 158)) continue;')
p.write_text(s,encoding='utf-8')

ped=(combat/'PedTargets.java').read_text(encoding='utf-8')
ped=ped.replace('selector.addGoal(w.getPriority(), new PedGoal', 'selector.addGoal(Math.max(0,w.getPriority()-1), new PedGoal')
ped=ped.replace('if (player != null && this.mob.distanceToSqr(player) <= this.mob.distanceToSqr(ped)) {',
    'if (player != null && (ped.hostFlags() & dev.libertycraft.link.Proto.ACTOR_PLAYER_VEHICLE) == 0 && this.mob.distanceToSqr(player) <= this.mob.distanceToSqr(ped)) {')
(target.parents[1]/'combat/PedTargets.java').write_text(ped,encoding='utf-8')

clock=(source.parent/'HostSky.java').read_text(encoding='utf-8')
clock=clock.replace('Math.max(0L, now + d)','dev.libertycraft.GroveClockRules.correctedTicks(now,d)')
(target.parent/'HostSky.java').write_text(clock,encoding='utf-8')

# Occupied CJ cars also need a body target: chasing the seated player's centre
# never reaches the exterior of a long car. Run-over already owns their pushing.
p=target.parents[1]/'combat/HostCombat.java'
s=(combat/'HostCombat.java').read_text(encoding='utf-8')
s=s.replace('if ((a.flags() & Proto.ACTOR_PLAYER_VEHICLE) == 0) {', 'if (true) { // GroveCraft: player car exterior is a valid mob target')
s=s.replace('ProxyPush.shoveMobs(level, proxy, a.x() - ox, a.z() - oz);',
    'if ((a.flags() & Proto.ACTOR_PLAYER_VEHICLE) == 0) ProxyPush.shoveMobs(level, proxy, a.x() - ox, a.z() - oz);')
p.write_text(s,encoding='utf-8')
