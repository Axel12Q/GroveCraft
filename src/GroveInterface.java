package dev.libertycraft.client;

import dev.libertycraft.link.Link;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.screens.inventory.InventoryScreen;
import net.minecraft.client.gui.screens.inventory.AbstractContainerScreen;
import dev.libertycraft.client.mixin.GroveContainerMixin;
import dev.libertycraft.client.mixin.GroveBookMixin;

/** Report the real container window, and use the normal client menu lifecycle. */
public final class GroveInterface {
    private static long nextObservation;
    private static boolean distanceReady;
    public static void inventory(Minecraft mc) {
        if (mc.player == null) return;
        InputBridge.releaseAll();
        if(mc.player.isSleeping()){
            mc.player.connection.send(new net.minecraft.network.protocol.game.ServerboundPlayerCommandPacket(mc.player,
                net.minecraft.network.protocol.game.ServerboundPlayerCommandPacket.Action.STOP_SLEEPING));
            mc.gui.setScreen(null);return;
        }
        if (mc.gui.screen() instanceof AbstractContainerScreen<?>) mc.player.closeContainer();
        else if (mc.gui.screen() instanceof GroveBookMixin) mc.gui.setScreen(null);
        else if (mc.gui.screen() == null) mc.gui.setScreen(new InventoryScreen(mc.player));
    }
    public static void frame(Minecraft mc) {
        if(!distanceReady&&Link.active()&&mc.level!=null){
            if(mc.options.renderDistance().get()<12)mc.options.renderDistance().set(12);
            distanceReady=true;
        }
        var screen = mc.gui.screen();
        if ((HostClient.sky().flags & (1 << 21)) != 0) {
            if (screen instanceof AbstractContainerScreen<?> && mc.player != null) mc.player.closeContainer();
            else if (screen instanceof GroveBookMixin) mc.gui.setScreen(null);
            Link.pushEvent(36, 0, 0,0,0,0,0);
            return;
        }
        if(mc.player!=null&&mc.player.isSleeping()){
            // Vanilla sleep screen stays interactive without pausing the night clock.
            Link.pushEvent(36,2,0,0,mc.getWindow().getWidth(),mc.getWindow().getHeight(),0);
        } else if (screen instanceof AbstractContainerScreen<?> && screen instanceof GroveContainerMixin layout) {
            int scale = mc.getWindow().getGuiScale();
            boolean recipes=screen instanceof dev.libertycraft.client.mixin.GroveRecipeBookMixin book&&book.groveRecipeBook().isVisible();
            int extra=recipes?178:0;
            Link.pushEvent(36, 1, (layout.groveLeft()-extra)*scale, layout.groveTop()*scale,
                    (layout.groveWidth()+extra)*scale, layout.groveHeight()*scale, 0);
        } else if (screen instanceof GroveBookMixin book) {
            int scale = mc.getWindow().getGuiScale();
            // Vanilla book: 192px page plus the centered 200px Done button.
            Link.pushEvent(36, 1, (book.groveBookLeft()-4)*scale, book.groveBookTop()*scale,
                    200*scale, (book.groveBookControlsTop()+20-book.groveBookTop())*scale, 0);
        } else Link.pushEvent(36, 0, 0,0,0,0,0);
        if (mc.player != null && System.nanoTime() > nextObservation) {
            nextObservation = System.nanoTime() + 1_000_000_000L;
            var report = new com.google.gson.JsonObject();
            report.addProperty("screen",screen == null ? "none" : screen.getClass().getSimpleName());
            report.addProperty("held",mc.player.getMainHandItem().toString());
            report.addProperty("armor",mc.player.getArmorValue());
            var equipment=new com.google.gson.JsonObject();
            for(var slot:net.minecraft.world.entity.EquipmentSlot.values()) if(slot.isArmor())
                equipment.addProperty(slot.getName(),mc.player.getItemBySlot(slot).toString());
            report.add("equipment",equipment);
            var slots = new com.google.gson.JsonArray();
            for (var slot : mc.player.containerMenu.slots) {
                var row = new com.google.gson.JsonObject(); row.addProperty("index",slot.index);
                row.addProperty("x",slot.x);row.addProperty("y",slot.y);row.addProperty("item",slot.getItem().toString());slots.add(row);
            }
            report.add("slots",slots);
            try { java.nio.file.Files.writeString(java.nio.file.Path.of("grovecraft-interface.json"),report.toString()); }
            catch (java.io.IOException ignored) {}
        }
    }
}
