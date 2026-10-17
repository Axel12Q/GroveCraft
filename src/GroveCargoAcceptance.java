package dev.libertycraft;
import java.nio.file.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.world.inventory.ChestMenu;
import net.minecraft.world.item.*;
import net.minecraft.world.level.storage.LevelResource;

/** Two real Minecraft launches, synthetic vehicle only, explicitly isolated test world. */
public final class GroveCargoAcceptance {
    private static boolean finished;
    private static void require(boolean ok,String message){if(!ok)throw new IllegalStateException(message);}
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.cargoAcceptance"))return;
        require(LibertyCraft.WORLD_NAME.equals("GroveCraftGtaTestCargoAcceptance"),"Isolated cargo world required");
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(finished||server.getTickCount()<100||server.getPlayerList().getPlayers().isEmpty())return;
            var player=server.getPlayerList().getPlayers().getFirst();
            if(!GroveCargo.eligible(player,1))return;
            finished=true;
            Path marker=server.getWorldPath(LevelResource.ROOT).resolve("cargo-acceptance-marker");
            Path report=FabricLoader.getInstance().getGameDir().resolve("grovecraft-cargo-acceptance.json");
            boolean reload=Files.exists(marker);
            try{
                require(!GroveCargo.open(player,999),"Unknown vehicle rejected");
                require(GroveCargo.open(player,1),"Real vanilla menu opened");
                var menu=(ChestMenu)player.containerMenu;
                require(menu.getRowCount()==3,"27 cargo slots");
                if(!reload){
                    require(menu.getContainer().isEmpty(),"Cargo starts empty");
                    player.getInventory().setItem(0,new ItemStack(Items.OAK_LOG,12)); // Explicit test fixture, never production.
                    menu.quickMoveStack(player,54);
                    require(player.getInventory().getItem(0).isEmpty(),"Deposit removes player stack");
                }
                require(menu.getContainer().getItem(0).is(Items.OAK_LOG)&&menu.getContainer().getItem(0).getCount()==12,"Stored wood survives reopen/reload");
                menu.quickMoveStack(player,0);
                require(menu.getContainer().isEmpty(),"Withdrawal removes cargo stack");
                int wood=0,slot=-1;
                for(int i=0;i<36;i++)if(player.getInventory().getItem(i).is(Items.OAK_LOG)){wood+=player.getInventory().getItem(i).getCount();slot=i;}
                require(wood==12,"No duplication during vanilla shift transfer");
                // Chest menu indexes: cargo27, inventory main27, hotbar9.
                menu.quickMoveStack(player,slot<9?54+slot:27+slot-9);
                player.closeContainer();
                require(GroveCargo.open(player,1),"Reopen same luggage");
                require(((ChestMenu)player.containerMenu).getContainer().getItem(0).getCount()==12,"One authoritative inventory");
                player.closeContainer();
                Files.writeString(marker,"12 oak logs stored via vanilla menu");
                Files.writeString(report,"{\"passed\":true,\"real_minecraft\":true,\"synthetic_vehicle\":true,\"reload\":"+reload+",\"cargo_slots\":27,\"wood\":12}");
            }catch(Throwable e){
                LibertyCraft.LOG.error("[GroveCraft] Cargo acceptance failed",e);
                try{Files.writeString(report,"{\"passed\":false,\"error\":"+new com.google.gson.Gson().toJson(e.toString())+"}");}catch(Exception ignored){}
            }
        });
    }
}
