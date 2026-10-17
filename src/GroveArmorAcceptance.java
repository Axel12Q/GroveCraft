package dev.libertycraft;
import java.nio.file.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.world.entity.EquipmentSlot;
import net.minecraft.world.item.*;
import net.minecraft.world.level.storage.LevelResource;

/** Actual Minecraft equip/defence/wear/save, only in an explicitly named private fixture. */
public final class GroveArmorAcceptance {
    private static boolean finished;
    private static int equippedTick;
    private static void require(boolean ok,String message){if(!ok)throw new IllegalStateException(message);}
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.armorAcceptance"))return;
        require(LibertyCraft.WORLD_NAME.equals("GroveCraftGtaTestArmorAcceptance"),"Isolated armor world required");
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(finished||server.getTickCount()<100||server.getPlayerList().getPlayers().isEmpty())return;
            var player=server.getPlayerList().getPlayers().getFirst();
            Path marker=server.getWorldPath(LevelResource.ROOT).resolve("armor-acceptance-marker");
            Path report=FabricLoader.getInstance().getGameDir().resolve("grovecraft-armor-acceptance.json");
            boolean reload=Files.exists(marker);
            try{
                var out=new com.google.gson.JsonObject();out.addProperty("reload",reload);
                if(!reload){
                    var slots=new EquipmentSlot[]{EquipmentSlot.HEAD,EquipmentSlot.CHEST,EquipmentSlot.LEGS,EquipmentSlot.FEET};
                    var items=new Item[]{Items.IRON_HELMET,Items.IRON_CHESTPLATE,Items.IRON_LEGGINGS,Items.IRON_BOOTS};
                    var source=player.damageSources().mobAttack(player);
                    var protection=(dev.libertycraft.mixin.GroveArmorMixin)(Object)player;
                    if(equippedTick==0){
                        for(var slot:slots)player.setItemSlot(slot,ItemStack.EMPTY);
                        // Real menu transfer; attributes update in the normal following living tick.
                        for(int i=0;i<4;i++){
                            player.getInventory().setItem(i,new ItemStack(items[i]));
                            player.inventoryMenu.quickMoveStack(player,36+i);
                            require(player.getItemBySlot(slots[i]).is(items[i]),"Shift equip routes to proper armor slot");
                            require(player.getInventory().getItem(i).isEmpty(),"Equip consumes source stack without duplication");
                        }
                        equippedTick=server.getTickCount();return;
                    }
                    if(server.getTickCount()-equippedTick<4)return;
                    float bare=8;
                    require(player.getArmorValue()==15,"Vanilla full iron attributes");
                    float defended=protection.groveArmor(source,8);
                    require(defended>0&&defended<bare,"Vanilla armor reduces damage");
                    require(player.getItemBySlot(EquipmentSlot.HEAD).getDamageValue()>0,"Vanilla damage wears equipment");
                    out.addProperty("bare_damage",bare);out.addProperty("armored_damage",defended);
                    // Unequip through normal menu, then re-equip and save the worn item.
                    player.inventoryMenu.quickMoveStack(player,5);
                    require(player.getItemBySlot(EquipmentSlot.HEAD).isEmpty(),"Unequip removes mesh source");
                    int index=-1;for(int i=0;i<36;i++)if(player.getInventory().getItem(i).is(Items.IRON_HELMET))index=i;
                    require(index>=0,"Unequip conserves helmet");
                    player.inventoryMenu.quickMoveStack(player,index<9?36+index:index);
                    require(player.getItemBySlot(EquipmentSlot.HEAD).is(Items.IRON_HELMET),"Re-equip worn helmet");
                    Files.writeString(marker,"worn iron armor");
                }else{
                    require(player.getArmorValue()==15,"Armor attributes survive full reload");
                    require(player.getItemBySlot(EquipmentSlot.HEAD).is(Items.IRON_HELMET)
                        &&player.getItemBySlot(EquipmentSlot.HEAD).getDamageValue()>0,"Worn helmet survives reload");
                }
                finished=true;player.closeContainer();server.saveEverything(false,true,false);
                out.addProperty("armor",player.getArmorValue());out.addProperty("helmet_wear",player.getItemBySlot(EquipmentSlot.HEAD).getDamageValue());
                out.addProperty("passed",true);Files.writeString(report,out.toString());
                LibertyCraft.LOG.info("[GroveCraft] Armor acceptance passed: {}",out);
            }catch(Throwable e){try{Files.writeString(report,"{\"passed\":false,\"error\":"+new com.google.gson.Gson().toJson(e.toString())+"}");}catch(Exception ignored){}throw new RuntimeException(e);}
        });
    }
}
