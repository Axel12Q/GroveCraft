package dev.libertycraft;
import java.nio.file.*;
import java.util.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.core.BlockPos;
import net.minecraft.world.entity.npc.villager.*;
import net.minecraft.world.inventory.MerchantMenu;
import net.minecraft.world.item.*;
import net.minecraft.world.level.block.entity.BarrelBlockEntity;
import net.minecraft.world.level.storage.LevelResource;
/** Real vanilla trade/container save cycle; explicit synthetic-ground isolated fixture. */
public final class GroveEconomyAcceptance {
    private static boolean done;
    private static void require(boolean ok,String reason){if(!ok)throw new IllegalStateException(reason);}
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.economyAcceptance"))return;
        require(LibertyCraft.WORLD_NAME.equals("GroveCraftGtaTestEconomyAcceptance"),"Isolated economy world required");
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(done||server.getTickCount()<100||server.getPlayerList().getPlayers().isEmpty())return;
            done=true;var player=server.getPlayerList().getPlayers().getFirst();var level=player.level();
            var site=new GroveEcologyRules.Site("economy:hamlet",2528,1720,GroveEcologyRules.Kind.HAMLET,37);
            var pos=new BlockPos(2524,21,1717);
            Path marker=server.getWorldPath(LevelResource.ROOT).resolve("economy-test-marker");
            Path report=FabricLoader.getInstance().getGameDir().resolve("grovecraft-economy-acceptance.json");
            boolean reload=Files.exists(marker);
            try{
                var flat=new GroveEcology.Terrain(){public double grass(int x,int z,double y){return 20.;}public boolean known(int x,int y,int z){return true;}public boolean occupied(BlockPos p){return false;}};
                if(!reload){
                    require(GroveEcology.generate(level,site,20,flat).equals("generated"),"Hamlet generated");
                    var villagers=level.getEntitiesOfClass(Villager.class,new net.minecraft.world.phys.AABB(2515,19,1705,2542,29,1735));
                    require(villagers.size()==2,"Two actual villagers");var villager=villagers.getFirst();
                    // Assign profession only in this fixture; native-field POI discovery is separate.
                    villager.setVillagerData(villager.getVillagerData().withProfession(level.registryAccess(),VillagerProfession.FARMER));
                    var offers=villager.getOffers();int index=-1;
                    for(int i=0;i<offers.size();i++)if(offers.get(i).getResult().is(Items.EMERALD)&&offers.get(i).getCostB().isEmpty()){index=i;break;}
                    require(index>=0,"Vanilla farmer buys crop for emerald");var offer=offers.get(index);int cost=offer.getCostA().getCount();
                    for(int i=0;i<36;i++)player.getInventory().setItem(i,ItemStack.EMPTY);
                    player.getInventory().setItem(0,offer.getCostA().copy());
                    villager.setTradingPlayer(player);villager.openTradingScreen(player,net.minecraft.network.chat.Component.literal("Settlement trade test"),1);
                    require(player.containerMenu instanceof MerchantMenu,"Real merchant menu");var menu=(MerchantMenu)player.containerMenu;
                    menu.setSelectionHint(index);menu.tryMoveItems(index);require(!menu.getSlot(2).getItem().isEmpty(),"Trade output available");
                    menu.quickMoveStack(player,2);require(offer.getUses()==1,"One trade counted");player.closeContainer();
                    int emeralds=0,slot=-1,remaining=0;
                    for(int i=0;i<36;i++){var item=player.getInventory().getItem(i);if(item.is(Items.EMERALD)){emeralds+=item.getCount();slot=i;}else if(!item.isEmpty())remaining+=item.getCount();}
                    require(emeralds==offer.getResult().getCount()&&remaining==0,"Exact payment consumed, no free output");
                    var barrel=(BarrelBlockEntity)level.getBlockEntity(pos);require(barrel!=null,"House barrel");
                    // Storage transfer is deliberate fixture code; trade itself used real menu slots.
                    barrel.setItem(11,player.getInventory().getItem(slot).copy());barrel.setChanged();player.getInventory().setItem(slot,ItemStack.EMPTY);
                    Files.writeString(marker,villager.getUUID()+"\n"+index+"\n"+cost+"\n"+emeralds+"\n");
                    LibertyCraft.LOG.info("[GroveCraft] economy acceptance paid {} {} -> {} emerald(s), uses1",cost,offer.getBaseCostA().getItem(),emeralds);
                }else{
                    var lines=Files.readAllLines(marker);var villager=(Villager)level.getEntity(UUID.fromString(lines.get(0)));
                    require(villager!=null,"Same villager UUID restored");require(villager.getOffers().get(Integer.parseInt(lines.get(1))).getUses()==1,"Trade uses persisted");
                    var barrel=(BarrelBlockEntity)level.getBlockEntity(pos);require(barrel!=null&&barrel.getItem(11).is(Items.EMERALD)&&barrel.getItem(11).getCount()==Integer.parseInt(lines.get(3)),"Stored proceeds persisted exactly");
                    require(GroveEcology.generate(level,site,20,flat).equals("visited"),"No structure/loot regeneration");
                }
                Files.writeString(report,"{\"passed\":true,\"real_minecraft\":true,\"synthetic_ground\":true,\"assigned_test_profession\":true,\"reload\":"+reload+",\"gta_cash_link\":false}");
            }catch(Throwable e){LibertyCraft.LOG.error("[GroveCraft] Economy acceptance failed",e);try{Files.writeString(report,"{\"passed\":false,\"error\":"+new com.google.gson.Gson().toJson(e.toString())+"}");}catch(Exception ignored){}}
        });
    }
}
