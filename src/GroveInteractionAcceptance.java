package dev.libertycraft;

import dev.libertycraft.link.Link;
import net.minecraft.core.BlockPos;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.entity.EntitySpawnReason;
import net.minecraft.world.entity.EntityTypes;
import net.minecraft.world.entity.animal.cow.Cow;
import net.minecraft.world.entity.animal.sheep.Sheep;
import net.minecraft.world.entity.monster.zombie.Zombie;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.phys.Vec3;

/** Real integrated-server fixture, opt-in and isolated from the user's world. */
public final class GroveInteractionAcceptance {
    private static int stage, wait;
    private static Cow cow, partner;
    private static Sheep sheep;
    private static Zombie zombie;
    private static float zombieHealth;
    private static final int BASE_FLAGS=9|(1<<17);
    private static void require(boolean ok, String reason) { if (!ok) throw new IllegalStateException(reason); }
    private static void aim(ServerPlayer p, Vec3 point) {
        GroveGameplay.act(p, 14, (int)(point.x * 256), (int)(point.y * 256), (int)(point.z * 256));
    }
    private static void use(ServerPlayer p, net.minecraft.world.entity.LivingEntity target) {
        aim(p, target.getBoundingBox().getCenter()); GroveGameplay.act(p, 15, 0, 0, 0);
    }
    private static void request(int action) throws Exception {
        request(action,BASE_FLAGS);
    }
    private static void request(int action,int flags) throws Exception {
        java.nio.file.Files.writeString(net.fabricmc.loader.api.FabricLoader.getInstance().getGameDir()
            .resolve("grovecraft-interaction-input.json"), "{\"action\":"+action+",\"flags\":"+flags+"}");
    }
    public static void register() {
        if (!Boolean.getBoolean("grovecraft.interactionAcceptance")) return;
        net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents.END_SERVER_TICK.register(server -> {
            if (stage == 99 || server.getPlayerList().getPlayers().isEmpty()) return;
            var p = server.getPlayerList().getPlayers().getFirst();
            var level = p.level();
            Link.SkyState host = new Link.SkyState();
            if (!Link.readSkyState(host) || (stage < 7 && !GroveInteraction.safe(host))) return;
            try {
                if (stage == 0) {
                    if (++wait < 80 || p.position().distanceToSqr(new Vec3(host.x, host.y, host.z)) > 1) return;
                    // Fresh isolated fixture: remove its previous entities/blocks, never touch GroveCraftLab.
                    for (var e : level.getEntitiesOfClass(net.minecraft.world.entity.Mob.class, p.getBoundingBox().inflate(12))) e.discard();
                    BlockPos origin = p.blockPosition();
                    for (int x=-5;x<=5;x++) for(int z=-5;z<=9;z++) {
                        level.setBlockAndUpdate(origin.offset(x,-1,z), Blocks.STONE.defaultBlockState());
                        for(int y=0;y<=4;y++) level.setBlockAndUpdate(origin.offset(x,y,z),Blocks.AIR.defaultBlockState());
                    }
                    p.getInventory().setSelectedSlot(0);
                    cow = EntityTypes.COW.create(level, EntitySpawnReason.COMMAND);
                    require(cow != null, "cow fixture"); cow.setPos(p.getX(),p.getY(),p.getZ()+2); cow.setNoAi(true); level.addFreshEntity(cow);
                    p.getInventory().setItem(0, new ItemStack(Items.BUCKET)); use(p,cow);
                    require(p.getMainHandItem().is(Items.MILK_BUCKET), "actual cow interaction fills milk bucket");
                    p.getInventory().setItem(0,new ItemStack(Items.WHEAT,4)); use(p,cow);
                    require(cow.isInLove() && p.getMainHandItem().getCount()==3,"wheat consumed once and vanilla love");
                    use(p,cow); require(p.getMainHandItem().getCount()==3,"cannot feed love animal repeatedly");
                    // A wall must prevent interaction; an inappropriate item must not place through an animal.
                    cow.resetLove();
                    for(int y=0;y<3;y++) level.setBlockAndUpdate(origin.offset(0,y,1),Blocks.STONE.defaultBlockState());
                    aim(p,cow.getBoundingBox().getCenter());
                    require(GroveInteraction.target(p)==null,"wall blocks creature selection");
                    for(int y=0;y<3;y++) level.setBlockAndUpdate(origin.offset(0,y,1),Blocks.AIR.defaultBlockState());
                    p.getInventory().setItem(0,new ItemStack(Items.OAK_PLANKS,4)); use(p,cow);
                    require(p.getMainHandItem().getCount()==4,"block not placed through creature");
                    cow.setPos(p.getX(),p.getY(),p.getZ()+6); aim(p,cow.getBoundingBox().getCenter());
                    require(GroveInteraction.target(p)==null,"distant creature out of reach");
                    cow.setPos(p.getX(),p.getY(),p.getZ()+2);
                    partner=EntityTypes.COW.create(level,EntitySpawnReason.COMMAND); require(partner!=null,"partner fixture");
                    partner.setPos(p.getX()+1,p.getY(),p.getZ()+2); partner.setNoAi(true); level.addFreshEntity(partner);
                    p.getInventory().setItem(0,new ItemStack(Items.WHEAT,4)); use(p,cow); use(p,partner);
                    require(cow.isInLove()&&partner.isInLove()&&p.getMainHandItem().getCount()==2,"both parents fed");
                    cow.setNoAi(false);partner.setNoAi(false); stage=1;wait=0;
                } else if(stage==1) {
                    boolean baby=level.getEntitiesOfClass(Cow.class,p.getBoundingBox().inflate(10)).stream().anyMatch(Cow::isBaby);
                    require(++wait<400,"natural breeding timeout");
                    if(!baby)return;
                    require(cow.getAge()>0&&partner.getAge()>0,"vanilla breeding cooldown");
                    for(var c:level.getEntitiesOfClass(Cow.class,p.getBoundingBox().inflate(12)))c.discard();
                    sheep=EntityTypes.SHEEP.create(level,EntitySpawnReason.COMMAND);require(sheep!=null,"sheep fixture");
                    sheep.setPos(p.getX(),p.getY(),p.getZ()+2);sheep.setNoAi(true);level.addFreshEntity(sheep);
                    p.getInventory().setItem(0,new ItemStack(Items.SHEARS));use(p,sheep);
                    require(sheep.isSheared()&&p.getMainHandItem().getDamageValue()==1,"actual shearing and durability");
                    sheep.discard();
                    zombie=EntityTypes.ZOMBIE.create(level,EntitySpawnReason.COMMAND);require(zombie!=null,"target fixture");
                    zombie.setItemSlot(net.minecraft.world.entity.EquipmentSlot.HEAD,new ItemStack(Items.IRON_HELMET));
                    zombie.setPos(p.getX(),p.getY(),p.getZ()+5);zombie.setNoAi(true);zombie.setNoGravity(true);level.addFreshEntity(zombie);
                    p.getInventory().setItem(0,new ItemStack(Items.BOW));p.getInventory().setItem(9,new ItemStack(Items.ARROW,16));
                    aim(p,zombie.getBoundingBox().getCenter());request(15);stage=2;wait=0;
                } else if(stage==2) {
                    aim(p,zombie.getBoundingBox().getCenter());
                    if(++wait<30)return;
                    require(p.isUsingItem(),"bow starts and stays charged");request(17);stage=21;wait=0;
                } else if(stage==21) {
                    if(++wait<5)return;
                    require(!p.isUsingItem()&&p.getInventory().getItem(9).getCount()==16,"cancel fires no arrow");
                    aim(p,zombie.getBoundingBox().getCenter());request(15);stage=3;wait=0;
                } else if(stage==3) {
                    aim(p,zombie.getBoundingBox().getCenter());if(++wait<25)return;
                    require(p.isUsingItem(),"second bow charge");zombieHealth=zombie.getHealth();request(16);stage=4;wait=0;
                } else if(stage==4) {
                    if(++wait<20)return;
                    require(p.getInventory().getItem(9).getCount()==15,"release consumes exactly one arrow");
                    require(p.getMainHandItem().getDamageValue()==1,"bow durability");
                    require(zombie.getHealth()<zombieHealth,"real arrow hits living target");
                    zombie.setPos(p.getX(),p.getY(),p.getZ()+1.1);
                    p.getInventory().setItem(0,new ItemStack(Items.SHIELD));aim(p,zombie.getBoundingBox().getCenter());request(15);stage=5;wait=0;
                } else if(stage==5) {
                    aim(p,zombie.getBoundingBox().getCenter());if(++wait<15)return;
                    require(p.isUsingItem(),"shield raised");
                    GroveInteraction.orient(p);
                    p.hurtServer(level,level.damageSources().mobAttack(zombie),4);
                    require(p.getMainHandItem().getDamageValue()>0,"vanilla shield absorbs frontal mob damage and wears");
                    zombie.setPos(p.getX(),p.getY(),p.getZ()-1.1);
                    // Keep facing forward: attacks from behind must still reach the native damage bridge.
                    stage=6;wait=0;
                } else if(stage==6) {
                    aim(p,new Vec3(p.getX(),p.getEyeY(),p.getZ()+4));if(++wait<20)return;
                    GroveInteraction.orient(p);p.hurtServer(level,level.damageSources().mobAttack(zombie),4);
                    GroveGameplay.act(p,17,0,0,0);require(!p.isUsingItem(),"shield cancellation");
                    zombie.setPos(p.getX()+1.1,p.getY(),p.getZ());zombie.setDeltaMovement(Vec3.ZERO);zombieHealth=zombie.getHealth();
                    p.getInventory().setItem(0,new ItemStack(Items.BOW));request(15,BASE_FLAGS|(1<<19));stage=7;wait=0;
                }else if(stage>=7&&stage<=9){
                    if(++wait<10)return;
                    require(!p.isUsingItem(),"Menu/sleep/arrival must reject held use, stage "+stage);
                    GroveGameplay.act(p,12,0,0,0);
                    require(zombie.getHealth()==zombieHealth,"Queued attack during unsafe state, stage "+stage);
                    require(p.getInventory().getItem(9).getCount()==15,"Unsafe state fires no arrow");
                    if(stage<9){stage++;wait=0;request(15,BASE_FLAGS|(stage==8?1<<23:1<<24));return;}
                    request(17);stage=10;wait=0;
                }else if(stage==10){
                    if(++wait<10)return;
                    var path=net.fabricmc.loader.api.FabricLoader.getInstance().getGameDir().resolve("grovecraft-interaction-acceptance.json");
                    java.nio.file.Files.writeString(path,"{\"passed\":true,\"real_minecraft\":true,\"fixture_host\":true,\"milk\":true,\"feeding\":true,\"breeding_baby\":true,\"cooldown\":true,\"shearing\":true,\"blocked_and_range\":true,\"bow_cancel\":true,\"arrow_damage\":true,\"shield_front_durability\":true,\"menu_sleep_respawn_actions_rejected\":true}");stage=99;
                }
            }catch(Exception error){stage=99;LibertyCraft.LOG.error("[GroveCraft] Interaction acceptance failed",error);
                try{java.nio.file.Files.writeString(net.fabricmc.loader.api.FabricLoader.getInstance().getGameDir().resolve("grovecraft-interaction-acceptance.json"),"{\"passed\":false}");}catch(Exception ignored){}
            }
        });
    }
}
