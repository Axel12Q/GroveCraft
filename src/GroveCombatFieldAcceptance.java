package dev.libertycraft;
import dev.libertycraft.link.Link;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.world.entity.*;
import net.minecraft.world.entity.monster.zombie.Zombie;
import net.minecraft.world.item.*;
import net.minecraft.world.phys.Vec3;
import java.nio.file.*;
/** Actual GTA mouse use and real vanilla arrows/shields; only an isolated test world. */
public final class GroveCombatFieldAcceptance {
    private static int stage,ticks;private static Zombie target;private static float health;
    private static boolean charged;private static Vec3 origin;
    private static void check(boolean ok,String reason){if(!ok)throw new IllegalStateException(reason);}
    private static void report(String status,boolean passed,net.minecraft.server.level.ServerPlayer p)throws Exception{
        var j=new com.google.gson.JsonObject();j.addProperty("status",status);j.addProperty("passed",passed);j.addProperty("stage",stage);
        j.addProperty("using",p.isUsingItem());j.addProperty("use_ticks",p.getTicksUsingItem());j.addProperty("arrows",p.getInventory().countItem(Items.ARROW));
        j.addProperty("durability",p.getMainHandItem().getDamageValue());j.addProperty("target_health",target==null?-1:target.getHealth());
        j.addProperty("x",p.getX());j.addProperty("y",p.getY());j.addProperty("z",p.getZ());
        var aim=GroveInteraction.observedAim(p);if(aim!=null){j.addProperty("aim_x",aim.x);j.addProperty("aim_y",aim.y);j.addProperty("aim_z",aim.z);}
        if(target!=null){j.addProperty("target_x",target.getX());j.addProperty("target_y",target.getBoundingBox().getCenter().y+.25);j.addProperty("target_z",target.getZ());}
        Files.writeString(FabricLoader.getInstance().getGameDir().resolve("grovecraft-combat-field.json"),j.toString());
    }
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.combatFieldAcceptance"))return;
        check(LibertyCraft.WORLD_NAME.equals("GroveCraftGtaTestCombat20261012"),"Private combat world required");
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(stage==99||!Link.active()||Link.hostSuspended()||server.getPlayerList().getPlayers().isEmpty())return;
            var p=server.getPlayerList().getPlayers().getFirst();var sky=new Link.SkyState();
            if(!Link.readSkyState(sky)||!sky.inGame()||sky.loading()||sky.menuOpen()||(sky.flags&((1<<16)|(1<<21)|(1<<24)|(1<<4)))!=0)return;
            try{
                ticks++;
                if(stage==0){
                    if(Math.abs(sky.x-2248)>20||Math.abs(sky.z-1262)>20||(sky.flags&(1<<17))==0||p.position().distanceToSqr(new Vec3(sky.x,sky.y,sky.z))>1)return;
                    for(var e:p.level().getEntitiesOfClass(Zombie.class,p.getBoundingBox().inflate(16),e->e.entityTags().contains("grovecraft_combat_test")))e.discard();
                    p.getInventory().clearContent();p.getInventory().setSelectedSlot(0);p.getInventory().setItem(0,new ItemStack(Items.BOW));p.getInventory().setItem(9,new ItemStack(Items.ARROW,16));
                    origin=p.position();target=EntityTypes.ZOMBIE.create(p.level(),EntitySpawnReason.COMMAND);check(target!=null,"Target creation");
                    target.addTag("grovecraft_combat_test");target.setNoAi(true);target.setNoGravity(true);target.setItemSlot(EquipmentSlot.HEAD,new ItemStack(Items.IRON_HELMET));
                    // A five-metre target here lies behind the native alley wall.
                    target.setPos(origin.x,origin.y,origin.z-2.2);check(p.level().addFreshEntity(target),"Target spawn");health=target.getHealth();stage=1;ticks=0;
                }else if(stage==1){
                    if(p.isUsingItem()&&p.getTicksUsingItem()>=20)charged=true;
                    if(charged&&!p.isUsingItem()&&p.getInventory().countItem(Items.ARROW)==15){stage=2;ticks=0;}
                }else if(stage==2&&ticks>=30){
                    check(target.getHealth()<health,"Released mouse arrow missed target");check(p.getMainHandItem().getDamageValue()==1,"Bow durability once");
                    p.getInventory().setItem(0,new ItemStack(Items.SHIELD));target.setPos(p.getX(),p.getY(),p.getZ()-1.1);stage=3;ticks=0;
                }else if(stage==3&&p.isUsingItem()&&p.getTicksUsingItem()>=10){
                    p.hurtServer(p.level(),p.level().damageSources().mobAttack(target),4);
                    check(p.getMainHandItem().getDamageValue()>0,"Frontal shield wear");stage=4;ticks=0;
                }else if(stage==4&&ticks>=30){
                    target.setPos(p.getX(),p.getY(),p.getZ()+1.1);target.setDeltaMovement(Vec3.ZERO);stage=5;ticks=0;
                }else if(stage==5&&ticks>=20){
                    check(p.isUsingItem(),"Shield remains held for rear hit");
                    target.setPos(p.getX(),p.getY(),p.getZ()+1.1);target.setDeltaMovement(Vec3.ZERO);
                    p.hurtServer(p.level(),p.level().damageSources().mobAttack(target),4);stage=6;ticks=0;
                }else if(stage==6&&ticks>=20){stage=7;ticks=0;report("rear_hit_sent_release_shield",false,p);}
                else if(stage==7&&!p.isUsingItem()){report("passed",true,p);stage=99;return;}
                if(ticks%10==0)report(stage==1?"aim_and_hold_bow":stage==3?"hold_shield":stage==4?"front_blocked":stage==7?"rear_hit_sent_release_shield":"waiting",false,p);
            }catch(Throwable e){stage=99;LibertyCraft.LOG.error("Combat field acceptance",e);try{Files.writeString(FabricLoader.getInstance().getGameDir().resolve("grovecraft-combat-field.json"),"{\"passed\":false,\"error\":"+new com.google.gson.Gson().toJson(e.toString())+"}");}catch(Exception ignored){}}
        });
    }
}
