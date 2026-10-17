package dev.libertycraft;
import com.mojang.brigadier.arguments.IntegerArgumentType;
import dev.libertycraft.link.Link;
import dev.libertycraft.world.HostDig;
import net.fabricmc.fabric.api.command.v2.CommandRegistrationCallback;
import net.minecraft.commands.Commands;
import net.minecraft.core.BlockPos;
import net.minecraft.network.chat.Component;
/** Isolated acceptance scene; marks a cut through the real persistent chunk attachment. */
public final class GroveNativeFellingTest {
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.fellingAcceptance")||!System.getProperty("grovecraft.testWorld","").startsWith("GroveCraftGtaTestFelling"))return;
        CommandRegistrationCallback.EVENT.register((dispatcher,access,environment)->dispatcher.register(Commands.literal("grovegrowtest")
            .executes(ctx->GroveNativeTrees.ageAcceptance(ctx.getSource().getPlayerOrException()))));
        CommandRegistrationCallback.EVENT.register((dispatcher,access,environment)->dispatcher.register(Commands.literal("grovefell")
            .then(Commands.argument("x",IntegerArgumentType.integer(-4000,4000)).then(Commands.argument("y",IntegerArgumentType.integer(-100,1000))
            .then(Commands.argument("z",IntegerArgumentType.integer(-4000,4000)).executes(ctx->{
                var p=ctx.getSource().getPlayerOrException();var sky=new Link.SkyState();
                if(!Link.active()||!Link.readSkyState(sky)||!GroveEcologyRules.safe(sky.flags,sky.worldId,sky.inGame(),sky.loading()))return 0;
                int x=IntegerArgumentType.getInteger(ctx,"x"),y=IntegerArgumentType.getInteger(ctx,"y"),z=IntegerArgumentType.getInteger(ctx,"z");
                var root=new BlockPos(x,y,z);if(p.distanceToSqr(x+.5,y+.5,z+.5)>100)return 0;
                int radius=LibertyCraft.WORLD_NAME.endsWith("Runtime")?0:1;
                int count=0;for(int dx=-radius;dx<=radius;dx++)for(int dz=-radius;dz<=radius;dz++)
                    if(HostDig.markDug(p.level(),sky.worldId+256,root.offset(dx,0,dz)))count++;
                ctx.getSource().sendSuccess(()->Component.literal("Native felling fixture cut recorded"),false);
                LibertyCraft.LOG.info("[GroveCraft] native felling fixture at {}: {} new dug cells",root,count);return count;
            }))))));
    }
}
