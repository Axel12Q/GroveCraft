package dev.libertycraft;

import com.mojang.serialization.Codec;
import dev.libertycraft.link.Link;
import dev.libertycraft.world.*;
import dev.libertycraft.world.city.BlockyCity;
import java.nio.file.*;
import java.util.*;
import net.fabricmc.fabric.api.attachment.v1.*;
import net.fabricmc.fabric.api.command.v2.CommandRegistrationCallback;
import net.fabricmc.fabric.api.event.lifecycle.v1.*;
import net.minecraft.commands.Commands;
import net.minecraft.core.*;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.network.chat.Component;
import net.minecraft.resources.Identifier;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.level.*;
import net.minecraft.server.permissions.Permissions;
import net.minecraft.world.entity.*;
import net.minecraft.world.entity.animal.Animal;
import net.minecraft.world.entity.npc.villager.Villager;
import net.minecraft.world.item.*;
import net.minecraft.world.level.block.*;
import net.minecraft.world.level.block.entity.*;
import net.minecraft.world.level.block.state.*;
import net.minecraft.world.level.block.state.properties.*;
import net.minecraft.world.level.chunk.LevelChunk;
import net.minecraft.world.level.storage.LevelResource;
import net.minecraft.world.phys.AABB;

/** Small, persistent countryside additions, using the same native geometry mobs stand on.
 * No GTA asset edits, map replacement, forced chunks, or
 * replenishing harvested features. The ledger is saved in the anchor's Minecraft chunk. */
public final class GroveEcology {
    private GroveEcology() {}
    private static final AttachmentType<List<String>> LEDGER = AttachmentRegistry.<List<String>>builder()
        .persistent(Codec.STRING.listOf()).buildAndRegister(Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"ecology_v1"));
    private static final Link.SkyState SKY = new Link.SkyState();
    private static final String CONFIG="grovecraft_ecology.properties";
    private static boolean enabled=true;
    private static int cursor, generated;
    private static GroveWorldPlan worldPlan;
    private static GroveStructureMap structureMap=new GroveStructureMap();
    private static final AttachmentType<List<String>> MAP_LEDGER=AttachmentRegistry.<List<String>>builder()
        .persistent(Codec.STRING.listOf()).buildAndRegister(Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"structure_map_v1"));
    private static List<GroveEcologyRules.Site> structureOwners=List.of();
    private static int mapSequence=new Random().nextInt(Integer.MAX_VALUE-1)+1;
    private static int mapPublishTick;
    private static final Map<String,Long> mapRetry=new HashMap<>();
    private static volatile GroveSurfaceAtlas surfaceAtlas;
    private static volatile ServerLevel atlasLevel;
    public static GroveSurfaceAtlas surfaceAtlas(){return surfaceAtlas;}
    public static ServerLevel atlasLevel(){return atlasLevel;}
    private static List<GroveEcologyRules.Site> plannedCandidates=List.of();
    private static int plannedX=Integer.MIN_VALUE,plannedZ=Integer.MIN_VALUE;
    private static double lastX,lastZ,leadX,leadZ;
    private static int lastSurveyTick;
    private static final Map<String,Integer> retry = new LinkedHashMap<>();
    private static final Map<String,Integer> searches = new LinkedHashMap<>();
    private static final Map<String,String> expansions = new LinkedHashMap<>();
    private static String last="Sin explorar";
    private static String attempt="null";
    /** Only recorded natural trees participate; player buildings and farms never do. */
    public static int toppleNaturalTree(ServerPlayer player,BlockPos root,BlockState broken) {
        if(!broken.is(net.minecraft.tags.BlockTags.LOGS)||!Link.readSkyState(SKY)
            ||!GroveEcologyRules.safe(SKY.flags,SKY.worldId,SKY.inGame(),SKY.loading()))return 0;
        var level=player.level();
        var sites=List.of(GroveEcologyRules.woodlandSite(level.getSeed(),Math.floorDiv(root.getX(),16),Math.floorDiv(root.getZ(),16)),
            GroveEcologyRules.site(level.getSeed(),Math.floorDiv(root.getX(),32),Math.floorDiv(root.getZ(),32),true));
        for(var site:sites) {
            if(site.x()!=root.getX()||site.z()!=root.getZ()||!level.hasChunkAt(root))continue;
            var chunk=level.getChunkAt(root);var ledger=chunk.getAttached(LEDGER);
            String fallen="felled:"+site.key();
            if(ledger==null||!ledger.contains(site.key())||ledger.contains(fallen)
                ||level.getBlockState(root.below()).is(net.minecraft.tags.BlockTags.LOGS))continue;
            var cells=GroveEcologyRules.plan(site);
            // A middle trunk segment is ordinary mining, not a base cut.
            if(!level.getBlockState(root.above(4)).is(net.minecraft.tags.BlockTags.LOGS))return 0;
            for(var cell:cells)if(!level.hasChunkAt(root.offset(cell.x(),cell.y(),cell.z())))return 0;
            double dx=root.getX()+.5-player.getX(),dz=root.getZ()+.5-player.getZ(),length=Math.hypot(dx,dz);
            if(length<.01){dx=1;dz=0;length=1;}
            List<net.minecraft.world.entity.Display.BlockDisplay> displays=new ArrayList<>();
            List<BlockPos> removed=new ArrayList<>();
            // Prepare every visual before touching the tree: cancellation must not consume resources.
            for(var cell:cells){
                var pos=root.offset(cell.x(),cell.y(),cell.z());var state=level.getBlockState(pos);
                if(pos.equals(root)&&state.isAir())state=broken; // legacy acceptance call after removing root
                var expected=BuiltInRegistries.BLOCK.getValue(Identifier.withDefaultNamespace(cell.block()));
                if(state.getBlock()!=expected)continue;
                var display=GroveTreeFalls.prepare(level,root,cell.x(),cell.y(),cell.z(),state,dx/length,dz/length,!player.isCreative());
                if(display==null)return 0;displays.add(display);removed.add(pos);
            }
            for(var display:displays)if(!level.addFreshEntity(display)){displays.forEach(Entity::discard);return 0;}
            var next=new ArrayList<>(ledger);next.add(fallen);chunk.setAttached(LEDGER,List.copyOf(next));
            int count=0;
            for(var pos:removed){level.setBlock(pos,Blocks.AIR.defaultBlockState(),18);count++;}
            if(!player.isCreative()){player.causeFoodExhaustion(.005f);player.getMainHandItem().hurtAndBreak(1,player,EquipmentSlot.MAINHAND);}
            LibertyCraft.LOG.info("[GroveCraft] natural tree base cut {}: {} falling blocks",root,count);
            return count;
        }
        return 0;
    }
    public interface Terrain {
        double grass(int x,int z,double nearY);
        default double settlement(int x,int z,double nearY){return grass(x,z,nearY);}
        boolean known(int x,int y,int z);
        boolean occupied(BlockPos pos);
    }
    private static final Terrain NATIVE = new Terrain() {
        public double grass(int x,int z,double nearY) {
            double cached=surfaceAtlas==null?Double.NaN:surfaceAtlas.ground(x,z);
            return Double.isFinite(cached)?cached:surface(x,z,nearY,false);
        }
        public double settlement(int x,int z,double nearY) {
            double cached=surfaceAtlas==null?Double.NaN:surfaceAtlas.ground(x,z);
            return Double.isFinite(cached)?cached:surface(x,z,nearY,true);
        }
        private double surface(int x,int z,double nearY,boolean settlement) {
            List<HostTri> tris=new ArrayList<>();
            HostCollision.trianglesNear(new AABB(x,nearY-24,z,x+1,nearY+24,z+1),tris);
            double highest=-Double.MAX_VALUE, grass=Double.NaN;
            for(HostTri t:tris) {
                double y=t.heightAt(x+.5,z+.5);
                if(!Double.isFinite(y))continue;
                highest=Math.max(highest,y);
                // Grassy/dirt hills are valid roots; buildings still require a nearly level plot.
                if(t.terrain && (t.gtaMaterial==37 || t.material==dev.libertycraft.link.Proto.DIG_DIRT
                        ||settlement&&t.material==dev.libertycraft.link.Proto.DIG_SAND) && Math.abs(t.ny)>.65
                        && (!Double.isFinite(grass)||y>grass))grass=y;
            }
            // A roof, tree crown or road above grass is not an empty plot.
            double water=HostWater.surfaceAt(x,z);
            if(Double.isFinite(water)&&water>grass+.1)return Double.NaN;
            // SA's upward/downward casts can label the exact same plane grass and gravel.
            // Prefer explicit grass only at the top plane (2mm tolerance), never below a roof.
            return GroveEcologyRules.grassIsTop(grass,highest) ? grass : Double.NaN;
        }
        public boolean known(int x,int y,int z) { return surfaceAtlas!=null&&surfaceAtlas.known(x,z)||HostCollision.isKnown(x,y,z); }
        public boolean occupied(BlockPos p) { return surfaceAtlas!=null&&surfaceAtlas.known(p.getX(),p.getZ())?surfaceAtlas.occupied(p.getX(),p.getY(),p.getZ()):HostCollision.hasGeometry(p); }
    };
    public static OptionalDouble wildlifeHeight(int x,int z,double nearY) {
        if(!GroveEcologyRules.woodland(x,z,2))return OptionalDouble.empty();
        double y=NATIVE.grass(x,z,nearY);
        if(!Double.isFinite(y))return OptionalDouble.empty();
        // Native voxels round the supporting surface up to eighths of a block.
        // Spawn above that enclosure, then let normal gravity settle the animal.
        double stand=Math.ceil(y)+.05;
        if(!HostCollision.isKnown(x,(int)stand+2,z))return OptionalDouble.empty();
        return OptionalDouble.of(stand);
    }
    public static void register() {
        GroveTreeFalls.register();
        net.fabricmc.fabric.api.event.player.PlayerBlockBreakEvents.BEFORE.register((world,player,pos,state,blockEntity)->{
            return !(player instanceof ServerPlayer serverPlayer&&toppleNaturalTree(serverPlayer,pos,state)>0);
        });
        ServerLifecycleEvents.SERVER_STARTED.register(server -> {
            structureMap=new GroveStructureMap();structureOwners=List.of();mapRetry.clear();mapPublishTick=0;
            cursor=generated=lastSurveyTick=0;leadX=leadZ=0;retry.clear();expansions.clear();last="Sin explorar";attempt="null";
            enabled=Boolean.parseBoolean(System.getProperty("grovecraft.ecology","true"));
            // Resolve every palette entry while real registries are ready, before
            // an unvisited village can fail halfway through construction.
            var palette=new HashSet<String>();
            for(var kind:GroveEcologyRules.Kind.values())for(var cell:GroveEcologyRules.plan(new GroveEcologyRules.Site("palette",0,0,kind,0)))palette.add(cell.block());
            for(var material:palette)state(material);
            LibertyCraft.LOG.info("[GroveCraft] validated {} blueprint materials",palette.size());
            surfaceAtlas=null;atlasLevel=server.overworld();
            String atlasFile=GroveReleasePaths.surfaceAtlas();
            if(atlasFile!=null)try { surfaceAtlas=GroveSurfaceAtlas.open(Path.of(atlasFile)); }
            catch(Exception e){LibertyCraft.LOG.error("Surface atlas unavailable; native surveys remain active",e);}
            try { worldPlan=GroveWorldPlan.loadOrCreate(server.getWorldPath(LevelResource.ROOT),server.overworld().getSeed());
                if(surfaceAtlas!=null)worldPlan.resolve(server.getWorldPath(LevelResource.ROOT),surfaceAtlas);
                plannedX=plannedZ=Integer.MIN_VALUE;plannedCandidates=List.of();
                structureOwners=worldPlan.all().stream().filter(s->GroveStructureMap.kind(s.kind())!=0).toList();
                LibertyCraft.LOG.info("[GroveCraft] persistent world plan: {} sites prepared",worldPlan.size());
            }catch(Exception e){worldPlan=null;enabled=false;LibertyCraft.LOG.error("World plan unavailable; ecology paused safely",e);}
            Path file=server.getWorldPath(LevelResource.ROOT).resolve(CONFIG);
            if(Files.exists(file)) try(var in=Files.newBufferedReader(file)) {
                Properties p=new Properties();p.load(in);enabled=Boolean.parseBoolean(p.getProperty("enabled","true"));
            }catch(Exception e){ enabled=false;LibertyCraft.LOG.error("[GroveCraft] Ecology config unreadable; generation paused",e); }
        });
        ServerLifecycleEvents.SERVER_STOPPED.register(server -> { retry.clear();enabled=false;atlasLevel=null;surfaceAtlas=null;
            structureMap=new GroveStructureMap();structureOwners=List.of();mapRetry.clear(); });
        ServerTickEvents.END_SERVER_TICK.register(GroveEcology::tick);
        CommandRegistrationCallback.EVENT.register((dispatcher,registry,environment) -> {
            var world=Commands.literal("world").requires(s -> s.permissions().hasPermission(Permissions.COMMANDS_GAMEMASTER)
                || s.getPlayer()!=null && s.getServer().isSingleplayerOwner(s.getPlayer().nameAndId()));
            for(boolean on:new boolean[]{true,false}) world.then(Commands.literal(on?"on":"off").executes(c -> {
                try {
                    Path file=c.getSource().getServer().getWorldPath(LevelResource.ROOT).resolve(CONFIG);
                    Path temp=file.resolveSibling(CONFIG+".tmp");Files.writeString(temp,"# GroveCraft countryside\nenabled="+on+"\n");
                    Files.move(temp,file,StandardCopyOption.REPLACE_EXISTING);enabled=on;
                    c.getSource().sendSuccess(() -> Component.literal("Generación "+(on?"activada":"pausada")+". Lo construido permanece."),false);return 1;
                }catch(Exception e){c.getSource().sendFailure(Component.literal("No se pudo guardar la configuración."));return 0;}
            }));
            world.then(Commands.literal("status").executes(c -> {
                c.getSource().sendSuccess(() -> Component.literal("Campo: "+(enabled?"activo":"pausado")+" | "+generated+" zonas nuevas esta sesión | "+last),false);return 1;
            }));
            world.then(Commands.literal("map").executes(c->{
                c.getSource().sendSuccess(()->Component.literal("Mapa: parcelas del mundo visibles desde el inicio; las estructuras visitadas conservan su ubicación real. Casa: aldea con aldeanos; bandera: saqueadores; camión: parada comercial; $: granja; cubiertos: campamento; ?: ruinas. Hasta 64 destinos; consulta los lejanos en el mapa de pausa."),false);return 1;
            }));
            if("GroveCraftGtaTestCuts20261014".equals(LibertyCraft.WORLD_NAME))world.then(Commands.literal("probe")
                .then(Commands.argument("pos",net.minecraft.commands.arguments.coordinates.BlockPosArgument.blockPos())
                    .executes(c->digProbe(c.getSource(),net.minecraft.commands.arguments.coordinates.BlockPosArgument.getBlockPos(c,"pos"),"legacy"))
                    .then(Commands.argument("ticket",com.mojang.brigadier.arguments.StringArgumentType.word())
                        .executes(c->digProbe(c.getSource(),net.minecraft.commands.arguments.coordinates.BlockPosArgument.getBlockPos(c,"pos"),
                            com.mojang.brigadier.arguments.StringArgumentType.getString(c,"ticket"))))));
            world.then(Commands.literal("survey").executes(c -> {
                var player=c.getSource().getPlayerOrException();var level=player.level();
                int gx=Math.floorDiv(player.getBlockX(),192),gz=Math.floorDiv(player.getBlockZ(),192);
                int count=0;
                for(int dx=-1;dx<=1;dx++)for(int dz=-1;dz<=1;dz++){
                    var site=GroveEcologyRules.site(level.getSeed(),gx+dx,gz+dz,false);
                    if(Math.hypot(site.x()-player.getX(),site.z()-player.getZ())>176
                        ||!GroveEcologyRules.countryside(site.x(),site.z(),GroveEcologyRules.radius(site.kind())+3))continue;
                    if(site.kind()==GroveEcologyRules.Kind.HAMLET){
                        int search=searches.getOrDefault(site.key(),0);var placed=GroveHamletPlan.nearby(site,search);
                        var fit=GroveHamletPlan.fit(placed,player.getY(),(x,z)->NATIVE.settlement(x,z,player.getY()));
                        var anchor=new BlockPos(site.x(),player.getBlockY(),site.z());
                        var done=level.hasChunkAt(anchor)?level.getChunkAt(anchor).getAttached(LEDGER):null;
                        String report=site.key()+" HAMLET candidate="+placed.x()+","+placed.z()+" search="+search
                            +" terrain="+(fit.valid()?"ready":fit.failure())+" visited="+(done!=null&&done.contains(site.key()));
                        LibertyCraft.LOG.info("[GroveCraft] structure survey {}",report);
                        c.getSource().sendSuccess(()->Component.literal(report),false);count++;continue;
                    }
                    int r=GroveEcologyRules.radius(site.kind())+2,missing=0;double min=Double.POSITIVE_INFINITY,max=Double.NEGATIVE_INFINITY;
                    for(int x=-r;x<=r;x++)for(int z=-r;z<=r;z++){
                        double y=NATIVE.grass(site.x()+x,site.z()+z,player.getY());
                        if(!Double.isFinite(y)){missing++;continue;}min=Math.min(min,y);max=Math.max(max,y);
                    }
                    var anchor=new BlockPos(site.x(),player.getBlockY(),site.z());
                    var done=level.hasChunkAt(anchor)?level.getChunkAt(anchor).getAttached(LEDGER):null;
                    String relief=Double.isFinite(min)?Double.toString(max-min):"unavailable";
                    String report=site.key()+" "+site.kind()+" @"+site.x()+","+site.z()+" grassMissing="+missing+" relief="+relief+" visited="+(done!=null&&done.contains(site.key()));
                    LibertyCraft.LOG.info("[GroveCraft] structure survey {}",report);
                    c.getSource().sendSuccess(()->Component.literal(report),false);count++;
                }return count;
            }));
            dispatcher.register(Commands.literal("grovecraft").then(world));
        });
    }
    private static void tick(MinecraftServer server) {
        // Send on the first active tick as well as periodically; no exploration gate.
        if((mapPublishTick==0||server.getTickCount()-mapPublishTick>=20)&&Link.active()&&!server.getPlayerList().getPlayers().isEmpty()
            &&Link.readSkyState(SKY)&&SKY.worldId==0&&SKY.inGame()&&!SKY.loading()){
            var player=server.getPlayerList().getPlayers().getFirst();
            if(BlockyCity.isMirror(player.level())){
                structureMap.restore(player.getAttached(MAP_LEDGER));
                restoreStructureMarkers(player.level());
                var saved=structureMap.ledger();if(!saved.equals(player.getAttached(MAP_LEDGER)))player.setAttached(MAP_LEDGER,saved);
                // Preview entries are regenerated from the personal world plan,
                // never persisted as evidence of successful structure placement.
                var destinations=new GroveStructureMap();destinations.restore(saved);destinations.addPlanned(worldPlan);
                var points=destinations.nearest(player.getX(),player.getZ());
                int sequence=mapSequence=mapSequence==Integer.MAX_VALUE?1:mapSequence+1;
                boolean sent=Link.pushEventLeaving(64,54,0,0,0,0,points.size(),sequence,0);
                for(int i=0;sent&&i<points.size();i++){var point=points.get(i);
                    sent=Link.pushEventLeaving(64,54,i+1,point.x()+.5f,point.y()+1,point.z()+.5f,0,sequence,point.kind());}
                if(sent&&Link.pushEventLeaving(64,54,GroveStructureMap.LIMIT+1,0,0,0,points.size(),sequence,0))mapPublishTick=server.getTickCount();
            }
        }
        if(!enabled || server.getTickCount()%5!=0 || !Link.active() || !HostCollision.active()
            || !Link.readSkyState(SKY) || !GroveEcologyRules.safe(SKY.flags,SKY.worldId,SKY.inGame(),SKY.loading())) return;
        if(server.getPlayerList().getPlayers().isEmpty())return;
        ServerPlayer p=server.getPlayerList().getPlayers().getFirst();
        ServerLevel level=p.level();
        if(!p.isAlive()||p.isSpectator()||!BlockyCity.isMirror(level))return;
        int surveyTick=server.getTickCount();
        if(lastSurveyTick!=0&&surveyTick>lastSurveyTick){
            double scale=60.0/(surveyTick-lastSurveyTick); // three seconds ahead at normal tick rate
            double vx=(p.getX()-lastX)*scale,vz=(p.getZ()-lastZ)*scale,n=Math.hypot(vx,vz);
            double[] lead=GroveSurveyRules.lookAhead(vx,vz);
            leadX=lead[0];leadZ=lead[1];
        }
        lastX=p.getX();lastZ=p.getZ();lastSurveyTick=surveyTick;
        if(worldPlan==null||worldPlan.seed!=level.getSeed())return;
        int planX=Math.floorDiv(p.getBlockX(),16),planZ=Math.floorDiv(p.getBlockZ(),16);
        if(planX!=plannedX||planZ!=plannedZ){
            plannedCandidates=worldPlan.nearby(p.getX(),p.getZ(),200);plannedX=planX;plannedZ=planZ;
        }
        List<GroveEcologyRules.Site> candidates=new ArrayList<>();
        for(var site:plannedCandidates){
            var placement=worldPlan.placement(site);
            if(placement!=null&&!Double.isFinite(placement.height()))continue;
            var actual=worldPlan.location(site);
            double distance=Math.hypot(actual.x()-p.getX(),actual.z()-p.getZ());
            boolean tree=site.kind()==GroveEcologyRules.Kind.OAK||site.kind()==GroveEcologyRules.Kind.BIRCH;
            if(distance>=(tree?8:36)&&distance<=176)candidates.add(site);
        }
        if(candidates.isEmpty()) { last="Busca terreno rural abierto";attempt="null";telemetry();return; }
        candidates=GroveSurveyRules.order(candidates,p.getX(),p.getZ(),leadX,leadZ,worldPlan::location);
        // Bounded pre-generation independent of the view: four rural plots per pass,
        // one heavier structure at most, with retries allowing newly streamed columns.
        int surveyed=0;boolean structureSurveyed=false;
        for(var site:candidates){
            if(surveyed>=8)break;
            if(retry.getOrDefault(site.key(),0)>server.getTickCount())continue;
            boolean tree=site.kind()==GroveEcologyRules.Kind.OAK||site.kind()==GroveEcologyRules.Kind.BIRCH;
            if(!tree&&structureSurveyed)continue;
            if(!tree)structureSurveyed=true;
            surveyed++;
        int search=searches.getOrDefault(site.key(),0);
        boolean adaptive=site.kind()==GroveEcologyRules.Kind.HAMLET||site.kind()==GroveEcologyRules.Kind.OUTPOST||site.kind()==GroveEcologyRules.Kind.WAYSTATION;
        var resolved=worldPlan.placement(site);
        if(resolved!=null&&!Double.isFinite(resolved.height()))continue;
        var placed=resolved!=null?resolved.site():adaptive?GroveHamletPlan.nearby(site,search):site;
        double placedDistance=Math.hypot(placed.x()-p.getX(),placed.z()-p.getZ());
        String result;
        boolean hamlet=site.kind()==GroveEcologyRules.Kind.HAMLET;
        if(placedDistance>176||adaptive&&(placedDistance<(hamlet?36:64)
                ||!GroveEcologyRules.countryside(placed.x(),placed.z(),GroveEcologyRules.radius(placed.kind())+3)))
            result="parcela fuera de alcance";
        else {
            // Ask the host to survey the specific missing plot, retaining its ray budget.
            // This prioritizes measurements; it never treats unknown space as empty.
            if(resolved==null)Link.pushEvent(46,SKY.worldId,placed.x(),(float)p.getY(),placed.z(),GroveEcologyRules.radius(placed.kind())+2,0,0);
            double plannedHeight=resolved!=null?resolved.height():surfaceAtlas==null?Double.NaN:surfaceAtlas.ground(placed.x(),placed.z());
            result=generateAt(level,site,placed,Double.isFinite(plannedHeight)?plannedHeight:p.getY(),NATIVE);
        }
        attempt="{\"key\":\""+site.key()+"\",\"kind\":\""+site.kind()+"\",\"x\":"+site.x()+",\"z\":"+site.z()
            +",\"placed_x\":"+placed.x()+",\"placed_z\":"+placed.z()+",\"search\":"+search
            +",\"result\":\""+result+"\",\"seed\":"+level.getSeed()+"}";
        if(site.key().startsWith("p:")){expansions.put(site.key(),attempt);if(expansions.size()>64)expansions.remove(expansions.keySet().iterator().next());}
        if(result.equals("generated")) { generated++;last=site.kind()+" en "+placed.x()+", "+placed.z(); }
        else {
            boolean advance=resolved==null&&adaptive&&!result.equals("visited");
            if(advance)searches.put(site.key(),(search+1)%25);
            retry.put(site.key(),server.getTickCount()+(result.equals("visited")?1200:advance&&search<24?5:result.contains("pendiente")||result.equals("chunk desconocido")?20:100));
            if(!result.equals("visited"))last="Terreno: "+result;
        }
        }
        if(searches.size()>256)searches.remove(searches.keySet().iterator().next());
        if(retry.size()>2048)retry.remove(retry.keySet().iterator().next());
        telemetry();
    }
    private static void telemetry() {
        try { Files.writeString(net.fabricmc.loader.api.FabricLoader.getInstance().getGameDir().resolve("grovecraft-ecology-status.json"),
            "{\"planned\":"+(worldPlan==null?0:worldPlan.size())+",\"terrain_resolved\":"+(worldPlan==null?0:worldPlan.resolvedCount())+",\"enabled\":"+enabled+",\"map_structures\":"+structureMap.size()+",\"generated_session\":"+generated+",\"last\":\""+last+"\",\"attempt\":"+attempt+",\"expansions\":["+String.join(",",expansions.values())+"]}"); }
        catch(Exception e){LibertyCraft.LOG.debug("Ecology telemetry",e);}
    }
    private static void restoreStructureMarkers(ServerLevel level){
        // Inspect loaded anchor chunks only; map discovery never loads or generates a chunk.
        for(var owner:structureOwners){
            var anchor=new BlockPos(owner.x(),64,owner.z());if(!level.hasChunkAt(anchor))continue;
            var chunk=level.getChunkAt(anchor);var done=chunk.getAttached(LEDGER);structureMap.restore(done);
            if(done==null||!done.contains(owner.key())||structureMap.contains(owner.key()))continue;
            if(mapRetry.getOrDefault(owner.key(),0L)>level.getGameTime())continue;
            mapRetry.put(owner.key(),level.getGameTime()+1200);
            // Older saves predate map metadata. Match their actual blueprint before registering;
            // never turn an ungenerated planned plot into a destination on the map.
            var placement=worldPlan.placement(owner);
            boolean adaptive=owner.kind()==GroveEcologyRules.Kind.HAMLET||owner.kind()==GroveEcologyRules.Kind.OUTPOST||owner.kind()==GroveEcologyRules.Kind.WAYSTATION;
            for(int candidate=0;candidate<(placement==null&&adaptive?25:1);candidate++){
                var site=placement!=null?placement.site():adaptive?GroveHamletPlan.nearby(owner,candidate):owner;
                double height=placement!=null?placement.height():NATIVE.settlement(site.x(),site.z(),64);
                if(!Double.isFinite(height))continue;
                var plan=GroveEcologyRules.plan(site);
                for(int base=(int)Math.floor(height)-4;base<=(int)Math.floor(height)+4;base++){
                    int matches=0;boolean mismatch=false;
                    for(var cell:plan){
                        if(cell.y()<=0)continue;
                        var pos=new BlockPos(site.x()+cell.x(),base+cell.y(),site.z()+cell.z());
                        if(!level.hasChunkAt(pos)){mismatch=true;break;}
                        if(level.getBlockState(pos).getBlock()==state(cell.block()).getBlock())matches++;
                        if(matches>=5)break;
                    }
                    if(!mismatch&&matches>=5){
                        var point=new GroveStructureMap.Point(owner.key(),GroveStructureMap.kind(site.kind()),site.x(),base,site.z());
                        var next=new ArrayList<>(done);next.add(GroveStructureMap.encode(point));chunk.setAttached(LEDGER,List.copyOf(next));
                        structureMap.remember(point);break;
                    }
                }
                if(structureMap.contains(owner.key()))break;
            }
        }
    }
    static String generate(ServerLevel level,GroveEcologyRules.Site site,double nearY,Terrain terrain) {
        return generateAt(level,site,site,nearY,terrain);
    }
    static String generateAt(ServerLevel level,GroveEcologyRules.Site owner,GroveEcologyRules.Site site,double nearY,Terrain terrain) {
        BlockPos anchor=new BlockPos(owner.x(),(int)nearY,owner.z());
        if(!level.hasChunkAt(anchor))return "chunk desconocido";
        LevelChunk chunk=level.getChunkAt(anchor);
        List<String> done=chunk.getAttached(LEDGER);
        if(done!=null&&done.contains(owner.key()))return "visited";
        if(site.kind()==GroveEcologyRules.Kind.HAMLET)return hamlet(level,chunk,done,owner,site,nearY,terrain);
        if(site.kind()==GroveEcologyRules.Kind.OUTPOST||site.kind()==GroveEcologyRules.Kind.WAYSTATION)
            return smallStructure(level,chunk,done,owner,site,nearY,terrain);
        int r=GroveEcologyRules.radius(site.kind())+2, height=GroveEcologyRules.height(site.kind());
        boolean tree=site.kind()==GroveEcologyRules.Kind.OAK||site.kind()==GroveEcologyRules.Kind.BIRCH;
        double min=Double.MAX_VALUE,max=-Double.MAX_VALUE;
        double[][] ground=new double[2*r+1][2*r+1];
        for(int dx=-r;dx<=r;dx++) for(int dz=-r;dz<=r;dz++) {
            // A trunk needs a grass root patch. Its canopy may overhang other ground;
            // every actual log/leaf cell still has to clear native geometry below.
            if(tree&&(Math.abs(dx)>1||Math.abs(dz)>1))continue;
            double y=terrain.grass(site.x()+dx,site.z()+dz,nearY);
            if(!Double.isFinite(y)||Math.abs(y-nearY)>24)return "suelo no apto";
            ground[dx+r][dz+r]=y;
            min=Math.min(min,y);max=Math.max(max,y);
        }
        if(max-min>(tree?3.5:1.5))return "pendiente";
        // Put the base into the surface cell, rather than levitating trunks or raising cabin
        // entrances a full block above the next integer height. Only this grass cell may overlap GTA.
        int base=(int)Math.floor(tree?ground[r][r]:max);
        // Validate the whole enclosure, not just occupied blueprint cells: leave native structures,
        // player blocks, water, excavated ground and breathing space intact.
        var plan=new ArrayList<>(GroveEcologyRules.plan(site));
        Set<BlockPos> treeCells=new HashSet<>();
        if(tree) for(var c:plan) treeCells.add(new BlockPos(site.x()+c.x(),base+c.y(),site.z()+c.z()));
        for(int dx=-r;dx<=r;dx++)for(int dz=-r;dz<=r;dz++)for(int y=(int)Math.floor(min)-1;y<=base+height+2;y++) {
            BlockPos pos=new BlockPos(site.x()+dx,y,site.z()+dz);
            if(tree&&!treeCells.contains(pos)&&!(Math.abs(dx)<=1&&Math.abs(dz)<=1&&y<=base+1))continue;
            if(!level.hasChunkAt(pos)||!terrain.known(pos.getX(),y,pos.getZ()))return "geometría pendiente";
            if(!level.getBlockState(pos).isAir() || HostDig.isDug(level,0,pos))return "zona modificada";
            if(!tree && y>Math.floor(ground[dx+r][dz+r]) && terrain.occupied(pos))return "obstáculo nativo";
        }
        AABB box=new AABB(site.x()-r,base,site.z()-r,site.x()+r+1,base+height+2,site.z()+r+1);
        if(!level.getEntitiesOfClass(Entity.class,box,e -> e.isAlive()).isEmpty())return "zona ocupada";
        if(tree) {
            for(var cell:plan) if(cell.y()>0&&terrain.occupied(new BlockPos(site.x()+cell.x(),base+cell.y(),site.z()+cell.z())))return "obstáculo nativo";
        } else {
            // Modest slopes get foundations instead of floating floors or terrain flattening.
            for(var cell:List.copyOf(plan)) if(cell.y()==0) {
                int floor=(int)Math.floor(ground[cell.x()+r][cell.z()+r]);
                for(int y=base-1;y>=floor;y--)plan.add(new GroveEcologyRules.Cell(cell.x(),y-base,cell.z(),"cobblestone"));
            }
        }
        if(plan.size()>768)return "presupuesto";
        return place(level,chunk,done,owner,site,base,base,base,plan);
    }
    private static String hamlet(ServerLevel level,LevelChunk chunk,List<String> done,GroveEcologyRules.Site owner,
                                  GroveEcologyRules.Site site,double nearY,Terrain terrain){
        var fit=GroveHamletPlan.fit(site,nearY,(x,z)->terrain.settlement(x,z,nearY));
        if(!fit.valid())return fit.failure();
        Map<GroveHamletPlan.Column,Integer> tops=new HashMap<>();
        for(var c:fit.cells())tops.merge(new GroveHamletPlan.Column(c.x(),c.z()),fit.base()+c.y()+2,Math::max);
        for(var e:fit.ground().entrySet()){
            var column=e.getKey();int floor=(int)Math.floor(e.getValue());
            int top=Math.max(tops.get(column),fit.floors().get(column)+3);
            for(int y=floor-1;y<=top;y++){
                var pos=new BlockPos(site.x()+column.x(),y,site.z()+column.z());
                if(!level.hasChunkAt(pos)||!terrain.known(pos.getX(),y,pos.getZ()))return "geometría pendiente";
                if(!level.getBlockState(pos).isAir()||HostDig.isDug(level,0,pos))return "zona modificada";
                if(y>floor&&terrain.occupied(pos))return "obstáculo nativo";
            }
        }
        // Keep living traffic/CJ clear of the complete village, including its gaps.
        int r=GroveEcologyRules.radius(site.kind());
        var box=new AABB(site.x()-r,fit.base(),site.z()-r,site.x()+r+1,fit.base()+12,site.z()+r+1);
        if(!level.getEntitiesOfClass(Entity.class,box,e->e.isAlive()).isEmpty())return "zona ocupada";
        return place(level,chunk,done,owner,site,fit.base(),fit.west(),fit.east(),fit.cells());
    }
    private static String smallStructure(ServerLevel level,LevelChunk chunk,List<String> done,GroveEcologyRules.Site owner,
                                        GroveEcologyRules.Site site,double nearY,Terrain terrain){
        if(site.kind()==GroveEcologyRules.Kind.OUTPOST&&level.getDifficulty()!=net.minecraft.world.Difficulty.PEACEFUL
            &&level.getEntitiesOfClass(Mob.class,new AABB(site.x()-160,nearY-32,site.z()-160,site.x()+160,nearY+32,site.z()+160),
                e->e.isAlive()&&e instanceof net.minecraft.world.entity.monster.Enemy).size()>13)return "zona ocupada";
        var plan=new ArrayList<>(GroveEcologyRules.plan(site));
        Map<GroveHamletPlan.Column,Double> ground=new LinkedHashMap<>();double min=Double.POSITIVE_INFINITY,max=Double.NEGATIVE_INFINITY;
        for(var c:plan){var column=new GroveHamletPlan.Column(c.x(),c.z());if(ground.containsKey(column))continue;
            double y=terrain.settlement(site.x()+c.x(),site.z()+c.z(),nearY);
            if(!Double.isFinite(y)||Math.abs(y-nearY)>24)return "suelo no apto";
            ground.put(column,y);min=Math.min(min,y);max=Math.max(max,y);
        }
        if(max-min>3)return "pendiente";int base=(int)Math.floor(max);
        for(var e:ground.entrySet()){
            int x=site.x()+e.getKey().x(),z=site.z()+e.getKey().z(),floor=(int)Math.floor(e.getValue());
            for(int y=floor-1;y<=base+GroveEcologyRules.height(site.kind())+2;y++){
                var p=new BlockPos(x,y,z);
                if(!level.hasChunkAt(p)||!terrain.known(x,y,z))return "geometría pendiente";
                if(!level.getBlockState(p).isAir()||HostDig.isDug(level,0,p))return "zona modificada";
                if(y>floor&&terrain.occupied(p))return "obstáculo nativo";
            }
            // Lower entrance steps are blueprint cells, not empty foundation space.
            int bottom=base;
            for(var cell:plan)if(cell.x()==e.getKey().x()&&cell.z()==e.getKey().z())bottom=Math.min(bottom,base+cell.y());
            for(int y=bottom-1;y>=floor;y--)plan.add(new GroveEcologyRules.Cell(e.getKey().x(),y-base,e.getKey().z(),"cobblestone"));
        }
        int r=GroveEcologyRules.radius(site.kind());
        if(!level.getEntitiesOfClass(Entity.class,new AABB(site.x()-r,base,site.z()-r,site.x()+r+1,base+13,site.z()+r+1),Entity::isAlive).isEmpty())return "zona ocupada";
        if(plan.size()>768)return "presupuesto";
        return place(level,chunk,done,owner,site,base,base,base,plan);
    }
    private static String place(ServerLevel level,LevelChunk chunk,List<String> done,GroveEcologyRules.Site owner,
                                GroveEcologyRules.Site site,int base,int west,int east,List<GroveEcologyRules.Cell> plan){
        List<BlockPos> wrote=new ArrayList<>();
        try {
            // Suppress intermediate neighbour updates until a complete structure exists. Crops,
            // lights, beds, containers and POIs are real vanilla block states/entities.
            for(var cell:plan) {
                BlockPos pos=new BlockPos(site.x()+cell.x(),base+cell.y(),site.z()+cell.z());
                if(!level.setBlock(pos,state(cell.block()),2))throw new IllegalStateException("Cannot place "+pos);
                wrote.add(pos);
            }
            for(BlockPos pos:wrote) level.updateNeighborsAt(pos,level.getBlockState(pos).getBlock());
            for(BlockPos pos:wrote) if(level.getBlockEntity(pos) instanceof BarrelBlockEntity barrel) loot(barrel,site.seed()^pos.asLong(),site.kind());
            List<String> next=new ArrayList<>(done==null?List.of():done);next.add(owner.key());
            GroveStructureMap.Point marker=null;
            if(GroveStructureMap.kind(site.kind())!=0){
                marker=new GroveStructureMap.Point(owner.key(),GroveStructureMap.kind(site.kind()),site.x(),base,site.z());
                next.add(GroveStructureMap.encode(marker));
            }
            chunk.setAttached(LEDGER,List.copyOf(next));if(marker!=null)structureMap.remember(marker);
            try { inhabitants(level,site,base,west,east); }
            catch(Exception e) { LibertyCraft.LOG.error("[GroveCraft] Structure saved, inhabitants unavailable",e); }
            LibertyCraft.LOG.info("[GroveCraft] ecology {} {} at {} {} {} ({} blocks)",site.key(),site.kind(),site.x(),base,site.z(),wrote.size());
            return "generated";
        }catch(Exception e) {
            for(BlockPos pos:wrote) level.setBlock(pos,Blocks.AIR.defaultBlockState(),3);
            LibertyCraft.LOG.error("[GroveCraft] Ecology placement rolled back",e);return "error de colocación";
        }
    }
    private static BlockState state(String name) {
        if(name.equals("cobble_stairs_south"))return Blocks.COBBLESTONE_STAIRS.defaultBlockState().setValue(StairBlock.FACING,Direction.SOUTH);
        if(name.equals("stairs_north"))return Blocks.DARK_OAK_STAIRS.defaultBlockState().setValue(StairBlock.FACING,Direction.NORTH);
        if(name.equals("stairs_south"))return Blocks.DARK_OAK_STAIRS.defaultBlockState().setValue(StairBlock.FACING,Direction.SOUTH);
        if(name.equals("stairs_west"))return Blocks.OAK_STAIRS.defaultBlockState().setValue(StairBlock.FACING,Direction.WEST);
        if(name.equals("stairs_east"))return Blocks.OAK_STAIRS.defaultBlockState().setValue(StairBlock.FACING,Direction.EAST);
        if(name.equals("bed_foot")||name.equals("bed_head"))return Blocks.BED.white().defaultBlockState()
            .setValue(BedBlock.FACING,Direction.SOUTH).setValue(BedBlock.PART,name.equals("bed_head")?BedPart.HEAD:BedPart.FOOT);
        Block b=BuiltInRegistries.BLOCK.getValue(Identifier.fromNamespaceAndPath("minecraft",name));
        if(b==null||b==Blocks.AIR)throw new IllegalArgumentException(name);
        BlockState s=b.defaultBlockState();
        if(b instanceof LeavesBlock)s=s.setValue(LeavesBlock.DISTANCE,1); // natural leaves: decay once logs are removed
        if(b==Blocks.FARMLAND)s=s.setValue(FarmlandBlock.MOISTURE,7);
        if(b==Blocks.WHEAT)s=s.setValue(CropBlock.AGE,Math.floorMod(name.hashCode(),4)+2);
        return s;
    }
    private static void loot(BarrelBlockEntity barrel,long seed,GroveEcologyRules.Kind kind) {
        Random r=new Random(seed);int slot=0;
        if(kind==GroveEcologyRules.Kind.OUTPOST){
            barrel.setItem(slot++,new ItemStack(Items.ARROW,8+r.nextInt(9)));
            barrel.setItem(slot++,new ItemStack(Items.IRON_INGOT,2+r.nextInt(3)));
            barrel.setItem(slot++,new ItemStack(Items.EMERALD,1+r.nextInt(3)));
            barrel.setItem(slot++,new ItemStack(Items.BREAD,2+r.nextInt(3)));
            if((seed&3)==0)barrel.setItem(slot++,new ItemStack(Items.CROSSBOW));
            barrel.setChanged();return;
        }
        barrel.setItem(slot++,new ItemStack(Items.BREAD,2+r.nextInt(4)));
        barrel.setItem(slot++,new ItemStack(Items.WHEAT_SEEDS,3+r.nextInt(5)));
        barrel.setItem(slot++,new ItemStack(Items.TORCH,3+r.nextInt(5)));
        if(kind==GroveEcologyRules.Kind.FARM||kind==GroveEcologyRules.Kind.HAMLET) {
            barrel.setItem(slot++,new ItemStack(Items.CARROT,2+r.nextInt(3)));
            barrel.setItem(slot++,new ItemStack(Items.POTATO,2+r.nextInt(3)));
        } else {
            barrel.setItem(slot++,new ItemStack(Items.COAL,2+r.nextInt(4)));
            barrel.setItem(slot++,new ItemStack(Items.IRON_INGOT,1+r.nextInt(2)));
        }
        barrel.setChanged();
    }
    private static void inhabitants(ServerLevel level,GroveEcologyRules.Site site,int base,int west,int east) {
        if((site.kind()==GroveEcologyRules.Kind.OAK||site.kind()==GroveEcologyRules.Kind.BIRCH)&&(site.seed()&63)==0){
            // A seed-owned initial herd, saved with its tree's one-time ledger.
            // Re-entering the parcel does not refill hunted animals.
            String[] kinds={"cow","pig","sheep","chicken"};
            var type=BuiltInRegistries.ENTITY_TYPE.getValue(Identifier.withDefaultNamespace(kinds[(int)((site.seed()>>>8)&3)]));
            for(int i=0;i<2;i++){
                int x=site.x()+4,z=site.z()+i*2-1;double ground=NATIVE.grass(x,z,base);
                if(!Double.isFinite(ground))continue;
                var entity=type.create(level,EntitySpawnReason.NATURAL);
                if(!(entity instanceof Animal animal))continue;
                animal.snapTo(x+.5,Math.ceil(ground)+.05,z+.5,(float)Math.floorMod(site.seed(),360),0);
                if(!level.noCollision(animal,animal.getBoundingBox()))continue;
                animal.finalizeSpawn(level,level.getCurrentDifficultyAt(animal.blockPosition()),EntitySpawnReason.NATURAL,null);
                animal.setPersistenceRequired();animal.addTag("grovecraft_world_animal");level.addFreshEntity(animal);
            }
        }
        if(site.kind()==GroveEcologyRules.Kind.HAMLET) for(int dx:new int[]{-5,6}) {
            Villager v=EntityTypes.VILLAGER.create(level,EntitySpawnReason.STRUCTURE);
            if(v!=null) {
                v.snapTo(site.x()+dx+.5,(dx<0?west:east)+1,site.z()-.5,0,0);
                v.finalizeSpawn(level,level.getCurrentDifficultyAt(v.blockPosition()),EntitySpawnReason.STRUCTURE,null);
                v.setPersistenceRequired();v.addTag("grovecraft_settler");
                BlockPos job=dx<0?findWork(level,site.x()-1,site.z()-3,base,Blocks.COMPOSTER):new BlockPos(site.x()+4,east+1,site.z()-2);
                if(job!=null)GroveVillageEconomy.establish(v,level,dx<0,job);
                level.addFreshEntity(v);
            }
        }
        if(site.kind()==GroveEcologyRules.Kind.WAYSTATION){
            Villager v=EntityTypes.VILLAGER.create(level,EntitySpawnReason.STRUCTURE);
            if(v!=null){v.snapTo(site.x()+.5,base+1,site.z()+.5,0,0);
                v.finalizeSpawn(level,level.getCurrentDifficultyAt(v.blockPosition()),EntitySpawnReason.STRUCTURE,null);
                v.setPersistenceRequired();v.addTag("grovecraft_settler");
                GroveVillageEconomy.establish(v,level,false,new BlockPos(site.x()-1,base,site.z()+4));
                v.addTag("grovecraft_waystation_access_v4");level.addFreshEntity(v);}
        }
        if(site.kind()==GroveEcologyRules.Kind.OUTPOST&&level.getDifficulty()!=net.minecraft.world.Difficulty.PEACEFUL){
            int count=level.getEntitiesOfClass(Mob.class,
                new AABB(site.x()-160,base-32,site.z()-160,site.x()+160,base+32,site.z()+160),e->e.isAlive()&&e instanceof net.minecraft.world.entity.monster.Enemy).size();
            for(int i=0;i<Math.min(3,16-count);i++){
                var guard=EntityTypes.PILLAGER.create(level,EntitySpawnReason.STRUCTURE);if(guard==null)continue;
                guard.snapTo(site.x()+(i==0?-1:i==1?1:0)+.5,base+(i==2?9:1),site.z()+(i==2?0:1)+.5,180,0);
                guard.finalizeSpawn(level,level.getCurrentDifficultyAt(guard.blockPosition()),EntitySpawnReason.STRUCTURE,null);
                guard.setPersistenceRequired();guard.setCanJoinRaid(false);guard.addTag("grovecraft_outpost_guard");
                if(level.noCollision(guard,guard.getBoundingBox()))level.addFreshEntity(guard);
            }
        }
        // Farm wildlife joins existing natural spawn caps, rather than refilling every visit.
        if(site.kind()==GroveEcologyRules.Kind.FARM||site.kind()==GroveEcologyRules.Kind.HAMLET) {
            int count=level.getEntitiesOfClass(Animal.class,new AABB(site.x()-128,base-32,site.z()-128,site.x()+128,base+32,site.z()+128)).size();
            for(int i=0;i<Math.min(2,8-count);i++) {
                int dz=i*2-1,dx=site.kind()==GroveEcologyRules.Kind.HAMLET?9:5;
                Animal a=EntityTypes.CHICKEN.create(level,EntitySpawnReason.STRUCTURE);
                if(a!=null) {
                    double surface=site.kind()==GroveEcologyRules.Kind.HAMLET?NATIVE.grass(site.x()+dx,site.z()+dz,base):base;
                    if(!Double.isFinite(surface))continue;
                    a.snapTo(site.x()+dx+.5,Math.ceil(surface)+1,site.z()+dz+.5,0,0);
                    if(!level.noCollision(a,a.getBoundingBox()))continue;
                    a.setPersistenceRequired();a.addTag("grovecraft_farm_animal");level.addFreshEntity(a); }
            }
        }
    }
    /** Upgrade only recorded, unmodified natural depots, inside their existing footprint.
     * Never fills dug native ground or rebuilds player-modified structures. */
    static BlockPos naturalDepotWorkplace(Villager v){
        var level=(ServerLevel)v.level();int x=v.getBlockX(),z=v.getBlockZ();
        BlockPos best=null;double distance=4096;
        for(int gx=Math.floorDiv(x,384)-1;gx<=Math.floorDiv(x,384)+1;gx++)for(int gz=Math.floorDiv(z,384)-1;gz<=Math.floorDiv(z,384)+1;gz++){
            var owner=GroveEcologyRules.expansionSite(level.getSeed(),gx,gz);if(owner.kind()!=GroveEcologyRules.Kind.WAYSTATION)continue;
            var anchor=new BlockPos(owner.x(),v.getBlockY(),owner.z());if(!level.hasChunkAt(anchor))continue;
            var ledger=level.getChunkAt(anchor).getAttached(LEDGER);if(ledger==null||!ledger.contains(owner.key()))continue;
            for(int i=0;i<25;i++){var site=GroveHamletPlan.nearby(owner,i);
                if(Math.hypot(site.x()-x,site.z()-z)>64)continue;
                for(int y=v.getBlockY()-3;y<=v.getBlockY()+4;y++){
                    var work=new BlockPos(site.x()-2,y,site.z()-2);if(!level.hasChunkAt(work)||!level.getBlockState(work).is(Blocks.SMITHING_TABLE))continue;
                    var center=work.offset(2,-1,2);
                    if(!level.getBlockState(center.above(4)).is(Blocks.OAK_SLAB)||!level.getBlockState(center.offset(2,1,-2)).is(Blocks.BARREL)
                        ||!level.getBlockState(center.offset(2,1,0)).is(Blocks.BARREL))continue;
                    double d=work.distToCenterSqr(v.position());if(d<distance){distance=d;best=work;}
                }
            }
        }return best;
    }
    static String marketAccessStage="not_checked";
    static void repairNaturalMarketAccess(Villager v){
        marketAccessStage="marker_or_role";
        String marker="grovecraft_waystation_access_v4";
        if(v.entityTags().contains(marker)||!v.entityTags().contains("grovecraft_market_supplier"))return;
        marketAccessStage="job_memory";
        var job=v.getBrain().getMemory(net.minecraft.world.entity.ai.memory.MemoryModuleType.JOB_SITE);
        if(job.isEmpty()||job.get().dimension()!=v.level().dimension())return;
        var level=(ServerLevel)v.level();var work=job.get().pos();
        marketAccessStage="workstation_type";
        if(!level.getBlockState(work).is(Blocks.SMITHING_TABLE))return;
        int x=work.getX()+2,z=work.getZ()+2,base=work.getY()-1;
        var center=new BlockPos(x,base,z);
        if(!level.hasChunkAt(center.offset(3,4,3))||!level.hasChunkAt(center.offset(-3,0,-3)))return;
        marketAccessStage="recorded_parcel";
        boolean recorded=false;
        for(int gx=Math.floorDiv(x,384)-1;gx<=Math.floorDiv(x,384)+1;gx++)for(int gz=Math.floorDiv(z,384)-1;gz<=Math.floorDiv(z,384)+1;gz++){
            var owner=GroveEcologyRules.expansionSite(level.getSeed(),gx,gz);
            if(owner.kind()!=GroveEcologyRules.Kind.WAYSTATION)continue;
            boolean matches=false;for(int i=0;i<25;i++){var s=GroveHamletPlan.nearby(owner,i);if(s.x()==x&&s.z()==z){matches=true;break;}}
            var anchor=new BlockPos(owner.x(),base,owner.z());if(!matches||!level.hasChunkAt(anchor))continue;
            var ledger=level.getChunkAt(anchor).getAttached(LEDGER);if(ledger!=null&&ledger.contains(owner.key()))recorded=true;
        }
        marketAccessStage="blueprint_ownership";
        if(!recorded||!level.getBlockState(center.above(4)).is(Blocks.OAK_SLAB)
            ||!level.getBlockState(center.offset(2,1,-2)).is(Blocks.BARREL)
            ||!level.getBlockState(center.offset(2,1,0)).is(Blocks.BARREL))return;
        marketAccessStage="north_entry";
        for(int dx:new int[]{-2,-1}){
            var step=center.offset(dx,0,-3);
            if(HostDig.isDug(level,0,step)||!level.getBlockState(step.above()).isAir()
                ||!(level.getBlockState(step).is(Blocks.COBBLESTONE)||level.getBlockState(step).equals(state("cobble_stairs_south"))))return;
        }
        // Work from an outdoor counter on native ground, without climbing inside.
        // Move the existing table; never create a second resource or reset offers.
        var counter=center.offset(-1,0,4);var support=counter.below();
        marketAccessStage="counter_clearance";
        if(!level.hasChunkAt(counter)||!HostCollision.isKnown(counter.getX(),counter.getY()+2,counter.getZ())
            ||!level.getBlockState(counter).isAir()||!level.getBlockState(support).isAir()
            ||HostDig.isDug(level,0,counter)||HostDig.isDug(level,0,support))return;
        double counterGround=NATIVE.settlement(counter.getX(),counter.getZ(),base);
        marketAccessStage="counter_ground_or_actor";
        if(!Double.isFinite(counterGround)||Math.abs(counterGround-support.getY())>1.5
            ||!level.getEntitiesOfClass(Entity.class,new AABB(support).expandTowards(0,1,0),Entity::isAlive).isEmpty())return;
        for(int dx:new int[]{-2,-1})level.setBlock(center.offset(dx,0,-3),state("cobble_stairs_south"),3);
        level.setBlock(support,Blocks.COBBLESTONE.defaultBlockState(),3);
        level.setBlock(counter,Blocks.SMITHING_TABLE.defaultBlockState(),3);level.setBlock(work,Blocks.AIR.defaultBlockState(),3);
        GroveVillageEconomy.moveWork(v,level,counter);
        v.addTag(marker);marketAccessStage="upgraded";
    }
    // Read-only, registered exclusively in the exact private cuts fixture. A
    // request ticket distinguishes a fresh server reply from queued chat text.
    private static int digProbe(net.minecraft.commands.CommandSourceStack source,BlockPos pos,String ticket)
            throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        var p=source.getPlayerOrException();var sky=new Link.SkyState();
        if(!ticket.matches("[A-Za-z0-9_-]{1,48}")||!Link.active()||!Link.readSkyState(sky)||!p.level().hasChunkAt(pos)
            ||Math.abs((double)pos.getX())>5000||Math.abs((double)pos.getZ())>5000||Math.abs((double)pos.getY())>1000)return 0;
        String report="GROVE_DIG_PROBE x="+pos.getX()+" y="+pos.getY()+" z="+pos.getZ()
            +" block="+BuiltInRegistries.BLOCK.getKey(p.level().getBlockState(pos).getBlock())
            +" dug="+HostDig.isDug(p.level(),sky.worldId,pos)
            +" material="+HostDig.classify(pos.getX(),pos.getY(),pos.getZ())
            +" architecture="+GroveArchitecture.cut(p.level(),sky.worldId,pos)
            +" wood="+GroveNativeTrees.cut(p.level(),sky.worldId,pos)
            +" world="+sky.worldId+" tick="+p.level().getGameTime()+" ticket="+ticket;
        LibertyCraft.LOG.info("{}",report);
        source.sendSuccess(()->Component.literal(report),false);return 1;
    }
    private static BlockPos findWork(ServerLevel level,int x,int z,int base,Block block){
        for(int y=base;y<=base+8;y++){var p=new BlockPos(x,y,z);if(level.getBlockState(p).is(block))return p;}return null;
    }
}
