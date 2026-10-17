package dev.libertycraft.client;

import dev.libertycraft.link.Link;
import net.minecraft.client.Minecraft;

/** Hold vanilla use through consumption; release after the actual item-use cycle finishes. */
public final class GroveClientUse {
    private static boolean held, sawUse, pending;
    private static boolean sustained;
    private static int expectedSlot;
    private static int originalCount;
    private static long deadline, pendingDeadline;
    private static boolean blocked(Minecraft mc){
        return !Link.active()||mc.player==null||mc.player.isSleeping()||mc.gui.screen()!=null
            ||HostClient.sky().menuOpen()||HostClient.sky().loading()
            ||(HostClient.sky().flags&((1<<4)|(1<<19)|(1<<21)|(1<<23)|(1<<24)))!=0;
    }
    public static void start(Minecraft minecraft, int slot) {
        if (blocked(minecraft)) return;
        if (slot < 0 || slot > 8) return;
        // Held-slot and inventory packets may arrive after the input event. Never use a stale block/tool.
        pending = true; expectedSlot = slot; pendingDeadline = System.nanoTime() + 750_000_000L;
        sustained = false;
    }
    public static void press(Minecraft minecraft) {
        if (blocked(minecraft) || !dev.libertycraft.GroveInteraction.sustained(minecraft.player.getMainHandItem())) return;
        expectedSlot = minecraft.player.getInventory().getSelectedSlot(); pending = true; sustained = true;
        pendingDeadline = System.nanoTime() + 750_000_000L;
    }
    public static void release(Minecraft minecraft) {
        if (!sustained) return;
        held = pending = sustained = false;
        minecraft.options.keyUse.setDown(false);
        if (minecraft.player != null) minecraft.player.stopUsingItem();
    }
    public static void frame(Minecraft minecraft) {
        if (pending) {
            if (blocked(minecraft) || System.nanoTime() > pendingDeadline) pending = false;
            else if (minecraft.player.getInventory().getSelectedSlot() == expectedSlot
                    && (sustained ? dev.libertycraft.GroveInteraction.sustained(minecraft.player.getMainHandItem())
                        : minecraft.player.getMainHandItem().get(net.minecraft.core.component.DataComponents.FOOD) != null)) {
                pending = false; held = true; sawUse = false; originalCount = minecraft.player.getMainHandItem().getCount();
                deadline = System.nanoTime() + (sustained ? 60_000_000_000L : 10_000_000_000L);
                if (sustained) minecraft.player.startUsingItem(net.minecraft.world.InteractionHand.MAIN_HAND);
            }
        }
        if (!held) return;
        if (minecraft.player != null && minecraft.player.isUsingItem()) sawUse = true;
        if (blocked(minecraft)
                || minecraft.player.getInventory().getSelectedSlot() != expectedSlot
                || minecraft.player.getMainHandItem().getCount() < originalCount
                || (!sustained && sawUse && !minecraft.player.isUsingItem()) || System.nanoTime() > deadline) {
            held = pending = false; minecraft.options.keyUse.setDown(false);
            if (minecraft.player != null) minecraft.player.stopUsingItem();
            sustained = false;
        } else minecraft.options.keyUse.setDown(true);
    }
}
