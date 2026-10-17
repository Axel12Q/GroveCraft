package dev.libertycraft;
import java.util.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerEntityEvents;
import net.minecraft.core.GlobalPos;
import net.minecraft.core.BlockPos;
import net.minecraft.world.entity.ai.memory.MemoryModuleType;
import net.minecraft.world.entity.npc.villager.*;
import net.minecraft.world.item.*;
import net.minecraft.world.item.trading.*;
import net.minecraft.server.level.ServerLevel;

/** Resource economy through actual vanilla merchant slots, stock, demand and saves.
 * Existing villagers retain their profession, original offers and their recorded uses. */
public final class GroveVillageEconomy {
    private GroveVillageEconomy(){}
    public static final String MARKET="grovecraft_market_v1";
    private static final Map<UUID,Long> observedWork=new LinkedHashMap<>();
    public static void observeWork(Villager v,long time){
        if(!(Boolean.getBoolean("grovecraft.runtimeAcceptance")||Boolean.getBoolean("grovecraft.marketWorkAcceptance"))||!v.entityTags().contains("grovecraft_settler"))return;
        observedWork.put(v.getUUID(),time);if(observedWork.size()>256)observedWork.remove(observedWork.keySet().iterator().next());
    }
    public static long observedWork(Villager v){return observedWork.getOrDefault(v.getUUID(),-1L);}
    public static void register(){
        net.fabricmc.fabric.api.event.lifecycle.v1.ServerLifecycleEvents.SERVER_STARTED.register(s->observedWork.clear());
        ServerEntityEvents.ENTITY_LOAD.register((entity,level)->{
            if(entity instanceof Villager v&&v.entityTags().contains("grovecraft_settler"))install(v);
        });
        net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents.END_SERVER_TICK.register(server->{
            if(server.getTickCount()%100!=0||server.getPlayerList().getPlayers().isEmpty())return;
            var p=server.getPlayerList().getPlayers().getFirst();
            for(var v:p.level().getEntitiesOfClass(Villager.class,p.getBoundingBox().inflate(192),
                v->v.entityTags().contains("grovecraft_settler")))install(v);
        });
    }
    public static void establish(Villager v,ServerLevel level,boolean farmer,BlockPos work){
        v.setVillagerData(v.getVillagerData().withProfession(level.registryAccess(),farmer?VillagerProfession.FARMER:VillagerProfession.TOOLSMITH));
        v.getBrain().setMemory(MemoryModuleType.JOB_SITE,GlobalPos.of(level.dimension(),work));
        // Setting the profession alone does not rebuild vanilla's work activities.
        level.getPoiManager().ensureLoadedAndValid(level,work,1);
        level.getPoiManager().take(v.getVillagerData().profession().value().heldJobSite(),(type,pos)->pos.equals(work),work,1);
        v.refreshBrain(level);
        v.addTag(farmer?"grovecraft_market_farmer":"grovecraft_market_supplier");
        rememberWork(v,work);
        v.setCustomName(net.minecraft.network.chat.Component.literal(farmer?"Agricultor":"Proveedor"));
        install(v);
    }
    public static void install(Villager v){
        restoreWork(v);
        GroveEcology.repairNaturalMarketAccess(v);
        if(v.getVillagerData().profession().is(VillagerProfession.NONE)||v.getVillagerData().profession().is(VillagerProfession.NITWIT))return;
        var offers=v.getOffers();boolean farmer=v.entityTags().contains("grovecraft_market_farmer");
        boolean general=!farmer&&!v.entityTags().contains("grovecraft_market_supplier");
        if(farmer||general){
            buy(offers,Items.WHEAT,20,12);buy(offers,Items.CARROT,24,8);buy(offers,Items.POTATO,24,8);
            sell(offers,Items.BREAD,4,1,8);sell(offers,Items.COOKED_BEEF,2,2,4);
            sell(offers,Items.WHEAT_SEEDS,8,1,6);
        }
        if(!farmer||general){
            buy(offers,Items.OAK_LOG,16,8);buy(offers,Items.BIRCH_LOG,16,8);buy(offers,Items.COAL,12,8);
            buy(offers,Items.IRON_INGOT,6,4);
            sell(offers,Items.IRON_PICKAXE,1,6,2);sell(offers,Items.IRON_AXE,1,5,2);
            sell(offers,Items.SHIELD,1,4,2);sell(offers,Items.ARROW,16,2,6);
            sell(offers,Items.TORCH,12,1,8);sell(offers,Items.BUCKET,1,3,3);
        }
        v.addTag(MARKET);
    }
    static void moveWork(Villager v,ServerLevel level,BlockPos work){
        v.releasePoi(MemoryModuleType.JOB_SITE);v.getBrain().setMemory(MemoryModuleType.JOB_SITE,GlobalPos.of(level.dimension(),work));
        level.getPoiManager().ensureLoadedAndValid(level,work,1);
        level.getPoiManager().take(v.getVillagerData().profession().value().heldJobSite(),(type,pos)->pos.equals(work),work,1);
        rememberWork(v,work);v.refreshBrain(level);
    }
    private static void rememberWork(Villager v,BlockPos work){
        v.entityTags().removeIf(t->t.startsWith("grovecraft_job:"));
        v.addTag("grovecraft_job:"+work.getX()+":"+work.getY()+":"+work.getZ());
    }
    /** Reclaim an intact assigned workstation after terrain/chunk streaming made
     * vanilla forget it. Missing/occupied stations never trigger stock resets. */
    private static void restoreWork(Villager v){
        boolean farmer=v.entityTags().contains("grovecraft_market_farmer"),supplier=v.entityTags().contains("grovecraft_market_supplier");
        if(!farmer&&!supplier)return;var level=(ServerLevel)v.level();
        var memory=v.getBrain().getMemory(MemoryModuleType.JOB_SITE);
        if(memory.isPresent()&&memory.get().dimension()==level.dimension()){
            var pos=memory.get().pos();if(level.hasChunkAt(pos)&&level.getBlockState(pos).is(farmer?net.minecraft.world.level.block.Blocks.COMPOSTER:net.minecraft.world.level.block.Blocks.SMITHING_TABLE))rememberWork(v,pos);
            return;
        }
        BlockPos work=null;
        for(var tag:v.entityTags())if(tag.startsWith("grovecraft_job:"))try{
            var xyz=tag.substring(15).split(":");if(xyz.length==3)work=new BlockPos(Integer.parseInt(xyz[0]),Integer.parseInt(xyz[1]),Integer.parseInt(xyz[2]));
        }catch(NumberFormatException ignored){}
        if(work==null&&supplier)work=GroveEcology.naturalDepotWorkplace(v);
        if(work==null||!level.hasChunkAt(work)||!level.getBlockState(work).is(farmer?net.minecraft.world.level.block.Blocks.COMPOSTER:net.minecraft.world.level.block.Blocks.SMITHING_TABLE))return;
        var desired=v.getVillagerData().withProfession(level.registryAccess(),farmer?VillagerProfession.FARMER:VillagerProfession.TOOLSMITH);
        var target=work;level.getPoiManager().ensureLoadedAndValid(level,target,1);
        if(level.getPoiManager().take(desired.profession().value().heldJobSite(),(type,pos)->pos.equals(target),target,1).isEmpty())return;
        v.setVillagerData(desired);v.getBrain().setMemory(MemoryModuleType.JOB_SITE,GlobalPos.of(level.dimension(),target));
        rememberWork(v,target);v.refreshBrain(level);
    }
    private static void buy(MerchantOffers offers,Item item,int count,int stock){
        if(offers.stream().anyMatch(o->o.getBaseCostA().is(item)&&o.getResult().is(Items.EMERALD)&&o.getCostB().isEmpty()))return;
        offers.add(new MerchantOffer(new ItemCost(item,count),new ItemStack(Items.EMERALD),stock,2,.05f));
    }
    private static void sell(MerchantOffers offers,Item item,int count,int price,int stock){
        if(offers.stream().anyMatch(o->o.getBaseCostA().is(Items.EMERALD)&&o.getResult().is(item)&&o.getCostB().isEmpty()))return;
        offers.add(new MerchantOffer(new ItemCost(Items.EMERALD,price),new ItemStack(item,count),stock,2,.05f));
    }
}
