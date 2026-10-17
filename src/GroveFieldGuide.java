package dev.libertycraft;

import java.util.*;
import net.fabricmc.fabric.api.command.v2.CommandRegistrationCallback;
import net.fabricmc.fabric.api.networking.v1.ServerPlayConnectionEvents;
import net.minecraft.commands.Commands;
import net.minecraft.core.component.DataComponents;
import net.minecraft.network.chat.Component;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.server.network.Filterable;
import net.minecraft.world.item.*;
import net.minecraft.world.item.component.WrittenBookContent;

/** A small expedition start and a readable in-game guide. Never resets an existing inventory. */
public final class GroveFieldGuide {
    private GroveFieldGuide() {}
    static ItemStack book() {
        String[] text={
            "SAN ANDREAS\nCuaderno de campo\n\nCJ sigue su historia. Entre misiones, el campo ofrece madera, comida y refugios. La noche trae peligros. Explora de día y lleva provisiones.",
            "PRIMERA SALIDA\n\nBusca campo en Red County o Flint County. La exploración prepara parcelas lejanas también al conducir. El primer sondeo puede tardar. No aparecen aldeas en ciudades ni aeropuertos.",
            "VIVIR DEL CAMPO\n\nTala madera y fabrica mesa, palos y herramientas. Las hojas pueden soltar retoños. Planta en tierra de Minecraft. Consigue piedra y carbón al excavar; usa el pico adecuado.",
            "COMIDA Y AGUA\n\nLas granjas tienen trigo, zanahorias y patatas. Replanta tras cosechar. Alimenta y cría animales para tener reservas. Cocina en el horno. Come para recuperar hambre y regenerar salud.",
            "TALLER MÓVIL\n\nG detrás de un coche estacionado abre tu equipaje. B abre su cajuela física. Guarda allí una mesa de crafteo y usa Alt + G para fabricar con la cuadrícula 3x3. Las recetas consumen materiales normales; la mesa se conserva. Cierra antes de conducir. Aldeas y proveedores permiten vender cosechas y comprar suministros.",
            "COMERCIO Y PELIGRO\n\nAldeas: vende cosechas y compra comida. Proveedores: madera, carbón e hierro por esmeraldas; herramientas y suministros. Trabajan para reponer existencias. Paradas comerciales y torres de saqueadores tienen botín finito. Transporta provisiones en el equipaje del coche.",
            "TU CAMPAÑA, TU MUNDO\n\nNueva partida de GTA crea un mundo Minecraft nuevo. Guardar en GTA guarda ambos juntos; cargar esa partida recupera sus bloques, inventario y recursos. F6 es una copia Minecraft adicional: no sustituye guardar GTA.",
            "MERCADOS GTA\n\nJ junto al icono de un aldeano abre compras y encargos en dólares GTA. También funciona Shift + clic derecho. Clic derecho normal conserva el comercio con esmeraldas. Libera espacio para recibir compras pendientes. Después de comerciar, guarda en GTA para conservar juntos el dinero y tus recursos.",
            "EXPLORACIÓN GUIADA\n\nAl conducir: aguja de velocidad, silueta de daño del coche y barra de aceleración/frenada. Un encargo activo muestra dirección, distancia y materiales en tu mochila. J usa mesa, horno, cama y contenedores señalados. Sus reglas de Minecraft siguen funcionando.\n",
            "ASISTENCIA DE CARRETERA\n\nAlt+B junto a una cajuela abre reparación, equipaje y taller. Motor: 3 hierro + 1 redstone de tu mochila, 5 segundos sin alejarte ni minar. +250 estado, máximo 90%. No salva coches ardiendo ni arregla carrocería. Mercados venden materiales; G permite sacarlos del equipaje.",
            "MAPA DE EXPEDICIÓN\n\nLas estructuras generadas quedan señaladas y se guardan en tu mundo: casa verde = aldea; bandera = saqueadores; camión = parada comercial; $ = granja; cubiertos = campamento; ? = ruinas. El minimapa muestra las cercanas. Abre el mapa de pausa para ver las lejanas. Hasta 64 puntos más próximos, dejando espacio para las misiones GTA. /grovecraft world map recuerda los símbolos.",
            "CONTROLES\n\n1–9 y rueda: objeto. Izquierdo: minar o atacar. Derecho: colocar, usar, comer o interactuar. E: inventario. En una misión GTA se restringe la construcción. /grovecraft world status muestra generación."
        };
        List<Filterable<Component>> pages=Arrays.stream(text).map(Component::literal).map(c -> Filterable.<Component>passThrough(c)).toList();
        ItemStack stack=new ItemStack(Items.WRITTEN_BOOK);
        stack.set(DataComponents.WRITTEN_BOOK_CONTENT,new WrittenBookContent(Filterable.passThrough("Cuaderno de campo"),"GroveCraft",0,pages,true));
        return stack;
    }
    private static boolean give(ServerPlayer player,ItemStack item) {
        for(int slot=0;slot<36;slot++) if(player.getInventory().getItem(slot).isEmpty()) {
            player.getInventory().setItem(slot,item);player.inventoryMenu.broadcastChanges();return true;
        }
        return false;
    }
    public static void register() {
        CommandRegistrationCallback.EVENT.register((dispatcher,registry,environment) -> dispatcher.register(
            Commands.literal("grovecraft").then(Commands.literal("guide").executes(c -> {
                if(give(c.getSource().getPlayerOrException(),book()))return 1;
                c.getSource().sendFailure(Component.literal("Deja un espacio libre para el cuaderno."));return 0;
            }))));
        ServerPlayConnectionEvents.JOIN.register((handler,sender,server) -> {
            ServerPlayer player=handler.getPlayer();
            if(!Boolean.getBoolean("grovecraft.expedition")||player.entityTags().contains("grovecraft_expedition_start"))return;
            // Refuse to add supplies to an existing/foreign inventory; this profile has its own world.
            boolean empty=true;for(int slot=0;slot<36;slot++)empty &= player.getInventory().getItem(slot).isEmpty();
            if(empty) { give(player,book());give(player,new ItemStack(Items.BREAD,3)); }
            player.addTag("grovecraft_expedition_start");
        });
    }
}
