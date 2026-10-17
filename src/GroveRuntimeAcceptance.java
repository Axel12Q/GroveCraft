package dev.libertycraft;
import java.nio.file.*;
import dev.libertycraft.link.Link;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.fabricmc.loader.api.FabricLoader;
/** Read-only server oracle, explicitly restricted to isolated runtime test worlds. */
public final class GroveRuntimeAcceptance {
    private static void marketOracle(net.minecraft.server.level.ServerPlayer player) throws Exception {
        var json=new com.google.gson.JsonObject();var markets=new com.google.gson.JsonArray();var guards=new com.google.gson.JsonArray();
        for(var e:player.level().getEntitiesOfClass(net.minecraft.world.entity.Mob.class,player.getBoundingBox().inflate(192),
            e->e.entityTags().contains("grovecraft_settler")||e.entityTags().contains("grovecraft_outpost_guard"))){
            var row=new com.google.gson.JsonObject();row.addProperty("uuid",e.getUUID().toString());
            row.addProperty("x",e.getX());row.addProperty("y",e.getY());row.addProperty("z",e.getZ());row.addProperty("alive",e.isAlive());
            if(e instanceof net.minecraft.world.entity.npc.villager.Villager v){
                row.addProperty("access_stage",GroveEcology.marketAccessStage);row.addProperty("tags",v.entityTags().toString());row.addProperty("profession",v.getVillagerData().profession().toString());row.addProperty("last_work",work(v));
                row.addProperty("job",v.getBrain().getMemory(net.minecraft.world.entity.ai.memory.MemoryModuleType.JOB_SITE).map(Object::toString).orElse("absent"));
                var offers=new com.google.gson.JsonArray();for(var o:v.getOffers()){
                    var offer=new com.google.gson.JsonObject();offer.addProperty("item",net.minecraft.core.registries.BuiltInRegistries.ITEM.getKey(o.getResult().getItem()).toString());
                    offer.addProperty("uses",o.getUses());offer.addProperty("max_uses",o.getMaxUses());offer.addProperty("sold_out",o.isOutOfStock());offers.add(offer);
                }row.add("offers",offers);markets.add(row);
            }else guards.add(row);
        }
        json.addProperty("game_time",player.level().getGameTime());json.add("markets",markets);json.add("guards",guards);
        Files.writeString(FabricLoader.getInstance().getGameDir().resolve("grovecraft-market-oracle.json"),json.toString());
    }
    private static long work(net.minecraft.world.entity.npc.villager.Villager v){
        var m=net.minecraft.world.entity.ai.memory.MemoryModuleType.LAST_WORKED_AT_POI;
        long remembered=v.getBrain().checkMemory(m,net.minecraft.world.entity.ai.memory.MemoryStatus.REGISTERED)?v.getBrain().getMemory(m).orElse(-1L):-1L;
        return Math.max(remembered,GroveVillageEconomy.observedWork(v));
    }
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.runtimeAcceptance"))return;
        if(!LibertyCraft.WORLD_NAME.startsWith("GroveCraftGtaTest"))throw new IllegalStateException("Private test world required");
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(server.getTickCount()%5!=0||server.getPlayerList().getPlayers().isEmpty())return;
            var player=server.getPlayerList().getPlayers().getFirst();var sky=new Link.SkyState();Link.readSkyState(sky);
            try{Files.writeString(FabricLoader.getInstance().getGameDir().resolve("grovecraft-runtime-oracle.json"),
                "{\"tick\":"+server.getTickCount()+",\"game_time\":"+player.level().getGameTime()+",\"food\":"+player.getFoodData().getFoodLevel()
                +",\"saturation\":"+player.getFoodData().getSaturationLevel()
                +",\"health\":"+player.getHealth()+",\"mirror\":"+dev.libertycraft.world.city.BlockyCity.isMirror(player.level())
                +",\"x\":"+player.getX()+",\"y\":"+player.getY()+",\"z\":"+player.getZ()+",\"host_flags\":"+sky.flags
                +",\"riding\":"+player.isPassenger()+"}");
                if(server.getTickCount()%20==0)marketOracle(player);
                }catch(Exception e){LibertyCraft.LOG.warn("Runtime oracle",e);}
        });
    }
}
