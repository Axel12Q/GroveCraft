package dev.libertycraft;
import dev.libertycraft.link.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.core.*;
import net.minecraft.world.*;
import net.minecraft.world.entity.*;
import net.minecraft.world.entity.item.ItemEntity;
import net.minecraft.world.item.*;
import net.minecraft.world.level.block.*;
import net.minecraft.world.level.block.state.properties.BedPart;
import net.minecraft.world.phys.*;
import java.nio.file.*;
/** Isolated real bed interaction, death/recovery and saved respawn oracle. */
public final class GroveBedAcceptance {
    private static int stage,ticks;private static boolean slept,death;private static BlockPos head;
    private static void check(boolean ok,String why){if(!ok)throw new IllegalStateException(why);}
    private static void report(String status,boolean passed,net.minecraft.server.level.ServerPlayer p,Link.SkyState sky)throws Exception{
        var j=new com.google.gson.JsonObject();j.addProperty("status",status);j.addProperty("passed",passed);j.addProperty("stage",stage);
        j.addProperty("sleeping",p.isSleeping());j.addProperty("hour",sky.gameHour);j.addProperty("flags",sky.flags);
        j.add("bed",new com.google.gson.Gson().toJsonTree(GroveBeds.saved(p)));j.addProperty("inventory_items",count(p));
        var items=new com.google.gson.JsonArray();for(var e:p.level().getEntitiesOfClass(ItemEntity.class,p.getBoundingBox().inflate(128),e->e.entityTags().contains("grovecraft_death_drop")))items.add(e.getItem().toString());j.add("drops",items);
        Files.writeString(FabricLoader.getInstance().getGameDir().resolve("grovecraft-bed-acceptance.json"),j.toString());
    }
    private static int count(net.minecraft.server.level.ServerPlayer p){int n=0;for(var s:p.getInventory().getNonEquipmentItems())n+=s.getCount();for(var slot:EquipmentSlot.values())if(slot!=EquipmentSlot.MAINHAND)n+=p.getItemBySlot(slot).getCount();return n;}
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.bedAcceptance"))return;
        check(LibertyCraft.WORLD_NAME.equals("GroveCraftGtaTestBeds20261011"),"Private bed world required");
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(stage>=9||!Link.active()||Link.hostSuspended()||server.getPlayerList().getPlayers().isEmpty())return;
            var p=server.getPlayerList().getPlayers().getFirst();var sky=new Link.SkyState();
            if(!Link.readSkyState(sky)||!sky.inGame()||sky.loading()||sky.menuOpen())return;
            if((sky.flags&(1<<21))!=0){if((sky.flags&(1<<22))!=0)death=true;return;}
            try{
                ticks++;
                if(stage==0){
                    // The tutorial briefly publishes a usable CJ before its taxi scene.
                    if((sky.flags&((1<<16)|(1<<4)))!=0||(sky.flags&(1<<17))==0||Math.abs(sky.x-2248)>20||Math.abs(sky.z-1262)>20)return;
                    if(p.position().distanceToSqr(new Vec3(sky.x,sky.y,sky.z))>4)return;
                    if(Boolean.getBoolean("grovecraft.bedAcceptanceReload")){
                        stage=8;var saved=GroveBeds.saved(p);check(saved!=null,"Saved attachment after reload");
                        var pos=new BlockPos(saved.x(),saved.y(),saved.z());var original=p.level().getBlockState(pos);
                        var other=pos.relative(original.getValue(AbstractBedBlock.PART)==BedPart.HEAD?original.getValue(AbstractBedBlock.FACING).getOpposite():original.getValue(AbstractBedBlock.FACING));
                        if(Boolean.getBoolean("grovecraft.bedAcceptanceRepairFixture")){
                            // Earlier private test flags2 allowed neighbor-shape removal.
                            // Repair that fixture only, then require another clean launch.
                            p.level().setBlock(other,original.setValue(AbstractBedBlock.PART,original.getValue(AbstractBedBlock.PART)==BedPart.HEAD?BedPart.FOOT:BedPart.HEAD),18);
                            report("private_fixture_repaired",true,p,sky);stage=9;return;
                        }
                        check(GroveBeds.valid(p,saved),"Saved bed intact after reload");var otherState=p.level().getBlockState(other);
                        try{p.level().setBlock(pos,Blocks.AIR.defaultBlockState(),18);check(!GroveBeds.valid(p,saved),"Broken bed accepted");}
                        finally{p.level().setBlock(pos,original,18);p.level().setBlock(other,otherState,18);}
                        var stand=BlockPos.containing(saved.sx(),saved.sy(),saved.sz());var previous=p.level().getBlockState(stand);
                        try{p.level().setBlock(stand,Blocks.STONE.defaultBlockState(),18);check(!GroveBeds.valid(p,saved),"Blocked bed exit accepted");}
                        finally{p.level().setBlock(stand,previous,18);}
                        check(GroveBeds.valid(p,saved),"Original bed restored");report("reload_and_invalid_bed_passed",true,p,sky);stage=9;return;
                    }
                    p.getInventory().clearContent();
                    for(var old:p.level().getEntitiesOfClass(ItemEntity.class,p.getBoundingBox().inflate(512),e->e.entityTags().contains("grovecraft_death_drop")))old.discard();
                    int y=(int)Math.ceil(sky.y)+1;var foot=BlockPos.containing(sky.x+2,y,sky.z);
                    for(int x=0;x<2;x++)for(int z=0;z<3;z++)p.level().setBlockAndUpdate(foot.offset(x,-1,z),Blocks.OAK_PLANKS.defaultBlockState());
                    head=foot.south();var bed=net.minecraft.core.registries.BuiltInRegistries.BLOCK.getValue(net.minecraft.resources.Identifier.withDefaultNamespace("red_bed")).defaultBlockState().setValue(AbstractBedBlock.FACING,Direction.SOUTH);
                    p.level().setBlockAndUpdate(foot,bed.setValue(AbstractBedBlock.PART,BedPart.FOOT));p.level().setBlockAndUpdate(head,bed.setValue(AbstractBedBlock.PART,BedPart.HEAD));
                    Link.pushEvent(Proto.EV_SET_TIME,0,20,0,0,0,0);stage=1;ticks=0;
                }else if(stage==1&&sky.gameHour>=19&&ticks>40&&ticks%20==0){
                    var result=p.gameMode.useItemOn(p,p.level(),ItemStack.EMPTY,InteractionHand.MAIN_HAND,new BlockHitResult(Vec3.atCenterOf(head),Direction.UP,head,false));
                    if(p.isSleeping()){stage=2;ticks=0;}
                    else check(ticks<240,"Real bed interaction failed: "+result);
                }else if(stage==2){
                    check(ticks<400,"Sleep did not advance the night");
                    if(p.isSleeping())slept=true;
                    if(slept&&!p.isSleeping()&&sky.gameHour>=5&&sky.gameHour<8){
                        check(GroveBeds.saved(p)!=null&&GroveBeds.valid(p,GroveBeds.saved(p)),"Actual sleep did not persist valid bed");
                        p.getInventory().clearContent();p.getInventory().setItem(0,new ItemStack(Items.OAK_PLANKS,7));p.getInventory().setItem(1,new ItemStack(Items.BREAD,3));
                        p.getInventory().setItem(2,new ItemStack(Items.EMERALD,13));p.setItemSlot(EquipmentSlot.HEAD,new ItemStack(Items.IRON_HELMET));p.setItemSlot(EquipmentSlot.OFFHAND,new ItemStack(Items.SHIELD));
                        check(count(p)==25,"Fixture exact inventory");stage=3;ticks=0;report("sleep_passed_move_then_starve",false,p,sky);
                    }
                }else if(stage==3){
                    var b=GroveBeds.saved(p);if(new Vec3(sky.x,sky.y,sky.z).distanceToSqr(new Vec3(b.sx(),b.sy(),b.sz()))>400){
                        p.getFoodData().setFoodLevel(0);p.getFoodData().setSaturation(0);stage=4;ticks=0;
                    }
                }else if(stage==4){
                    if(!death){p.getFoodData().setFoodLevel(0);p.getFoodData().setSaturation(0);if(ticks%10==0)Link.pushEvent(41,0,1,0,0,0,0);}
                    else {stage=5;ticks=0;}
                }else if(stage==5&&(sky.flags&((1<<21)|(1<<24)))==0){
                    var b=GroveBeds.saved(p);if(new Vec3(sky.x,sky.y,sky.z).distanceToSqr(new Vec3(b.sx(),b.sy(),b.sz()))<9){
                        check(count(p)==0,"Death inventory retained/duplicated");stage=6;ticks=0;report("native_bed_respawn_passed_return_for_drops",false,p,sky);
                    }
                }else if(stage==6&&count(p)==25){check(p.getItemBySlot(EquipmentSlot.HEAD).isEmpty(),"Recovered helmet is an item, not auto-equipped");report("passed",true,p,sky);stage=9;}
                if(ticks%20==0&&stage!=9)report("waiting",false,p,sky);
            }catch(Throwable e){stage=9;LibertyCraft.LOG.error("Bed acceptance",e);try{Files.writeString(FabricLoader.getInstance().getGameDir().resolve("grovecraft-bed-acceptance.json"),"{\"passed\":false,\"error\":"+new com.google.gson.Gson().toJson(e.toString())+"}");}catch(Exception ignored){}}
        });
    }
}
