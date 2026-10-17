package dev.libertycraft;
import dev.libertycraft.link.Link;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.entity.*;
import net.minecraft.world.entity.npc.villager.*;
import net.minecraft.world.item.*;
import net.minecraft.world.inventory.ContainerInput;
import net.minecraft.world.phys.Vec3;
import java.nio.file.*;
import java.util.*;
/** Opt-in real GTA cash fixture, isolated from all user worlds. First sale uses real GUI mouse. */
public final class GroveTownFieldAcceptance {
    private static int stage,ticks,base;private static Villager villager;private static GroveTownEconomy.Board first;
    private static final List<Integer> fillers=new ArrayList<>();private static boolean reload;
    private static void check(boolean ok,String reason){if(!ok)throw new IllegalStateException(reason);}
    private static Path file(String name){return FabricLoader.getInstance().getGameDir().resolve(name);}
    private static void report(ServerPlayer p,boolean passed)throws Exception{
        var j=new com.google.gson.JsonObject();var s=GroveTownEconomy.state(p);
        j.addProperty("passed",passed);j.addProperty("stage",stage);j.addProperty("reload",reload);j.addProperty("cash_before",base);j.addProperty("cash",GroveTownEconomy.observedCash());
        j.addProperty("logs",GroveTownEconomy.count(p,Items.OAK_LOG));j.addProperty("wheat",GroveTownEconomy.count(p,Items.WHEAT));
        j.addProperty("bread",GroveTownEconomy.count(p,Items.BREAD));j.addProperty("picks",GroveTownEconomy.count(p,Items.IRON_PICKAXE));j.addProperty("buckets",GroveTownEconomy.count(p,Items.BUCKET));
        j.addProperty("reputation",s.reputation);j.addProperty("next_transaction",s.next);j.addProperty("pending",s.pending!=null);j.addProperty("mail",GroveTownEconomy.mailCount(p));
        j.addProperty("escrow",GroveTownEconomy.escrowCount(p));j.addProperty("menu",p.containerMenu instanceof GroveTownEconomy.Board);
        j.addProperty("x",p.getX());j.addProperty("y",p.getY());j.addProperty("z",p.getZ());
        var aim=GroveInteraction.observedAim(p);if(aim!=null){j.addProperty("aim_x",aim.x);j.addProperty("aim_y",aim.y);j.addProperty("aim_z",aim.z);}
        if(villager!=null){j.addProperty("target_x",villager.getX());j.addProperty("target_y",villager.getBoundingBox().getCenter().y+.25);j.addProperty("target_z",villager.getZ());}
        Files.writeString(file("grovecraft-town-field.json"),j.toString());
    }
    private static void open(ServerPlayer p,int next){check(GroveTownEconomy.open(p,villager),"Board opens through eligible villager");stage=next;ticks=0;}
    private static void click(ServerPlayer p,int slot,int next){check(p.containerMenu instanceof GroveTownEconomy.Board,"Real board menu");
        p.containerMenu.clicked(slot,0,ContainerInput.PICKUP,p);stage=next;ticks=0;}
    private static boolean paid(ServerPlayer p,int cash){return GroveTownEconomy.state(p).pending==null&&GroveTownEconomy.observedCash()==cash&&ticks>=25;}
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.townFieldAcceptance"))return;
        check(LibertyCraft.WORLD_NAME.matches("GroveCraftGtaTestTown20261012[A-Z]?"),"Exact private town world required");
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(stage==99||!Link.active()||Link.hostSuspended()||server.getPlayerList().getPlayers().isEmpty())return;
            var p=server.getPlayerList().getPlayers().getFirst();var sky=new Link.SkyState();
            if(!Link.readSkyState(sky))return;
            try{
                // A hospital return is a death test, not a save/reload failure.
                // Preserve the world's real drops and stop instead of restocking it.
                check(stage==0||(sky.flags&(1<<22))==0,"CJ died during the economy trial; inspect death drops before reusing this world");
                if(!sky.inGame()||sky.loading()||sky.menuOpen()||(sky.flags&((1<<16)|(1<<21)|(1<<24)|(1<<4)))!=0)return;
                ticks++;if(stage==0){
                    if((sky.flags&(1<<17))==0||Math.abs(sky.x-2248)>20||Math.abs(sky.z-1262)>20
                        ||p.position().distanceToSqr(new Vec3(sky.x,sky.y,sky.z))>1||GroveTownEconomy.observedCash()<0)return;
                    base=GroveTownEconomy.observedCash();var s=GroveTownEconomy.state(p);
                    reload=Files.exists(file("grovecraft-town-"+LibertyCraft.WORLD_NAME+"-saved.json"));
                    if(reload){
                        check(s.pending==null&&s.next==8&&s.reputation==1&&s.job==null,"Saved town progression");
                        check(GroveTownEconomy.count(p,Items.OAK_LOG)==32&&GroveTownEconomy.count(p,Items.WHEAT)==32&&GroveTownEconomy.count(p,Items.BREAD)==4
                            &&GroveTownEconomy.count(p,Items.IRON_PICKAXE)==2&&GroveTownEconomy.count(p,Items.BUCKET)==1,"Saved resources exact");
                        long token=Long.parseLong(s.token,16);Link.pushEvent(48,7,-110,(float)(token&0xffffff),(float)(token>>>24),0,0);
                        stage=30;ticks=0;
                    }else if(s.next==2&&s.reputation==0){
                        // Resume the deliberately interrupted first sale. This private
                        // GTA tutorial was verified to begin with exactly $350.
                        check(base==350||base==430,"Controlled tutorial cash before resumed sale");base=350;
                        villager=p.level().getEntitiesOfClass(Villager.class,p.getBoundingBox().inflate(16),v->v.entityTags().contains("grovecraft_settler")).stream().findFirst().orElseThrow();
                        stage=2;ticks=0;
                    }else{
                        check(s.next==1&&s.pending==null,"Use another private suffix after an interrupted trial");
                        p.getInventory().clearContent();p.getInventory().setSelectedSlot(0);p.getInventory().setItem(0,new ItemStack(Items.BOW));
                        p.getInventory().setItem(9,new ItemStack(Items.OAK_LOG,64));p.getInventory().setItem(10,new ItemStack(Items.WHEAT,40));
                        villager=EntityTypes.VILLAGER.create(p.level(),EntitySpawnReason.COMMAND);check(villager!=null,"Villager fixture");
                        villager.setPos(p.getX(),p.getY(),p.getZ()-1.8);villager.setNoAi(true);villager.setNoGravity(true);villager.setPersistenceRequired();villager.addTag("grovecraft_settler");
                        villager.setVillagerData(villager.getVillagerData().withProfession(p.level().registryAccess(),VillagerProfession.FARMER));
                        check(p.level().addFreshEntity(villager),"Villager spawn");
                        stage=1;ticks=0; // External normal mouse/Shift opens the board, then buys/sells.
                    }
                }else if(stage==1){
                    if(first==null&&p.containerMenu instanceof GroveTownEconomy.Board board)first=board;
                    if(GroveTownEconomy.state(p).pending!=null){check(first!=null,"Actual town board opened before sale");stage=2;ticks=0;}
                }
                else if(stage==2&&paid(p,base+80)){check(GroveTownEconomy.count(p,Items.OAK_LOG)==48,"Exactly16 logs sold");
                    int next=GroveTownEconomy.state(p).next;if(first!=null)first.clicked(2,0,ContainerInput.PICKUP,p);check(GroveTownEconomy.state(p).next==next,"Closed menu replay rejected");
                    if(p.getInventory().getItem(0).is(Items.BOW))p.getInventory().setItem(0,ItemStack.EMPTY);open(p,3);}
                else if(stage==3&&ticks>20)click(p,11,4);
                else if(stage==4&&paid(p,base+40)){check(GroveTownEconomy.count(p,Items.BREAD)==4,"Buy4 bread with real GTA dollars");open(p,5);}
                else if(stage==5&&ticks>20)click(p,13,6);
                else if(stage==6&&paid(p,base-140))open(p,7);
                else if(stage==7&&ticks>20)click(p,13,8);
                else if(stage==8&&paid(p,base-320))open(p,81);
                else if(stage==81&&ticks>20){int next=GroveTownEconomy.state(p).next;click(p,13,82);check(GroveTownEconomy.state(p).next==next,"Finite shop stock");}
                else if(stage==82&&ticks>25)open(p,9);
                else if(stage==9&&ticks>20)click(p,14,10);
                else if(stage==10&&paid(p,base-320)&&GroveTownEconomy.state(p).next==6){check(GroveTownEconomy.count(p,Items.BUCKET)==0,"Insufficient funds gives no bucket");open(p,11);}
                else if(stage==11&&ticks>20)click(p,20,12);
                else if(stage==12&&ticks>25){check(GroveTownEconomy.state(p).job!=null,"Persistent local job accepted");open(p,13);}
                else if(stage==13&&ticks>20)click(p,21,14);
                else if(stage==14&&paid(p,base-180)){
                    check(GroveTownEconomy.state(p).reputation==1&&GroveTownEconomy.state(p).job==null,"Job payment/reputation once");
                    check(GroveTownEconomy.count(p,Items.OAK_LOG)==32&&GroveTownEconomy.count(p,Items.WHEAT)==32,"Job consumes16 logs8 wheat");
                    for(int i=0;i<36;i++)if(p.getInventory().getItem(i).isEmpty()){fillers.add(i);p.getInventory().setItem(i,new ItemStack(Items.STONE,64));}
                    open(p,15);
                }else if(stage==15&&ticks>20)click(p,14,16);
                else if(stage==16&&paid(p,base-290)){check(GroveTownEconomy.mailCount(p)==1&&GroveTownEconomy.count(p,Items.BUCKET)==0,"Full inventory purchase kept in mailbox");
                    p.getInventory().setItem(fillers.getFirst(),ItemStack.EMPTY);stage=17;ticks=0;}
                else if(stage==17&&ticks>25&&GroveTownEconomy.mailCount(p)==0){
                    check(GroveTownEconomy.count(p,Items.BUCKET)==1,"Mailbox delivery exactly once");
                    for(int slot:fillers)if(p.getInventory().getItem(slot).is(Items.STONE))p.getInventory().setItem(slot,ItemStack.EMPTY);
                    server.getPlayerList().saveAll();Files.writeString(file("grovecraft-town-"+LibertyCraft.WORLD_NAME+"-saved.json"),new com.google.gson.Gson().toJson(GroveTownEconomy.state(p)));
                    report(p,true);stage=99;return;
                }else if(stage==30&&ticks>60){check(GroveTownEconomy.observedCash()==base,"Reloaded native receipt pays/charges zero extra");
                    check(GroveTownEconomy.count(p,Items.BUCKET)==1&&GroveTownEconomy.escrowCount(p)==0&&GroveTownEconomy.mailCount(p)==0,"Reload cannot duplicate escrow/output");report(p,true);stage=99;return;}
                if(ticks%10==0)report(p,false);
            }catch(Throwable e){LibertyCraft.LOG.error("Town field acceptance",e);try{
                report(p,false);var j=com.google.gson.JsonParser.parseString(Files.readString(file("grovecraft-town-field.json"))).getAsJsonObject();
                j.addProperty("error",e.toString());j.addProperty("native_flags",sky.flags);Files.writeString(file("grovecraft-town-field.json"),j.toString());
            }catch(Exception ignored){}stage=99;}
        });
    }
}
