package dev.libertycraft;
import dev.libertycraft.link.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.world.item.*;
import net.minecraft.world.inventory.*;
import net.minecraft.world.level.storage.LevelResource;
import java.nio.file.*;
import java.util.*;
/** Private fixture: actual crafting/return-on-close and persistence, never user resources. */
public final class GroveWorkshopAcceptance {
    private static int stage,wait;private static boolean reload,done;private static int vehicle;
    private static void check(boolean ok,String reason){if(!ok)throw new IllegalStateException(reason);}
    private static int count(net.minecraft.server.level.ServerPlayer p,Item item){int n=0;for(int i=0;i<36;i++)if(p.getInventory().getItem(i).is(item))n+=p.getInventory().getItem(i).getCount();return n;}
    private static Path report(){return FabricLoader.getInstance().getGameDir().resolve("grovecraft-workshop-acceptance.json");}
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.workshopAcceptance"))return;
        String world=System.getProperty("grovecraft.testWorld");
        check("GroveCraftGtaTestWorkshopAcceptance".equals(world)||"GroveCraftGtaTestWorkshopField20261013".equals(world),"Exact private workshop world");
        boolean field=world.endsWith("Field20261013");
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(done||!Link.active()||Link.hostSuspended()||server.getPlayerList().getPlayers().isEmpty())return;
            var p=server.getPlayerList().getPlayers().getFirst();var sky=new Link.SkyState();if(!Link.readSkyState(sky)||!GroveCargoRules.safeFlags(sky.flags,true))return;
            var marker=server.getWorldPath(LevelResource.ROOT).resolve("workshop-accepted");
            try{
                if(stage==0){
                    reload=Files.exists(marker);
                    if(reload){check(GroveCargo.hasWorkbench(p)&&count(p,Items.IRON_PICKAXE)==1&&count(p,Items.IRON_INGOT)==2,"Reload exact table/tool/materials");stage=5;}
                    else{p.getInventory().clearContent();p.setAttached(GroveCargo.CARGO,List.of());stage=1;}
                }
                if(stage==1){
                    var actors=new ArrayList<Link.Actor>();long[] stamp={0};Link.readActors(actors,stamp);
                    vehicle=actors.stream().filter(a->(a.formId()&0xff000000)==0x56000000).mapToInt(a->(a.formId()&0x00fffff0)>>>4).filter(id->GroveCargo.eligible(p,id)).findFirst().orElse(0);
                    if(vehicle==0)return;
                    check(!GroveCargo.workshop(p,999999),"Unknown lease refused");check(!GroveCargo.workshop(p,vehicle),"No table refused");
                    p.setAttached(GroveCargo.CARGO,List.of(new ItemStack(Items.CRAFTING_TABLE)));
                    p.getInventory().setItem(9,new ItemStack(Items.IRON_INGOT,3));p.getInventory().setItem(10,new ItemStack(Items.STICK,2));
                    if(!field){GroveGameplay.act(p,26,vehicle,0,0);check(p.containerMenu instanceof CraftingMenu,"Workshop input dispatch opened 3x3");}stage=2;
                }else if(stage==2){
                    if(!(p.containerMenu instanceof CraftingMenu menu))return;
                    if(field&&++wait<100)return; // Time to inspect the real native Alt+G screen.
                    if(field){var actors=new ArrayList<Link.Actor>();long[] stamp={0};Link.readActors(actors,stamp);
                        vehicle=actors.stream().filter(a->(a.formId()&0xff000000)==0x56000000).mapToInt(a->(a.formId()&0x00fffff0)>>>4).filter(id->GroveCargo.eligible(p,id)).findFirst().orElse(0);
                        check(vehicle>0,"Current parked field vehicle");}
                    menu.clicked(10,0,ContainerInput.PICKUP,p);
                    for(int slot:new int[]{1,2,3})menu.clicked(slot,1,ContainerInput.PICKUP,p);
                    menu.clicked(11,0,ContainerInput.PICKUP,p);
                    for(int slot:new int[]{5,8})menu.clicked(slot,1,ContainerInput.PICKUP,p);
                    check(menu.getCarried().isEmpty(),"Recipe cursor empty");stage=3;wait=0;
                }else if(stage==3){
                    var menu=(CraftingMenu)p.containerMenu;
                    if(!menu.getSlot(0).hasItem()){check(++wait<100,"Vanilla recipe result arrived");return;}
                    check(menu.getSlot(0).getItem().is(Items.IRON_PICKAXE),"Actual iron pickaxe recipe");menu.quickMoveStack(p,0);p.closeContainer();
                    check(count(p,Items.IRON_PICKAXE)==1&&count(p,Items.IRON_INGOT)==0&&count(p,Items.STICK)==0,"Exact recipe consumption");
                    p.getInventory().setItem(10,new ItemStack(Items.IRON_INGOT,2));stage=6;wait=0;
                }else if(stage==6){
                    // Native owns-menu flag clears after the close packet is presented.
                    if(!GroveCargoRules.safeFlags(sky.flags,false)){check(++wait<100,"Host acknowledged menu close");return;}
                    check(GroveCargo.workshop(p,vehicle),"Reopen after crafting");
                    var menu=(CraftingMenu)p.containerMenu;menu.getSlot(1).set(p.getInventory().removeItem(10,1));stage=4;wait=0;
                    if(field)p.closeContainer();
                }else if(stage==4){
                    if(p.containerMenu instanceof CraftingMenu){check(++wait<200,"Invalid vehicle closes workshop");}
                    else{check(count(p,Items.IRON_INGOT)==2&&count(p,Items.IRON_PICKAXE)==1,"Partial recipe returned once");check(GroveCargo.hasWorkbench(p),"Table preserved");stage=5;}
                }
                var j=new com.google.gson.JsonObject();j.addProperty("passed",stage==5);j.addProperty("stage",stage);j.addProperty("reload",reload);j.addProperty("synthetic_vehicle",!field);
                j.addProperty("iron",count(p,Items.IRON_INGOT));j.addProperty("picks",count(p,Items.IRON_PICKAXE));j.addProperty("table",GroveCargo.hasWorkbench(p));Files.writeString(report(),j.toString());
                if(stage==5){server.getPlayerList().saveAll();Files.writeString(marker,"one table, one crafted pickaxe, two returned iron ingots");done=true;}
            }catch(Exception e){done=true;LibertyCraft.LOG.error("Workshop fixture",e);try{Files.writeString(report(),"{\"passed\":false,\"error\":"+new com.google.gson.Gson().toJson(e.toString())+"}");}catch(Exception ignored){}}
        });
    }
}
