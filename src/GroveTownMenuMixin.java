package dev.libertycraft.client.mixin;
import dev.libertycraft.link.Link;
import net.minecraft.client.Minecraft;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.inventory.*;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.*;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
/** Board icons are buttons. Send the normal click packet without locally taking its icon. */
@Mixin(AbstractContainerMenu.class)
public class GroveTownMenuMixin {
    @Inject(method="clicked",at=@At("HEAD"),cancellable=true)
    private void groveTownButton(int slot,int button,ContainerInput type,Player player,CallbackInfo ci){
        var minecraft=Minecraft.getInstance();var screen=minecraft.gui.screen();
        if(Link.active()&&player==minecraft.player&&screen!=null
            &&screen.getTitle().getString().equals("Mercado GTA · encargos"))ci.cancel();
    }
}
