package dev.libertycraft;
import dev.libertycraft.world.HostDig;

import dev.libertycraft.link.Link;
import dev.libertycraft.link.Proto;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.phys.BlockHitResult;
import net.minecraft.world.phys.Vec3;

/** Local integrated-server actions. Protocol extension 32 is reserved for GroveCraft. */
public final class GroveGameplay {
    private GroveGameplay() {}
    private static final net.minecraft.world.item.Item[] KIT = {Items.OAK_PLANKS, Items.COBBLESTONE, Items.DIRT,
            Items.GLASS, Items.CRAFTING_TABLE, Items.WOODEN_HOE, Items.WHEAT_SEEDS, Items.BREAD, Items.OAK_SAPLING};
    private static int itemType(ItemStack stack) {
        for (int i = 0; i < KIT.length; i++) if (stack.is(KIT[i])) return i + 1;
        if (stack.is(Items.DIAMOND_PICKAXE)) return 10;
        if (stack.is(Items.COOKED_BEEF)) return 11;
        if (stack.is(Items.TORCH)) return 12;
        if (stack.is(Items.ENDER_CHEST)) return 13;
        if (stack.isEmpty()) return 0;
        if (stack.getItem() instanceof net.minecraft.world.item.ProjectileWeaponItem) return 20;
        if (stack.get(net.minecraft.core.component.DataComponents.BLOCKS_ATTACKS) != null) return 21;
        if (stack.is(Items.TRIDENT)) return 22;
        if (stack.is(Items.BUCKET)) return 17;
        if (stack.is(Items.WATER_BUCKET)) return 18;
        if (stack.is(Items.LAVA_BUCKET)) return 19;
        if (stack.getItem() instanceof net.minecraft.world.item.BlockItem) return 14;
        if (stack.get(net.minecraft.core.component.DataComponents.FOOD) != null) return 11;
        if (stack.is(net.minecraft.tags.ItemTags.SWORDS)) return 15;
        if (stack.get(net.minecraft.core.component.DataComponents.TOOL) != null) return 10;
        return 16;
    }
    private static Vec3 previous;
    private static volatile BlockPos mining;
    private static ItemStack miningTool = ItemStack.EMPTY;
    private static volatile int miningNativeMaterial;
    private static volatile float miningProgress;
    public static BlockPos miningPosition() { return mining; }
    public static float miningFraction() { return miningProgress; }
    public static boolean virtualMining() { return mining!=null&&miningNativeMaterial>0; }
    private static final java.util.Map<java.util.UUID, Double> dropFloors = new java.util.HashMap<>();
    public static void preserveNativeDrop(net.minecraft.world.entity.item.ItemEntity item,double floor) {
        if (!Double.isFinite(floor) || floor < -60 || floor > 1000) return;
        dropFloors.put(item.getUUID(),floor);
        item.addTag("grovecraft_ground:"+Double.toHexString(floor));
    }
    private static int lastMineTick, breakCooldown,lastJumpTick;
    private static boolean wasGrounded;
    private static final java.util.Map<java.util.UUID, Long> meals = new java.util.concurrent.ConcurrentHashMap<>();
    private static int mealSlot, mealCount, mealNutrition;
    private static boolean mealFull;
    public static boolean canOvereat(java.util.UUID uuid) {
        return meals.getOrDefault(uuid, 0L) > System.nanoTime();
    }
    private static void abort(ServerPlayer player) {
        if (mining != null) player.level().destroyBlockProgress(player.getId(), mining, -1);
        mining = null; miningProgress = 0;miningNativeMaterial=0;
        Link.pushEvent(34, 0, 0, 0, 0, 0, 0);
    }
    private static boolean reachable(BlockPos pos, Link.SkyState host) {
        return Math.abs((double)pos.getX()) <= 10000 && Math.abs((double)pos.getZ()) <= 10000
                && pos.getY() >= -1000 && pos.getY() <= 1000
                && Vec3.atCenterOf(pos).distanceToSqr(new Vec3(host.x, host.y + 1, host.z)) <= 36;
    }
    private static int virtualMaterial(ServerPlayer player,BlockPos pos,int world){
        var level=player.level();if(!HostDig.destruction||!level.hasChunkAt(pos)||HostDig.isDug(level,world,pos))return 0;
        boolean adjacent=false;
        for(var direction:Direction.values()){
            var neighbour=pos.relative(direction);
            if(level.hasChunkAt(neighbour)&&HostDig.isDug(level,world,neighbour)){adjacent=true;break;}
        }
        if(!adjacent)return 0;
        int material=HostDig.classify(pos.getX(),pos.getY(),pos.getZ());
        return material>0&&!GroveNativeTrees.wood(material)?material:0;
    }
    public static void register() {
        net.fabricmc.fabric.api.event.lifecycle.v1.ServerLifecycleEvents.SERVER_STOPPING.register(server -> {
            previous = null; mining = null;miningNativeMaterial=0; meals.clear(); dropFloors.clear(); miningProgress = 0; breakCooldown = 0; lastJumpTick = 0; wasGrounded = false; GroveFluidWake.reset(); GroveInteraction.reset();
        });
        net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents.END_SERVER_TICK.register(server -> {
            Link.SkyState host = new Link.SkyState();
            if (!Link.active() || !Link.readSkyState(host) || (host.flags & Proto.SKY_IN_GAME) == 0
                    || server.getPlayerList().getPlayers().isEmpty()) { previous = null; return; }
            var player = server.getPlayerList().getPlayers().getFirst();
            GroveInteraction.tick(player, host);
            Vec3 now = new Vec3(host.x, host.y, host.z);
            boolean safe = !player.isSleeping()&&(host.flags & (Proto.SKY_LOADING | Proto.SKY_MENU_OPEN | Proto.SKY_IN_VEHICLE | (1<<21) | (1<<23) | (1<<24))) == 0;
            if(!safe){abort(player);player.stopUsingItem();}
            if (safe && (host.flags & ((1 << 16) | (1 << 19))) == 0) GroveFluidWake.tick(player);
            boolean grounded = (host.flags & (1 << 17)) != 0;
            boolean sprinting = (host.flags & (1 << 18)) != 0;
            player.setOnGround(grounded);
            if (safe && previous != null) {
                double distance = Math.hypot(now.x - previous.x, now.z - previous.z);
                // Vanilla: walking costs nothing; sprinting costs 0.1 exhaustion per metre.
                if (Double.isFinite(distance) && distance < 8) {
                    if (player.isInWater()) player.causeFoodExhaustion((float)(distance * .01));
                    else if (sprinting) player.causeFoodExhaustion((float)(distance * .1));
                }
                if (wasGrounded && !grounded && now.y > previous.y + .12&&server.getTickCount()-lastJumpTick>=20){
                    player.causeFoodExhaustion(sprinting ? .2f : .05f);
                    lastJumpTick=server.getTickCount();
                }
            }
            previous = safe ? now : null; wasGrounded = safe && grounded;
            int tick = server.getTickCount();
            if (breakCooldown > 0 && safe) --breakCooldown;
            if (mining != null) {
                if (!safe || (host.flags & (1 << 16)) != 0 || tick - lastMineTick > 6
                        || !reachable(mining, host) || !ItemStack.isSameItemSameComponents(miningTool, player.getMainHandItem())) abort(player);
                else {
                    var actual = player.level().getBlockState(mining);
                    var block = miningNativeMaterial>0?HostDig.materialState(miningNativeMaterial):actual;
                    if (block.isAir()||(miningNativeMaterial>0&&(!actual.isAir()||virtualMaterial(player,mining,host.worldId)!=miningNativeMaterial))) abort(player);
                    else {
                        miningProgress += block.getDestroyProgress(player, player.level(), mining);
                        player.level().destroyBlockProgress(player.getId(), mining, Math.min(9, (int)(miningProgress * 10)));
                        Link.pushEvent(34, 0, Math.min(1, miningProgress), 1, 0, 0, 0);
                        if (miningProgress >= 1) {
                            // Vanilla survival harvesting, correct-tool loot tables, enchantments and durability.
                            if(miningNativeMaterial>0)HostDig.open(player,host.worldId,mining,miningNativeMaterial);
                            else player.gameMode.destroyBlock(mining);
                            for (var item : player.level().getEntitiesOfClass(net.minecraft.world.entity.item.ItemEntity.class,
                                    new net.minecraft.world.phys.AABB(mining).inflate(1))) {
                                if (!dropFloors.containsKey(item.getUUID())) {
                                    dropFloors.put(item.getUUID(), host.y);
                                    item.addTag("grovecraft_ground:" + Double.toHexString(host.y));
                                }
                            }
                            abort(player); breakCooldown = 5;
                        }
                    }
                }
            }
            if (safe && tick % 20 == 0) for (var item : player.level().getEntitiesOfClass(
                    net.minecraft.world.entity.item.ItemEntity.class, player.getBoundingBox().inflate(32))) {
                if (dropFloors.containsKey(item.getUUID())) continue;
                for (String tag : item.entityTags()) if (tag.startsWith("grovecraft_ground:")) {
                    try { double floor = Double.valueOf(tag.substring(18));
                        if (Double.isFinite(floor) && floor > -60 && floor < 1000) dropFloors.put(item.getUUID(), floor);
                    } catch (NumberFormatException ignored) {}
                }
            }
            if (safe) for (var entry : new java.util.ArrayList<>(dropFloors.entrySet())) {
                var entity = player.level().getEntity(entry.getKey());
                if (!(entity instanceof net.minecraft.world.entity.item.ItemEntity item) || item.isRemoved()) {
                    dropFloors.remove(entry.getKey()); continue;
                }
                if (item.getY() < entry.getValue()) {
                    item.setPos(item.getX(), entry.getValue(), item.getZ());
                    item.setDeltaMovement(item.getDeltaMovement().multiply(.6, 0, .6));
                    item.setNoGravity(true);
                }
                // The no-physics CJ mirror does not run vanilla ground contacts; keep real pickup delay/inventory rules.
                if (item.getBoundingBox().intersects(player.getBoundingBox().inflate(.5))) item.playerTouch(player);
            }
            if (canOvereat(player.getUUID())) {
                var stack = player.getInventory().getItem(mealSlot);
                if (stack.getCount() < mealCount || stack.isEmpty()) {
                    if (mealFull) Link.pushEvent(35, 0, mealNutrition * 2f, 0, 0, 0, 0);
                    meals.remove(player.getUUID());
                } else if (!safe || player.getInventory().getSelectedSlot() != mealSlot) {
                    player.stopUsingItem(); meals.remove(player.getUUID());
                }
            }
            // GTA owns body weight; tiny per-second deltas prevent abrupt shape changes.
            if (safe && tick % 20 == 0 && player.getFoodData().getFoodLevel() <= 6)
                Link.pushEvent(35, 0, -.05f, 0, 0, 0, 0);
            if (tick % 4 == 0) Link.pushEvent(34, 0, miningProgress, mining != null ? 1 : 0,
                    player.isUsingItem() && player.getUseItem().get(net.minecraft.core.component.DataComponents.FOOD) != null ? 1 : 0,
                    safe && player.isUsingItem() && GroveInteraction.sustained(player.getUseItem()) ? itemType(player.getUseItem()) : 0, 0);
            if (tick % 4 == 0) Link.pushEvent(38, 0,
                    (float)player.getAttributeValue(net.minecraft.world.entity.ai.attributes.Attributes.ATTACK_DAMAGE),
                    (float)Math.clamp(1/player.getAttributeValue(net.minecraft.world.entity.ai.attributes.Attributes.ATTACK_SPEED),.1,5),0,0,0);
            if (server.getTickCount() % 20 == 0) {
                var clock = server.registryAccess().lookupOrThrow(net.minecraft.core.registries.Registries.WORLD_CLOCK)
                        .getOrThrow(net.minecraft.world.clock.WorldClocks.OVERWORLD);
                long time = server.clockManager().getInstance(clock).totalTicks();
                Link.pushEvent(32, player.getInventory().getSelectedSlot(), player.getFoodData().getFoodLevel(),
                        player.getHealth(), player.getMainHandItem().getCount(), dev.libertycraft.world.SkyRules.hourOf(time), itemType(player.getMainHandItem()));
                for (int slot = 0; slot < 9; slot++) {
                    var stack = player.getInventory().getItem(slot);
                    Link.pushEvent(33, slot, stack.getCount(), itemType(stack), 0, 0,
                            stack.getItem() instanceof net.minecraft.world.item.BlockItem ? 1 : 0);
                }
            }
        });
    }
    public static void act(ServerPlayer player, int action, int x, int y, int z) {
        Link.SkyState host = new Link.SkyState();
        Link.readSkyState(host);
        if(action==501||action==502){GroveTownEconomy.receive(player,action,x,y,z);return;}
        if(action==503){GroveRoadside.receive(player,x,y,z);return;}
        if(action>=2600&&action<=2799){GroveNativeTrees.fell(player,action-2000,x,y,z);return;}
        if (action == 17) { GroveInteraction.finish(player, false); return; }
        if(action==19){
            if(Link.active()&&(host.flags&Proto.SKY_IN_GAME)!=0&&(host.flags&(Proto.SKY_LOADING|Proto.SKY_MENU_OPEN))==0)
                GroveArmorState.nativeWear(player,x,y);
            return;
        }
        // Keep asynchronous receipts, native drops and armor damage above this
        // gate; block only player interactions while the mechanic is working.
        if(GroveRoadside.busy(player)&&action!=9&&action!=13&&action!=14&&action!=16&&action!=17&&action!=5&&action!=8&&!(action>=100&&action<=108))return;
        // Recheck on the server thread: an event queued before a mission/vehicle change is unsafe.
        if (!Link.active() || (host.flags & Proto.SKY_IN_GAME) == 0
                || player.isSleeping() || (host.flags & (Proto.SKY_LOADING | Proto.SKY_MENU_OPEN | Proto.SKY_IN_VEHICLE | (1<<19) | (1<<21) | (1<<23) | (1<<24))) != 0) return;
        if ((host.flags & (1 << 16)) != 0 && action != 4 && action != 8 && !(action >= 100 && action <= 108)) return;
        if (action == 14) { if (GroveInteraction.safe(host)) GroveInteraction.aim(player, x, y, z); return; }
        if (action == 18) { abort(player);GroveInteraction.finish(player,false);GroveCargo.open(player,x);return; }
        if(action==28){player.stopUsingItem();abort(player);GroveInteraction.finish(player,false);GroveInteractionHints.merchant(player,x);return;}
        if(action==29){player.stopUsingItem();abort(player);GroveInteraction.finish(player,false);GroveInteractionHints.useBlock(player,new BlockPos(x,y,z));return;}
        if (action == 26) { abort(player);GroveInteraction.finish(player,false);GroveCargo.workshop(player,x);return; }
        if(action==30){player.stopUsingItem();abort(player);GroveInteraction.finish(player,false);GroveRoadside.open(player,x);return;}
        if (action == 16) { GroveInteraction.finish(player, GroveInteraction.safe(host)); return; }
        if(action==25||(action>=410&&action<=415)){
            var target=GroveInteraction.target(player);
            if(GroveTownEconomy.enabled()&&target instanceof net.minecraft.world.entity.npc.villager.Villager v&&v.entityTags().contains("grovecraft_settler")){
                abort(player);GroveInteraction.finish(player,false);GroveTownEconomy.open(player,v);return;
            }
            action=action==25?15:action-10;
        }
        if (action == 15 || (action >= 400 && action <= 405)) {
            if (!GroveInteraction.safe(host)) return;
            abort(player);
            if (GroveInteraction.creature(player)) return;
            if (action == 15) { GroveInteraction.use(player); return; }
            action -= 100; // same face/position checks as block interactions
        }
        if (action >= 100 && action <= 108) {
            GroveInteraction.finish(player, false);
            player.getInventory().setSelectedSlot(action - 100);
            player.connection.send(new net.minecraft.network.protocol.game.ClientboundSetHeldSlotPacket(action - 100));
            return;
        }
        if (action == 8) { player.causeFoodExhaustion(1.2f); return; }
        if (action == 9) { abort(player); return; }
        if (action == 13) {
            player.causeFoodExhaustion(.1f);
            var stack = player.getMainHandItem();
            if (stack.isDamageableItem()) stack.hurtAndBreak(stack.is(net.minecraft.tags.ItemTags.SWORDS) ? 1 : 2,
                    player,net.minecraft.world.entity.EquipmentSlot.MAINHAND);
            player.resetAttackStrengthTicker();return;
        }
        if (action == 12) {
            abort(player);
            double yaw = Math.toRadians(host.yaw);
            Vec3 direction = new Vec3(-Math.sin(yaw), 0, Math.cos(yaw));
            net.minecraft.world.entity.LivingEntity target = null;
            double distance = 9;
            for (var entity : player.level().getEntitiesOfClass(net.minecraft.world.entity.LivingEntity.class,
                    player.getBoundingBox().inflate(3), entity -> entity != player && entity.isAlive()
                        && (!(entity instanceof dev.libertycraft.combat.HostActorEntity)
                            || !player.getMainHandItem().isEmpty()))) {
                Vec3 delta = entity.position().subtract(player.position());
                double length = delta.lengthSqr();
                if (entity instanceof dev.libertycraft.combat.HostActorEntity actor && (actor.isMission() || actor.isHostCorpse())) continue;
                Vec3 eye = player.getEyePosition();
                Vec3 aim = entity.getBoundingBox().getCenter();
                var blockHit = player.level().clip(new net.minecraft.world.level.ClipContext(eye, aim,
                        net.minecraft.world.level.ClipContext.Block.COLLIDER, net.minecraft.world.level.ClipContext.Fluid.NONE, player));
                var nativeHit = dev.libertycraft.world.HostClip.cast(eye, aim);
                boolean obstructed = blockHit.getType() != net.minecraft.world.phys.HitResult.Type.MISS
                        && eye.distanceToSqr(blockHit.getLocation()) + .01 < eye.distanceToSqr(aim);
                if (nativeHit != null && nativeHit.t() < .98) obstructed = true;
                boolean nativeCandidate=entity instanceof dev.libertycraft.combat.HostActorEntity;
                boolean nativeSelected=target instanceof dev.libertycraft.combat.HostActorEntity;
                // Broad native vehicle proxies must not steal a melee swing from a
                // reachable Minecraft mob immediately beside a parked bike/car.
                if (!obstructed && length < 9 && delta.normalize().dot(direction) > .65
                        && (target==null||!nativeCandidate&&nativeSelected
                            ||nativeCandidate==nativeSelected&&length<distance)) { target = entity; distance = length; }
            }
            if (target != null) player.attack(target);
            return;
        }
        if (action == 4) {
            if (player.getInventory().getSelectedSlot() != x) return;
            var food = player.getMainHandItem().get(net.minecraft.core.component.DataComponents.FOOD);
            if (food == null || canOvereat(player.getUUID())) return;
            mealSlot = x; mealCount = player.getMainHandItem().getCount(); mealNutrition = food.nutrition();
            mealFull = player.getFoodData().getFoodLevel() == 20;
            meals.put(player.getUUID(), System.nanoTime() + 10_000_000_000L);
            player.gameMode.useItem(player, player.level(), player.getMainHandItem(), InteractionHand.MAIN_HAND);
            return;
        }
        if (action == 5) {
            player.level().getServer().saveEverything(false, true, false);
            LibertyCraft.LOG.info("[GroveCraft] Minecraft checkpoint saved");
            return;
        }
        if (action == 6) {
            if (Boolean.getBoolean("grovecraft.expedition")) return;
            // Explicit once-per-player lab kit, persisted as an entity tag. Never overwrite inventory.
            if (!player.entityTags().contains("grovecraft_lab_kit")) {
                var inventory = player.getInventory();
                for (int slot = 0; slot < KIT.length; slot++) {
                    var item = KIT[slot];
                    var current = inventory.getItem(slot);
                    if (current.is(item)) continue;
                    if (!current.isEmpty()) {
                        int free = -1;
                        for (int n = 9; n < 36; n++) if (inventory.getItem(n).isEmpty()) { free = n; break; }
                        if (free < 0) { LibertyCraft.LOG.warn("[GroveCraft] Kit needs empty inventory slots; nothing is discarded"); return; }
                        inventory.setItem(free, current);
                    }
                    inventory.setItem(slot, new ItemStack(item, item == Items.WOODEN_HOE ? 1 : 16));
                }
                player.addTag("grovecraft_lab_kit");
            }
            return;
        }
        Direction face = Direction.UP;
        if (action >= 200 && action <= 205) { face = Direction.values()[action - 200]; action = 1; }
        if (action >= 300 && action <= 305) { face = Direction.values()[action - 300]; action = 3; }
        var pos = new BlockPos(x, y, z);
        if (Math.abs((double)x) > 10000 || Math.abs((double)z) > 10000 || y < -1000 || y > 1000
                || new Vec3(x + 0.5, y + 0.5, z + 0.5).distanceToSqr(new Vec3(host.x, host.y + 1.0, host.z)) > 36) return;
        if (action == 2) {
            if (breakCooldown > 0 || player.isUsingItem()) return;
            lastMineTick = player.level().getServer().getTickCount();
            if (!pos.equals(mining)) {
                abort(player);
                var block = player.level().getBlockState(pos);
                if(block.isAir()){
                    miningNativeMaterial=virtualMaterial(player,pos,host.worldId);
                    if(miningNativeMaterial>0){
                        block=HostDig.materialState(miningNativeMaterial);
                        LibertyCraft.LOG.info("[GroveCraft] virtual terrain mining started {} material {}",pos,miningNativeMaterial);
                    }
                }
                if (block.isAir() || block.getDestroySpeed(player.level(), pos) < 0) return;
                mining = pos; miningTool = player.getMainHandItem().copy(); miningProgress = 0;
                block.attack(player.level(), pos, player);
            }
        } else if (action == 1 || action == 3) {
            if (player.getMainHandItem().getItem() instanceof net.minecraft.world.item.BucketItem) {
                GroveBucketTarget.set(new BlockHitResult(Vec3.atCenterOf(pos), face, pos, false));
                try {
                    player.gameMode.useItem(player, player.level(), player.getMainHandItem(), InteractionHand.MAIN_HAND);
                } finally { GroveBucketTarget.clear(); }
                LibertyCraft.LOG.info("[GroveCraft] Bucket used at {}: {} / {}", pos, player.getMainHandItem(), player.level().getBlockState(pos.relative(face)));
                return;
            }
            var result = player.gameMode.useItemOn(player, player.level(), player.getMainHandItem(), InteractionHand.MAIN_HAND,
                    new BlockHitResult(new Vec3(x + 0.5 + face.getStepX() * 0.5, y + 0.5 + face.getStepY() * 0.5,
                            z + 0.5 + face.getStepZ() * 0.5), face, pos, false));
            LibertyCraft.LOG.info("[GroveCraft] Item use result {} soil {} above {} light {}", result,
                    player.level().getBlockState(pos), player.level().getBlockState(pos.above()), player.level().getRawBrightness(pos.above(), 0));
            if (!result.consumesAction() && player.getMainHandItem().get(net.minecraft.core.component.DataComponents.FOOD) != null)
                Link.pushEvent(37, player.getInventory().getSelectedSlot(),0,0,0,0,0);
            else if (!result.consumesAction()) GroveInteraction.use(player);
        }
        if (action != 2) LibertyCraft.LOG.info("[GroveCraft] action {} at {} item {}", action, pos, player.getMainHandItem());
    }
}
