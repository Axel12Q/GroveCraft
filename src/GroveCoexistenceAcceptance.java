package dev.libertycraft;
import java.nio.file.*;
import java.util.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.core.BlockPos;
import net.minecraft.server.level.*;
import net.minecraft.world.entity.npc.villager.Villager;
import net.minecraft.world.entity.monster.illager.Pillager;
import net.minecraft.world.inventory.MerchantMenu;
import net.minecraft.world.item.*;
import net.minecraft.world.level.block.entity.BarrelBlockEntity;
import net.minecraft.world.level.storage.LevelResource;
import net.minecraft.world.phys.AABB;

/** Real merchant payment/stock and normal save/reload. Synthetic native ground
 * is explicit; this fixture never runs in a user's Lab or Expedition world. */
public final class GroveCoexistenceAcceptance {
    private static boolean done;
    private static void check(boolean ok,String why){if(!ok)throw new IllegalStateException(why);}
    private static int amount(ServerPlayer p,Item item){int n=0;for(int i=0;i<36;i++)if(p.getInventory().getItem(i).is(item))n+=p.getInventory().getItem(i).getCount();return n;}
    private static void trade(ServerPlayer p,Villager v,int index){
        v.setTradingPlayer(p);v.openTradingScreen(p,net.minecraft.network.chat.Component.literal("GroveCraft mercado"),1);
        check(p.containerMenu instanceof MerchantMenu,"Real merchant screen");var menu=(MerchantMenu)p.containerMenu;
        menu.setSelectionHint(index);menu.tryMoveItems(index);check(!menu.getSlot(2).getItem().isEmpty(),"Paid output available");
        menu.quickMoveStack(p,2);p.closeContainer();
    }
    private static int find(Villager v,Item cost,Item result){
        var offers=v.getOffers();for(int i=0;i<offers.size();i++)if(offers.get(i).getBaseCostA().is(cost)&&offers.get(i).getResult().is(result)&&offers.get(i).getCostB().isEmpty())return i;
        throw new IllegalStateException("Missing trade "+cost+" -> "+result);
    }
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.coexistenceAcceptance"))return;
        check(LibertyCraft.WORLD_NAME.equals("GroveCraftGtaTestCoexistenceAcceptance"),"Private world required");
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(done||server.getTickCount()<100||server.getPlayerList().getPlayers().isEmpty())return;done=true;
            var p=server.getPlayerList().getPlayers().getFirst();var level=p.level();
            var village=new GroveEcologyRules.Site("market:hamlet",2528,1720,GroveEcologyRules.Kind.HAMLET,37);
            var outpost=new GroveEcologyRules.Site("market:outpost",2464,1792,GroveEcologyRules.Kind.OUTPOST,42);
            var depot=new GroveEcologyRules.Site("market:waystation",2400,1792,GroveEcologyRules.Kind.WAYSTATION,43);
            var flat=new GroveEcology.Terrain(){public double grass(int x,int z,double y){return 20.;}public boolean known(int x,int y,int z){return true;}public boolean occupied(BlockPos b){return false;}};
            Path marker=server.getWorldPath(LevelResource.ROOT).resolve("coexistence-marker.json");
            Path report=FabricLoader.getInstance().getGameDir().resolve("grovecraft-coexistence-acceptance.json");
            var json=new com.google.gson.Gson();boolean reload=Files.exists(marker);
            try{
                if(!reload){
                    var legacy=net.minecraft.world.entity.EntityTypes.VILLAGER.create(level,net.minecraft.world.entity.EntitySpawnReason.STRUCTURE);
                    check(legacy!=null,"Legacy merchant fixture");
                    legacy.setVillagerData(legacy.getVillagerData().withProfession(level.registryAccess(),net.minecraft.world.entity.npc.villager.VillagerProfession.FISHERMAN));
                    var original=new net.minecraft.world.item.trading.MerchantOffer(new net.minecraft.world.item.trading.ItemCost(Items.COAL,12),new ItemStack(Items.EMERALD),8,2,.05f);
                    original.increaseUses();original.increaseUses();original.increaseUses();
                    var oldOffers=new net.minecraft.world.item.trading.MerchantOffers();oldOffers.add(original);legacy.setOffers(oldOffers);
                    GroveVillageEconomy.install(legacy);find(legacy,Items.WHEAT,Items.EMERALD);find(legacy,Items.EMERALD,Items.IRON_PICKAXE);
                    check(legacy.getOffers().getFirst()==original&&original.getUses()==3,"Legacy profession/offers/uses preserved");
                    int oldCount=legacy.getOffers().size();GroveVillageEconomy.install(legacy);check(legacy.getOffers().size()==oldCount,"Legacy upgrade once");
                    check(GroveEcology.generate(level,village,20,flat).equals("generated"),"Village generation");
                    var villagers=level.getEntitiesOfClass(Villager.class,new AABB(2515,19,1705,2542,29,1735));
                    check(villagers.size()==2,"Two village jobs");
                    var farmer=villagers.stream().filter(v->v.entityTags().contains("grovecraft_market_farmer")).findFirst().orElseThrow();
                    var supplier=villagers.stream().filter(v->v.entityTags().contains("grovecraft_market_supplier")).findFirst().orElseThrow();
                    check(farmer.getBrain().getMemory(net.minecraft.world.entity.ai.memory.MemoryModuleType.JOB_SITE).isPresent(),"Real job site memory");
                    int count=supplier.getOffers().size();GroveVillageEconomy.install(supplier);check(supplier.getOffers().size()==count,"No duplicate offers");
                    int crop=find(farmer,Items.WHEAT,Items.EMERALD),pick=find(supplier,Items.EMERALD,Items.IRON_PICKAXE);
                    var offer=supplier.getOffers().get(pick);int price=offer.getCostA().getCount();
                    for(int i=0;i<36;i++)p.getInventory().setItem(i,ItemStack.EMPTY);
                    p.getInventory().setItem(0,new ItemStack(Items.EMERALD,price-1));
                    supplier.setTradingPlayer(p);supplier.openTradingScreen(p,net.minecraft.network.chat.Component.literal("Underpayment"),1);
                    var menu=(MerchantMenu)p.containerMenu;menu.setSelectionHint(pick);menu.tryMoveItems(pick);
                    check(menu.getSlot(2).getItem().isEmpty(),"Underpayment has no output");p.closeContainer();
                    check(amount(p,Items.EMERALD)==price-1,"Underpayment not consumed");p.getInventory().setItem(0,ItemStack.EMPTY);
                    int wheatPaid=0;
                    for(int purchase=0;purchase<2;purchase++){
                        int needed=offer.getCostA().getCount();
                        for(int n=0;n<needed;n++){
                            var cropOffer=farmer.getOffers().get(crop);check(!cropOffer.isOutOfStock(),"Finite crop budget enough");
                            int cost=cropOffer.getCostA().getCount();wheatPaid+=cost;
                            p.getInventory().add(cropOffer.getCostA().copy());trade(p,farmer,crop);
                        }
                        trade(p,supplier,pick);
                    }
                    check(amount(p,Items.IRON_PICKAXE)==2&&amount(p,Items.EMERALD)==0&&amount(p,Items.WHEAT)==0,"Production -> currency -> two tools, exact payment");
                    check(offer.getUses()==2&&offer.isOutOfStock(),"Supplier stock exhausted");
                    var profession=supplier.getVillagerData();
                    supplier.setVillagerData(profession.withProfession(level.registryAccess(),net.minecraft.world.entity.npc.villager.VillagerProfession.NONE));
                    supplier.setVillagerData(profession);supplier.refreshBrain(level);GroveVillageEconomy.install(supplier);
                    check(supplier.getOffers().get(pick)==offer&&offer.getUses()==2&&offer.isOutOfStock(),"Changing jobs preserves depleted market stock");
                    var workplace=supplier.getBrain().getMemory(net.minecraft.world.entity.ai.memory.MemoryModuleType.JOB_SITE).orElseThrow();
                    supplier.releasePoi(net.minecraft.world.entity.ai.memory.MemoryModuleType.JOB_SITE);
                    supplier.getBrain().eraseMemory(net.minecraft.world.entity.ai.memory.MemoryModuleType.JOB_SITE);
                    GroveVillageEconomy.install(supplier);
                    check(supplier.getBrain().getMemory(net.minecraft.world.entity.ai.memory.MemoryModuleType.JOB_SITE).orElseThrow().equals(workplace)
                        &&supplier.getOffers().get(pick)==offer&&offer.getUses()==2,"Reclaiming an intact job preserves depleted stock");
                    supplier.setTradingPlayer(p);supplier.openTradingScreen(p,net.minecraft.network.chat.Component.literal("Sold out"),1);
                    menu=(MerchantMenu)p.containerMenu;menu.setSelectionHint(pick);menu.tryMoveItems(pick);
                    check(menu.getSlot(2).getItem().isEmpty(),"No free third tool");p.closeContainer();
                    var store=(BarrelBlockEntity)level.getBlockEntity(new BlockPos(2524,21,1717));check(store!=null,"Real store");
                    int slot=11;for(int i=0;i<36;i++)if(p.getInventory().getItem(i).is(Items.IRON_PICKAXE)){
                        store.setItem(slot++,p.getInventory().getItem(i).copy());p.getInventory().setItem(i,ItemStack.EMPTY);
                    }store.setChanged();
                    check(GroveEcology.generate(level,outpost,20,flat).equals("generated"),"Outpost plan");
                    var guards=level.getEntitiesOfClass(Pillager.class,new AABB(2455,19,1783,2474,33,1802));
                    check(guards.size()==3,"Three finite real pillagers");guards.getFirst().discard();
                    var loot=(BarrelBlockEntity)level.getBlockEntity(new BlockPos(2462,21,1790));
                    check(loot!=null&&!loot.isEmpty(),"Finite outpost loot");loot.clearContent();loot.setChanged();
                    check(GroveEcology.generate(level,outpost,20,flat).equals("visited"),"No immediate loot/enemy refill");
                    check(GroveEcology.generate(level,depot,20,flat).equals("generated"),"Waystation plan");
                    check(level.getEntitiesOfClass(Villager.class,new AABB(2395,19,1787,2406,28,1798)).size()==1,"One actual waystation supplier");
                    var saved=new com.google.gson.JsonObject();saved.addProperty("farmer",farmer.getUUID().toString());saved.addProperty("supplier",supplier.getUUID().toString());
                    saved.addProperty("pick",pick);saved.addProperty("offers",count);saved.addProperty("crop",crop);saved.addProperty("cropUses",farmer.getOffers().get(crop).getUses());saved.addProperty("wheatPaid",wheatPaid);
                    var ids=new com.google.gson.JsonArray();for(var g:guards)if(!g.isRemoved())ids.add(g.getUUID().toString());saved.add("guards",ids);
                    Files.writeString(marker,json.toJson(saved));
                }else{
                    var saved=json.fromJson(Files.readString(marker),com.google.gson.JsonObject.class);
                    var supplier=(Villager)level.getEntity(UUID.fromString(saved.get("supplier").getAsString()));
                    var farmer=(Villager)level.getEntity(UUID.fromString(saved.get("farmer").getAsString()));
                    check(supplier!=null&&farmer!=null,"Same merchants restored");
                    check(supplier.getOffers().size()==saved.get("offers").getAsInt(),"No duplicate market after load");
                    var offer=supplier.getOffers().get(saved.get("pick").getAsInt());check(offer.getUses()==2&&offer.isOutOfStock(),"Sold-out stock persists until work/restock");
                    check(farmer.getOffers().get(saved.get("crop").getAsInt()).getUses()==saved.get("cropUses").getAsInt(),"Farmer trade history persists");
                    var store=(BarrelBlockEntity)level.getBlockEntity(new BlockPos(2524,21,1717));
                    check(store!=null&&store.getItem(11).is(Items.IRON_PICKAXE)&&store.getItem(12).is(Items.IRON_PICKAXE),"Both stored tools persist");
                    var loot=(BarrelBlockEntity)level.getBlockEntity(new BlockPos(2462,21,1790));check(loot!=null&&loot.isEmpty(),"Loot remains empty");
                    for(var id:saved.getAsJsonArray("guards"))check(level.getEntity(UUID.fromString(id.getAsString())) instanceof Pillager,"Saved surviving guard UUID");
                    check(level.getEntitiesOfClass(Pillager.class,new AABB(2455,19,1783,2474,33,1802)).size()==2,"Removed guard did not respawn");
                    for(var s:List.of(village,outpost,depot))check(GroveEcology.generate(level,s,20,flat).equals("visited"),"Persistent structure identity");
                }
                Files.writeString(report,"{\"passed\":true,\"real_minecraft\":true,\"synthetic_ground\":true,\"reload\":"+reload+",\"gta_cash_link\":false,\"finite_stock\":true,\"outpost_persisted\":true}");
            }catch(Throwable e){LibertyCraft.LOG.error("Coexistence acceptance failed",e);try{Files.writeString(report,"{\"passed\":false,\"error\":"+json.toJson(e.toString())+"}");}catch(Exception ignored){}}
        });
    }
}
