package dev.libertycraft;

import dev.libertycraft.link.Link;
import dev.libertycraft.link.Proto;
import dev.libertycraft.world.HostClip;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.InteractionResult;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.phys.Vec3;
import net.minecraft.world.phys.BlockHitResult;
import net.minecraft.world.level.ClipContext;
import net.minecraft.world.phys.HitResult;

/** Server-owned creature targeting and press/hold/release item use. Never trusts entity IDs from GTA. */
public final class GroveInteraction {
    private GroveInteraction() {}
    private record Aim(Vec3 point, long time) {}
    private record Held(int slot, ItemStack item) {}
    private static final java.util.Map<java.util.UUID, Aim> aims = new java.util.HashMap<>();
    private static final java.util.Map<java.util.UUID, Held> held = new java.util.concurrent.ConcurrentHashMap<>();
    public static Vec3 observedAim(ServerPlayer p){
        var a=aims.get(p.getUUID());return a!=null&&System.nanoTime()-a.time<500_000_000L?a.point:null;
    }
    public static void reset() { aims.clear(); held.clear(); }
    public static boolean owns(ServerPlayer player) {
        return Link.active() && held.containsKey(player.getUUID());
    }
    public static boolean safe(Link.SkyState host) {
        return Link.active() && (host.flags & Proto.SKY_IN_GAME) != 0
                && (host.flags & (Proto.SKY_LOADING | Proto.SKY_MENU_OPEN | Proto.SKY_IN_VEHICLE
                      | (1 << 16) | (1 << 19) | (1 << 21) | (1 << 23) | (1 << 24))) == 0;
    }
    public static void aim(ServerPlayer player, int x, int y, int z) {
        Vec3 point = new Vec3(x / 256., y / 256., z / 256.);
        if (point.distanceToSqr(player.getEyePosition()) <= 4096)
            aims.put(player.getUUID(), new Aim(point, System.nanoTime()));
    }
    private static Vec3 direction(ServerPlayer player) {
        Aim aim = aims.get(player.getUUID());
        if (aim != null && System.nanoTime() - aim.time < 500_000_000L) {
            Vec3 delta = aim.point.subtract(player.getEyePosition());
            if (delta.lengthSqr() > .01) return delta.normalize();
        }
        return player.getLookAngle();
    }
    public static void orient(ServerPlayer player) {
        Vec3 dir = direction(player);
        float yaw = (float)Math.toDegrees(Math.atan2(-dir.x, dir.z));
        player.setYRot(yaw); player.setYHeadRot(yaw);
        player.setXRot((float)Math.toDegrees(-Math.asin(Math.clamp(dir.y, -1, 1))));
    }
    /** The first intersected living creature in normal survival reach; walls always win. */
    public static LivingEntity target(ServerPlayer player) {
        Vec3 from = player.getEyePosition();
        double reach = Math.min(3, player.entityInteractionRange());
        Vec3 end = from.add(direction(player).scale(reach));
        var block = player.level().clip(new ClipContext(from, end, ClipContext.Block.COLLIDER, ClipContext.Fluid.NONE, player));
        double limit = block.getType() == HitResult.Type.MISS ? reach * reach : from.distanceToSqr(block.getLocation());
        var nativeHit = HostClip.cast(from, end);
        if (nativeHit != null) limit = Math.min(limit, reach * reach * nativeHit.t() * nativeHit.t());
        LivingEntity target = null;
        for (var entity : player.level().getEntitiesOfClass(LivingEntity.class,
                player.getBoundingBox().inflate(reach), e -> e != player && e.isAlive()
                    && !(e instanceof dev.libertycraft.combat.HostActorEntity))) {
            var hit = entity.getBoundingBox().inflate(.1).clip(from, end);
            if (hit.isEmpty()) continue;
            double distance = from.distanceToSqr(hit.get());
            if (distance + .001 < limit) { limit = distance; target = entity; }
        }
        return target;
    }
    public static BlockHitResult blockTarget(ServerPlayer player) {
        Vec3 from=player.getEyePosition(),end=from.add(direction(player).scale(4.5));
        var hit=player.level().clip(new ClipContext(from,end,ClipContext.Block.OUTLINE,ClipContext.Fluid.NONE,player));
        if(hit.getType()==HitResult.Type.MISS)return null;
        var nativeHit=HostClip.cast(from,end);
        if(nativeHit!=null&&from.distanceTo(hit.getLocation())>4.5*nativeHit.t()+.08)return null;
        return hit;
    }
    public static boolean creature(ServerPlayer player) {
        LivingEntity target = target(player);
        if (target == null) return false;
        if(GroveTownEconomy.enabled()&&player.isShiftKeyDown()&&target instanceof net.minecraft.world.entity.npc.villager.Villager v
            &&v.entityTags().contains("grovecraft_settler"))return GroveTownEconomy.open(player,v);
        var result = player.interactOn(target, InteractionHand.MAIN_HAND,
                target.getBoundingBox().getCenter().subtract(target.position()));
        if (result.consumesAction()) {
            LibertyCraft.LOG.info("[GroveCraft] Creature interaction {} item {} result {}", target.getType().toShortString(), player.getMainHandItem(), result);
            Link.pushEvent(39, 0, 0, 0, 0, 0, 0);
        }
        // Block placement never passes through a clicked creature. Other items
        // may fall back to air use (raise a shield, draw a bow, eat) after PASS.
        return result.consumesAction() || player.getMainHandItem().getItem() instanceof net.minecraft.world.item.BlockItem;
    }
    public static boolean sustained(ItemStack stack) {
        return stack.getItem() instanceof net.minecraft.world.item.ProjectileWeaponItem
                || stack.is(Items.TRIDENT)
                || stack.get(net.minecraft.core.component.DataComponents.BLOCKS_ATTACKS) != null;
    }
    public static void use(ServerPlayer player) {
        var stack = player.getMainHandItem();
        if (stack.get(net.minecraft.core.component.DataComponents.FOOD) != null) {
            Link.pushEvent(37, player.getInventory().getSelectedSlot(), 0, 0, 0, 0, 0);
            return;
        }
        if (player.isUsingItem() || stack.isEmpty()) return;
        orient(player);
        var result = player.gameMode.useItem(player, player.level(), stack, InteractionHand.MAIN_HAND);
        LibertyCraft.LOG.info("[GroveCraft] Held use start item {} result {} using {}",stack,result,player.isUsingItem());
        if (result.consumesAction() && sustained(stack) && player.isUsingItem())
            held.put(player.getUUID(), new Held(player.getInventory().getSelectedSlot(), stack.copy()));
    }
    public static void finish(ServerPlayer player, boolean release) {
        if (held.remove(player.getUUID()) == null) return;
        LibertyCraft.LOG.info("[GroveCraft] Held use finish release {} using {} ticks {}",release,player.isUsingItem(),player.getTicksUsingItem());
        if (release) { orient(player); player.releaseUsingItem(); }
        else player.stopUsingItem();
    }
    public static void tick(ServerPlayer player, Link.SkyState host) {
        Held use = held.get(player.getUUID());
        if (use == null) return;
        Aim aim = aims.get(player.getUUID());
        if (!safe(host) || aim == null || System.nanoTime() - aim.time > 500_000_000L
                || use.slot != player.getInventory().getSelectedSlot()
                || !ItemStack.isSameItem(use.item, player.getMainHandItem()) || !player.isAlive()) {
            finish(player, false); return;
        }
        if (!player.isUsingItem()) { LibertyCraft.LOG.info("[GroveCraft] Held use stopped externally"); held.remove(player.getUUID()); return; }
        orient(player);
    }
}
