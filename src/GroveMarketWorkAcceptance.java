package dev.libertycraft;
import java.nio.file.*;
import java.util.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.world.entity.npc.villager.Villager;
import net.minecraft.world.entity.ai.memory.MemoryModuleType;
import net.minecraft.world.item.*;
import net.minecraft.world.inventory.MerchantMenu;
import dev.libertycraft.link.Link;

/** Opt-in paid transaction followed by unassisted vanilla work/restocking on
 * an existing naturally generated GTA parcel. Never calls restock or moves a villager. */
public final class GroveMarketWorkAcceptance {
    private static UUID merchant;
    private static int trade=-1,startTick,workTicks;
    private static long initialWork;
    private static boolean finished,paidThisSession;
    private static void check(boolean ok,String why){if(!ok)throw new IllegalStateException(why);}
    private static long work(net.minecraft.world.entity.npc.villager.Villager v){
        var m=net.minecraft.world.entity.ai.memory.MemoryModuleType.LAST_WORKED_AT_POI;
        long remembered=v.getBrain().checkMemory(m,net.minecraft.world.entity.ai.memory.MemoryStatus.REGISTERED)?v.getBrain().getMemory(m).orElse(-1L):-1L;
        return Math.max(remembered,GroveVillageEconomy.observedWork(v));
    }
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.marketWorkAcceptance"))return;
        check(LibertyCraft.WORLD_NAME.equals("GroveCraftGtaTestMarketsPriority20261011"),"Exact private field world required");
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(finished||server.getPlayerList().getPlayers().isEmpty())return;
            var p=server.getPlayerList().getPlayers().getFirst();var level=p.level();var sky=new Link.SkyState();
            if(!Link.active()||Link.hostSuspended()||!Link.readSkyState(sky)||!sky.inGame()||sky.loading())return;
            // Initial saved player coordinates can precede the first host synchronization.
            if(sky.menuOpen()||sky.scene()||Math.hypot(p.getX()-sky.x,p.getZ()-sky.z)>4)return;
            try{
                if(merchant==null){
                    var nearby=level.getEntitiesOfClass(Villager.class,p.getBoundingBox().inflate(32),
                        v->v.entityTags().contains("grovecraft_market_supplier")&&v.getBrain().getMemory(MemoryModuleType.JOB_SITE).isPresent());
                    if(nearby.isEmpty())return;var v=nearby.getFirst();
                    for(int i=0;i<v.getOffers().size();i++){var o=v.getOffers().get(i);
                        if(o.getBaseCostA().is(Items.EMERALD)&&o.getResult().is(Items.IRON_PICKAXE)&&o.getCostB().isEmpty()){trade=i;break;}}
                    check(trade>=0,"Supplier pick offer missing");var offer=v.getOffers().get(trade);
                    check(offer.getMaxUses()==2,"Finite supplier stock expected");
                    if(offer.isOutOfStock()&&offer.getUses()==2){
                        // Resume the previous paid test after normal save/unload; never reset uses.
                        merchant=v.getUUID();initialWork=work(v);startTick=server.getTickCount();
                        report("waiting_for_vanilla_work",false,v,0);return;
                    }
                    check(offer.getUses()==0,"Fresh stock or previously exhausted test expected");
                    // Only this private fixture gets payment; all production worlds are excluded above.
                    for(int i=0;i<36;i++)p.getInventory().setItem(i,ItemStack.EMPTY);
                    for(int i=0;i<2;i++){
                        p.getInventory().add(offer.getCostA().copy());v.setTradingPlayer(p);
                        v.openTradingScreen(p,net.minecraft.network.chat.Component.literal("Field work test"),1);
                        check(p.containerMenu instanceof MerchantMenu,"Real merchant menu");var menu=(MerchantMenu)p.containerMenu;
                        menu.setSelectionHint(trade);menu.tryMoveItems(trade);check(!menu.getSlot(2).getItem().isEmpty(),"Paid output missing");
                        menu.quickMoveStack(p,2);p.closeContainer();
                    }
                    check(offer.isOutOfStock()&&offer.getUses()==2,"Two paid picks must exhaust stock");
                    int picks=0,emeralds=0;for(int i=0;i<36;i++){var item=p.getInventory().getItem(i);if(item.is(Items.IRON_PICKAXE))picks+=item.getCount();if(item.is(Items.EMERALD))emeralds+=item.getCount();}
                    check(picks==2&&emeralds==0,"Exact paid inventory");
                    paidThisSession=true;
                    merchant=v.getUUID();initialWork=work(v);startTick=server.getTickCount();
                    report("waiting_for_vanilla_work",false,v,0);return;
                }
                workTicks++;
                var v=(Villager)level.getEntity(merchant);if(v==null)return;
                check(v.isAlive(),"Merchant died during work test");
                long work=work(v);
                if(v.getOffers().get(trade).getUses()==0){
                    check(work>initialWork,"Stock reset must follow actual vanilla work");
                    report("passed",true,v,server.getTickCount()-startTick);finished=true;
                }else if(workTicks>2400){report("work_timeout",false,v,server.getTickCount()-startTick);finished=true;}
                else if(workTicks%100==0)report("waiting_for_vanilla_work",false,v,server.getTickCount()-startTick);
            }catch(Throwable e){finished=true;LibertyCraft.LOG.error("Field market work acceptance",e);
                try{Files.writeString(FabricLoader.getInstance().getGameDir().resolve("grovecraft-market-work-acceptance.json"),"{\"passed\":false,\"error\":"+new com.google.gson.Gson().toJson(e.toString())+"}");}catch(Exception ignored){}}
        });
    }
    private static void report(String status,boolean passed,Villager v,int elapsed) throws Exception{
        var j=new com.google.gson.JsonObject();j.addProperty("status",status);j.addProperty("passed",passed);j.addProperty("real_gta_ground",true);
        j.addProperty("merchant",v.getUUID().toString());j.addProperty("elapsed_ticks",elapsed);
        j.addProperty("paid_this_session",paidThisSession);
        j.addProperty("uses",v.getOffers().get(trade).getUses());j.addProperty("initial_work",initialWork);
        j.addProperty("last_work",work(v));
        j.addProperty("job",v.getBrain().getMemory(MemoryModuleType.JOB_SITE).map(Object::toString).orElse("absent"));
        j.addProperty("x",v.getX());j.addProperty("y",v.getY());j.addProperty("z",v.getZ());
        Files.writeString(FabricLoader.getInstance().getGameDir().resolve("grovecraft-market-work-acceptance.json"),j.toString());
    }
}
