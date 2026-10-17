package dev.libertycraft;
import com.google.gson.Gson;
import com.mojang.serialization.Codec;
import dev.libertycraft.link.*;
import net.fabricmc.fabric.api.attachment.v1.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.minecraft.resources.Identifier;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.item.*;
import net.minecraft.network.chat.Component;
import net.minecraft.world.*;
import net.minecraft.world.inventory.*;
import net.minecraft.world.entity.player.Player;
import net.minecraft.core.component.DataComponents;
import net.minecraft.world.item.component.ItemLore;
import java.util.*;

/** Material escrow survives reconnect/save. Only a definitive native receipt consumes or returns it. */
public final class GroveRoadside {
    private GroveRoadside(){}
    private static final Gson JSON=new Gson();
    private static final AttachmentType<String> STATE=AttachmentRegistry.<String>builder().persistent(Codec.STRING).copyOnDeath()
        .buildAndRegister(Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"roadside_v1"));
    private static final AttachmentType<List<ItemStack>> ESCROW=AttachmentRegistry.<List<ItemStack>>builder().persistent(ItemStack.OPTIONAL_CODEC.listOf()).copyOnDeath()
        .buildAndRegister(Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"roadside_escrow_v1"));
    private static final AttachmentType<List<ItemStack>> RETURNS=AttachmentRegistry.<List<ItemStack>>builder().persistent(ItemStack.OPTIONAL_CODEC.listOf()).copyOnDeath()
        .buildAndRegister(Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"roadside_returns_v1"));
    static final class State {String token;int next=1,id,vehicle,status=9;}
    private record Work(int vehicle,int progress,int misses){}
    private record Access(int vehicle,int kind,int expires){}
    private static final Map<UUID,Work> work=new HashMap<>();
    private static final Map<UUID,Access> access=new HashMap<>();
    private static final Map<UUID,Integer> sent=new HashMap<>();
    private static State state(ServerPlayer p){
        String raw=p.getAttached(STATE);State s=raw==null?new State():JSON.fromJson(raw,State.class);
        if(s.token==null)s.token=UUID.randomUUID().toString().replace("-","").substring(0,12);
        if(!s.token.matches("[0-9a-f]{12}")||s.next<1||s.id<0||s.id>s.next)throw new IllegalStateException("Invalid repair receipt");if(raw==null)store(p,s);return s;
    }
    private static void store(ServerPlayer p,State s){p.setAttached(STATE,JSON.toJson(s));}
    private static void say(ServerPlayer p,String s){p.sendSystemMessage(Component.literal(s));}
    public static boolean busy(ServerPlayer p){State s=state(p);return work.containsKey(p.getUUID())||(s.id!=0&&s.status!=-2);}
    private static ItemStack button(Item item,String name,String detail){var s=new ItemStack(item);s.set(DataComponents.CUSTOM_NAME,Component.literal(name));s.set(DataComponents.LORE,new ItemLore(List.of(Component.literal(detail))));return s;}
    public static void open(ServerPlayer p,int vehicle){
        if(!GroveCargo.eligible(p,vehicle))return;
        var contents=new SimpleContainer(9);
        contents.setItem(2,button(Items.IRON_INGOT,"Reparar motor · 3 hierro + 1 redstone","5 segundos; +250 estado, máximo 90%. Materiales de la mochila."));
        contents.setItem(4,button(Items.CHEST,"Equipaje de expedición","Guarda recursos o saca materiales para reparar."));
        contents.setItem(6,button(Items.CRAFTING_TABLE,"Taller móvil","Necesita una mesa guardada en el equipaje."));
        p.openMenu(new SimpleMenuProvider((id,inv,who)->new ChestMenu(MenuType.GENERIC_9x1,id,inv,contents,1){
            @Override public boolean stillValid(Player who){return who==p&&GroveCargo.eligible(p,vehicle);}
            @Override public ItemStack quickMoveStack(Player who,int index){return ItemStack.EMPTY;}
            @Override public void clicked(int index,int mouse,ContainerInput type,Player who){
                if(who!=p||!stillValid(who)||mouse!=0||type!=ContainerInput.PICKUP||(index!=2&&index!=4&&index!=6))return;
                p.closeContainer();if(index==2)start(p,vehicle);else access.put(p.getUUID(),new Access(vehicle,index,p.level().getServer().getTickCount()+40));
            }
        },Component.literal("Asistencia de carretera")));
    }
    public static void start(ServerPlayer p,int vehicle){
        if(!GroveCargo.eligible(p,vehicle)||!p.isAlive())return;
        State s=state(p);if(s.id!=0||work.containsKey(p.getUUID())){say(p,"La reparación anterior sigue pendiente.");return;}
        if(GroveTownEconomy.count(p,Items.IRON_INGOT)<3||GroveTownEconomy.count(p,Items.REDSTONE)<1){say(p,"Reparación: 3 lingotes de hierro + 1 redstone en tu mochila. Saca materiales con G.");return;}
        work.put(p.getUUID(),new Work(vehicle,0,0));Link.pushEvent(53,1,0,0,0,0,0);say(p,"Reparando motor… permanece junto a la cajuela 5 segundos.");
    }
    private static void reserve(ServerPlayer p,State s,int vehicle){
        if(GroveTownEconomy.count(p,Items.IRON_INGOT)<3||GroveTownEconomy.count(p,Items.REDSTONE)<1)return;
        var items=new ArrayList<ItemStack>();
        for(var item:List.of(Items.IRON_INGOT,Items.REDSTONE)){
            int need=item==Items.IRON_INGOT?3:1;
            for(int i=0;i<36&&need>0;i++){var stack=p.getInventory().getItem(i);if(stack.is(item)){int n=Math.min(need,stack.getCount());items.add(stack.split(n));need-=n;}}
        }
        p.setAttached(ESCROW,List.copyOf(items));s.id=s.next;s.vehicle=vehicle;s.status=9;store(p,s);
        p.inventoryMenu.broadcastChanges();p.level().getServer().getPlayerList().saveAll();
    }
    public static void receive(ServerPlayer p,int id,int result,int signature){
        State s=state(p);int lo=Integer.parseInt(s.token.substring(6),16),hi=Integer.parseInt(s.token.substring(0,6),16);
        if(s.id!=id||signature!=(lo^hi)||(result!=0&&result!=1&&result!=-2))return;
        if(result==-2){if(s.status!=-2)say(p,"Recibo de reparación sin confirmar: materiales protegidos en reserva. No se repetirá el efecto a ciegas.");s.status=-2;store(p,s);return;}
        if(result==0){var old=p.getAttached(RETURNS);var back=new ArrayList<ItemStack>();if(old!=null)back.addAll(old);var escrow=p.getAttached(ESCROW);if(escrow!=null)back.addAll(escrow);p.setAttached(RETURNS,List.copyOf(back));}
        p.setAttached(ESCROW,List.of());s.next++;s.id=0;s.vehicle=0;s.status=9;store(p,s);
        say(p,result==1?"Motor reparado: +250 de estado, máximo 90%. La carrocería se arregla en el taller de GTA.":"Reparación cancelada: coche no apto o ya está al 90%. Materiales devueltos.");
        p.level().getServer().getPlayerList().saveAll();
    }
    public static void register(){
        net.fabricmc.fabric.api.event.lifecycle.v1.ServerLifecycleEvents.SERVER_STOPPING.register(s->{work.clear();access.clear();sent.clear();});
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(!Link.active()||Link.hostSuspended()||server.getTickCount()%4!=0)return;
            for(var p:server.getPlayerList().getPlayers())try{
                var sky=new Link.SkyState();if(!Link.readSkyState(sky))continue;
                // Menu flag can precede the suspension latch by one frame. Freeze
                // preparation even during that transition instead of cancelling it.
                if((sky.flags&Proto.SKY_MENU_OPEN)!=0)continue;
                if((sky.flags&(1<<21))!=0&&(sky.flags&(1<<22))==0&&p.isAlive())continue;
                if(!GroveCargoRules.safeFlags(sky.flags,true)||!p.isAlive()||p.isSleeping()){
                    if(work.containsKey(p.getUUID()))LibertyCraft.LOG.info("[GroveCraft] Repair cancelled by host flags {} alive {}",sky.flags,p.isAlive());
                    work.remove(p.getUUID());access.remove(p.getUUID());Link.pushEvent(53,0,0,0,0,0,0);continue;}
                var a=access.get(p.getUUID());if(a!=null&&server.getTickCount()>a.expires){access.remove(p.getUUID());a=null;}
                if((sky.flags&(1<<19))!=0)continue;
                if(a!=null){access.remove(p.getUUID());if(a.kind==4)GroveCargo.open(p,a.vehicle);else GroveCargo.workshop(p,a.vehicle);continue;}
                var returns=p.getAttached(RETURNS);if(returns!=null&&!returns.isEmpty()){
                    var remaining=new ArrayList<ItemStack>();for(var source:returns){var item=source.copy();p.getInventory().add(item);if(!item.isEmpty())remaining.add(item);}p.setAttached(RETURNS,List.copyOf(remaining));
                }
                var w=work.get(p.getUUID());
                if(w!=null){
                    // A resumed GTA frame can precede its refreshed actor lease.
                    // Wait up to one active second without progress or material cost.
                    if(!GroveCargo.eligible(p,w.vehicle)&&w.misses<20){work.put(p.getUUID(),new Work(w.vehicle,w.progress,w.misses+4));Link.pushEvent(53,1,w.progress,0,0,0,0);continue;}
                    if(!GroveCargo.eligible(p,w.vehicle)||p.isUsingItem()||GroveGameplay.miningPosition()!=null){
                        LibertyCraft.LOG.info("[GroveCraft] Repair cancelled: eligible {} using {} mining {}",GroveCargo.eligible(p,w.vehicle),p.isUsingItem(),GroveGameplay.miningPosition());
                        work.remove(p.getUUID());Link.pushEvent(53,0,0,0,0,0,0);say(p,"Reparación interrumpida; no se gastaron materiales.");}
                    else {int progress=Math.min(100,w.progress+4);work.put(p.getUUID(),new Work(w.vehicle,progress,0));Link.pushEvent(53,1,progress,0,0,0,0);
                        if(progress>=100){work.remove(p.getUUID());reserve(p,state(p),w.vehicle);}}
                }
                State s=state(p);
                if(s.id!=0){Link.pushEvent(53,s.status==-2?3:2,100,0,0,0,0);
                    if(s.status!=-2&&server.getTickCount()-sent.getOrDefault(p.getUUID(),-100)>=20){
                        int lo=Integer.parseInt(s.token.substring(6),16),hi=Integer.parseInt(s.token.substring(0,6),16);
                        Link.pushEvent(52,s.id,lo,hi,s.vehicle,0,0);sent.put(p.getUUID(),server.getTickCount());}
                }else if(!work.containsKey(p.getUUID()))Link.pushEvent(53,0,0,0,0,0,0);
                if(Boolean.getBoolean("grovecraft.roadsideAcceptance")&&server.getTickCount()%20==0){
                    var j=new com.google.gson.JsonObject();j.addProperty("iron",GroveTownEconomy.count(p,Items.IRON_INGOT));j.addProperty("redstone",GroveTownEconomy.count(p,Items.REDSTONE));
                    j.addProperty("pending",s.id);j.addProperty("status",s.status);j.addProperty("next",s.next);j.addProperty("working",work.containsKey(p.getUUID()));
                    var escrow=p.getAttached(ESCROW);j.addProperty("escrow",escrow==null?0:escrow.stream().mapToInt(ItemStack::getCount).sum());
                    try{GroveCampaignFiles.atomic(java.nio.file.Path.of("grovecraft-roadside-test.json").toAbsolutePath(),j.toString());}catch(java.io.IOException e){throw new IllegalStateException(e);}
                }
            }catch(RuntimeException e){LibertyCraft.LOG.error("Roadside receipt held safely",e);}
        });
    }
}
