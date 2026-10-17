package dev.libertycraft.mixin;

import dev.libertycraft.GroveInteraction;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.server.network.ServerGamePacketListenerImpl;
import net.minecraft.network.protocol.game.ServerboundPlayerActionPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/** GTA's hold/release is authoritative only during its explicit, timeout-bounded item-use lease. */
@Mixin(ServerGamePacketListenerImpl.class)
public abstract class GroveUseMixin {
    @Shadow public ServerPlayer player;
    @Inject(method="handlePlayerAction",at=@At("HEAD"),cancellable=true)
    private void grove$hostRelease(ServerboundPlayerActionPacket packet, CallbackInfo ci) {
        // The mirror's physical mouse is released even while GTA's RMB is held.
        // Never let that unrelated client release prematurely fire the server bow.
        if (packet.getAction() == ServerboundPlayerActionPacket.Action.RELEASE_USE_ITEM
                && GroveInteraction.owns(player)) ci.cancel();
    }
}
