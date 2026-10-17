package dev.libertycraft.client;
import dev.libertycraft.*;
import dev.libertycraft.link.Link;
import dev.libertycraft.world.city.CityStore;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.screens.TitleScreen;
import net.minecraft.client.multiplayer.ClientLevel;
import net.minecraft.world.level.storage.LevelResource;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;
/** Independent of game tick/pause: a GTA save can request a durable server save. */
public final class GroveCampaignClient {
    private static final String ROOT=GroveReleasePaths.campaignRoot();
    // The launcher supplies a root only for personal campaigns or the paired fixture.
    // Expedition also has a legacy testWorld property; it must follow GTA saves too.
    private static final boolean ENABLED=ROOT!=null&&!ROOT.isBlank();
    private static final ExecutorService IO=Executors.newSingleThreadExecutor(r->{var t=new Thread(r,"GroveCraft paired saves");t.setDaemon(true);return t;});
    private static String handled="",opening="",world="";private static long nextPoll;private static volatile boolean openingAllowed;
    private record Command(int pid,String action,String campaign,String work,String snapshot,String request){}
    private static Path root(){return Path.of(ROOT).toAbsolutePath().normalize();}
    private static void ack(Command c,String text){try{GroveCampaignFiles.atomic(root().resolve("acks").resolve(c.request+".txt"),text+"\n");}
        catch(Exception e){LibertyCraft.LOG.error("Paired save acknowledgment",e);}}
    public static boolean mayOpen(){return !ENABLED||openingAllowed;}
    public static void frame(Minecraft mc){
        if(!ENABLED||!Link.active())return;
        if(!opening.isEmpty()&&mc.level!=null&&mc.player!=null&&mc.getSingleplayerServer()!=null){
            var path=mc.getSingleplayerServer().getWorldPath(LevelResource.ROOT).toAbsolutePath().normalize();
            var expected=FabricLoader.getInstance().getGameDir().resolve("saves").resolve(world).toAbsolutePath().normalize();
            if(path.equals(expected)){try{GroveCampaignFiles.atomic(root().resolve("acks").resolve(opening+".txt"),"ready\n");opening="";}
                catch(Exception e){LibertyCraft.LOG.error("Campaign ready",e);}}
        }
        if(System.nanoTime()<nextPoll)return;nextPoll=System.nanoTime()+200_000_000L;
        try{
            var lines=Files.readAllLines(root().resolve("control.txt"));if(lines.size()!=7||!lines.getFirst().equals("GroveCraftCampaign1"))return;
            var c=new Command(Integer.parseInt(lines.get(1)),lines.get(2),GroveCampaignFiles.id(lines.get(3)),GroveCampaignFiles.id(lines.get(4)),lines.get(5),GroveCampaignFiles.id(lines.get(6)));
            if(c.pid!=Link.hostPid()||c.request.equals(handled))return;handled=c.request;
            if(c.action.equals("wait")){openingAllowed=false;return;}
            if(c.action.equals("open")||c.action.equals("restore"))switchWorld(mc,c);
            else if(c.action.equals("save"))save(mc,c);
            else if(c.action.equals("verify"))IO.execute(()->{
                try{GroveCampaignFiles.verify(root().resolve("snapshots").resolve(GroveCampaignFiles.id(c.snapshot)));ack(c,"verified");}
                catch(Exception e){LibertyCraft.LOG.error("Paired load rejected before GTA changes",e);ack(c,"error invalid-snapshot");}
            });
        }catch(NoSuchFileException ignored){}catch(Exception e){LibertyCraft.LOG.error("Campaign control rejected",e);}
    }
    private static void switchWorld(Minecraft mc,Command c){
        openingAllowed=false;opening="";String target=GroveCampaignFiles.world(c.campaign,c.work);
        if(mc.level!=null)mc.disconnectFromWorld(ClientLevel.DEFAULT_QUIT_MESSAGE);
        mc.gui.setScreen(new TitleScreen());MirrorWorld.campaignReset();
        IO.execute(()->{
            try{
                var saves=FabricLoader.getInstance().getGameDir().resolve("saves");var destination=GroveCampaignFiles.child(saves,target);
                if(Files.exists(destination))throw new IllegalStateException("Campaign working world already exists");
                if(c.action.equals("restore"))GroveCampaignFiles.restore(root().resolve("snapshots").resolve(GroveCampaignFiles.id(c.snapshot)),destination);
                mc.execute(()->{if(!handled.equals(c.request))return;world=target;LibertyCraft.WORLD_NAME=target;opening=c.request;openingAllowed=true;
                    LibertyCraft.LOG.info("[GroveCraft] campaign {} -> {} ({})",c.campaign,target,c.action);});
            }catch(Exception e){LibertyCraft.LOG.error("Campaign world preserved; cannot switch",e);ack(c,"error "+e.getClass().getSimpleName());}
        });
    }
    private static void save(Minecraft mc,Command c){
        if(!world.equals(GroveCampaignFiles.world(c.campaign,c.work))||!opening.isEmpty()||mc.getSingleplayerServer()==null){ack(c,"error wrong-world");return;}
        GroveCampaignFiles.id(c.snapshot);var server=mc.getSingleplayerServer();
        server.execute(()->{
            try{
                for(var player:server.getPlayerList().getPlayers())if(GroveTownEconomy.hasPending(player))
                    throw new IllegalStateException("Finish the pending market payment before saving GTA");
                // Vanilla crafting grids are transient menu inventories. Return their
                // inputs/cursor before snapshotting the persistent player inventory.
                for(var player:server.getPlayerList().getPlayers())player.closeContainer();
                if(!server.saveEverything(false,true,true))throw new IllegalStateException("Minecraft world save failed");
                var city=CityStore.current();if(city!=null)city.flush(true);
                var source=server.getWorldPath(LevelResource.ROOT).toAbsolutePath().normalize();
                var expected=FabricLoader.getInstance().getGameDir().resolve("saves").resolve(world).toAbsolutePath().normalize();
                if(!source.equals(expected))throw new IllegalStateException("Campaign changed before save");
                GroveCampaignFiles.snapshot(source,root().resolve("snapshots").resolve(c.snapshot));
                ack(c,"saved");LibertyCraft.LOG.info("[GroveCraft] paired Minecraft snapshot durable {}",c.snapshot);
            }catch(Exception e){LibertyCraft.LOG.error("GTA paired save refused; previous pair preserved",e);ack(c,"error "+e.getClass().getSimpleName());}
        });
    }
}
