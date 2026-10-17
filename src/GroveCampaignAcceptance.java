package dev.libertycraft;
import dev.libertycraft.link.Link;
import net.fabricmc.fabric.api.event.lifecycle.v1.*;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.entity.*;
import net.minecraft.world.entity.npc.villager.*;
import net.minecraft.world.item.*;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.inventory.ContainerInput;
import net.minecraft.core.BlockPos;
import net.minecraft.world.phys.Vec3;
import java.nio.file.*;
/** Explicit private paired-save fixture. Native saves use a separate GTA directory. */
public final class GroveCampaignAcceptance {
    private static boolean initialized;private static Villager trader;private static BlockPos marker;
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.campaignAcceptance"))return;
        if(!"GroveCraftGtaTestCampaignAcceptance".equals(System.getProperty("grovecraft.testWorld")))throw new IllegalStateException("Private campaign fixture required");
        var root=Path.of(System.getenv("GROVECRAFT_CAMPAIGN_ROOT"));if(!root.getFileName().toString().equals("pair-test-data"))throw new IllegalStateException("Private campaign directory required");
        ServerLifecycleEvents.SERVER_STARTED.register(s->{initialized=false;trader=null;marker=null;});
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(!Link.active()||Link.hostSuspended()||server.getPlayerList().getPlayers().isEmpty())return;
            var p=server.getPlayerList().getPlayers().getFirst();var sky=new Link.SkyState();
            if(!Link.readSkyState(sky)||!sky.inGame()||sky.loading()||sky.menuOpen()||(sky.flags&((1<<16)|(1<<21)|(1<<22)|(1<<4)))!=0)return;
            if(p.position().distanceToSqr(new Vec3(sky.x,sky.y,sky.z))>1||(sky.flags&(1<<17))==0)return;
            try{
                if(!initialized){
                    if(!p.entityTags().contains("grovecraft_campaign_fixture")){
                        p.getInventory().clearContent();p.getInventory().setItem(9,new ItemStack(Items.OAK_LOG,23));
                        p.getInventory().setItem(10,new ItemStack(Items.DIAMOND,17));p.getInventory().setItem(11,new ItemStack(Items.BREAD,11));
                        p.addTag("grovecraft_campaign_fixture");marker=p.blockPosition().offset(6,2,0);
                        p.addTag("grovecraft_campaign_marker:"+marker.getX()+":"+marker.getY()+":"+marker.getZ());p.level().setBlock(marker,Blocks.OAK_PLANKS.defaultBlockState(),3);
                    }else for(var tag:p.entityTags())if(tag.startsWith("grovecraft_campaign_marker:")){
                        var xyz=tag.substring("grovecraft_campaign_marker:".length()).split(":");marker=new BlockPos(Integer.parseInt(xyz[0]),Integer.parseInt(xyz[1]),Integer.parseInt(xyz[2]));}
                    trader=p.level().getEntitiesOfClass(Villager.class,p.getBoundingBox().inflate(16),v->v.entityTags().contains("grovecraft_campaign_trader")).stream().findFirst().orElse(null);
                    if(trader==null){trader=EntityTypes.VILLAGER.create(p.level(),EntitySpawnReason.COMMAND);trader.setPos(p.getX(),p.getY(),p.getZ()-1.8);
                        trader.setNoAi(true);trader.setNoGravity(true);trader.setPersistenceRequired();trader.addTag("grovecraft_settler");trader.addTag("grovecraft_campaign_trader");
                        trader.setVillagerData(trader.getVillagerData().withProfession(p.level().registryAccess(),VillagerProfession.FARMER));p.level().addFreshEntity(trader);}
                    initialized=true;
                }
                var command=root.resolve("test-edit.txt");if(Files.exists(command)){
                    String action=Files.readString(command).trim();Files.delete(command);
                    if(action.equals("edit")){p.getInventory().setItem(10,new ItemStack(Items.DIAMOND,63));p.level().setBlock(marker,Blocks.STONE.defaultBlockState(),3);}
                    else if(action.equals("sell")||action.equals("buy")){
                        if(!GroveTownEconomy.open(p,trader))throw new IllegalStateException("Eligible fixture trader");
                        p.containerMenu.clicked(action.equals("sell")?2:11,0,ContainerInput.PICKUP,p);
                    }
                }
                if(server.getTickCount()%10==0){
                    var j=new com.google.gson.JsonObject();j.addProperty("world",LibertyCraft.WORLD_NAME);j.addProperty("logs",GroveTownEconomy.count(p,Items.OAK_LOG));
                    j.addProperty("diamonds",GroveTownEconomy.count(p,Items.DIAMOND));j.addProperty("bread",GroveTownEconomy.count(p,Items.BREAD));
                    var state=GroveTownEconomy.state(p);j.addProperty("next",state.next);j.addProperty("token",state.token);j.addProperty("pending",state.pending!=null);j.addProperty("job",state.job!=null);
                    j.addProperty("cash",GroveTownEconomy.observedCash());j.addProperty("wood",p.level().getBlockState(marker).is(Blocks.OAK_PLANKS));j.addProperty("stone",p.level().getBlockState(marker).is(Blocks.STONE));
                    j.addProperty("health",p.getHealth());GroveCampaignFiles.atomic(root.resolve("field.json"),j.toString());
                }
            }catch(Exception e){LibertyCraft.LOG.error("Campaign fixture",e);}
        });
    }
}
