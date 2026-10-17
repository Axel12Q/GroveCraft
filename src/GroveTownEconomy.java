package dev.libertycraft;

import com.google.gson.Gson;
import com.mojang.serialization.Codec;
import dev.libertycraft.link.Link;
import net.fabricmc.fabric.api.attachment.v1.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.*;
import net.minecraft.core.component.DataComponents;
import net.minecraft.core.registries.*;
import net.minecraft.network.chat.Component;
import net.minecraft.resources.Identifier;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.tags.ItemTags;
import net.minecraft.world.*;
import net.minecraft.world.entity.npc.villager.Villager;
import net.minecraft.world.entity.player.*;
import net.minecraft.world.inventory.*;
import net.minecraft.world.item.*;
import net.minecraft.world.item.component.ItemLore;
import net.minecraft.world.level.storage.LevelResource;
import net.minecraft.world.clock.WorldClocks;
import net.minecraft.nbt.*;
import java.util.*;
import java.nio.file.*;

/** Finite village dollar trade and resource deliveries. Items stay in persistent
 * escrow until the signature-bound GTA money operation acknowledges its receipt. */
public final class GroveTownEconomy {
    private GroveTownEconomy(){}
    private static final Gson JSON=new Gson();
    private static final AttachmentType<String> STATE=AttachmentRegistry.<String>builder().persistent(Codec.STRING).copyOnDeath()
        .buildAndRegister(Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"town_economy_v1"));
    private static final AttachmentType<List<ItemStack>> ESCROW=AttachmentRegistry.<List<ItemStack>>builder()
        .persistent(ItemStack.OPTIONAL_CODEC.listOf()).copyOnDeath().buildAndRegister(Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"town_escrow_v1"));
    private static final AttachmentType<List<ItemStack>> MAIL=AttachmentRegistry.<List<ItemStack>>builder()
        .persistent(ItemStack.OPTIONAL_CODEC.listOf()).copyOnDeath().buildAndRegister(Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"town_mail_v1"));
    public static final class Market {String key;double x,y,z;Market(String key,double x,double y,double z){this.key=key;this.x=x;this.y=y;this.z=z;}}
    public static final class Job {String origin,target;int kind;double x,z;}
    public static final class Pending {int id,amount,slot,status=9;String market;boolean job;}
    public static final class State {
        String token;int next=1,earned,reputation;long day=-1,lastJobDay=-1;
        Map<String,Integer> used=new LinkedHashMap<>();Map<String,Market> markets=new LinkedHashMap<>();Job job;Pending pending;
    }
    private record Offer(int slot,Item item,int count,int dollars,int stock,boolean sale){}
    private static final List<Offer> OFFERS=List.of(
        new Offer(2,Items.OAK_LOG,16,80,4,true),new Offer(3,Items.COAL,12,100,3,true),
        new Offer(4,Items.WHEAT,20,60,4,true),new Offer(5,Items.IRON_INGOT,6,150,2,true),
        new Offer(6,Items.EMERALD,1,35,4,true),new Offer(11,Items.BREAD,4,40,4,false),
        new Offer(12,Items.ARROW,16,80,3,false),new Offer(13,Items.IRON_PICKAXE,1,180,2,false),
        new Offer(14,Items.BUCKET,1,110,2,false),new Offer(15,Items.REDSTONE,4,80,3,false),
        new Offer(16,Items.IRON_INGOT,3,90,3,false));
    private static int reportedCash=-1;private static long cashAt,nextSend;
    public static boolean enabled(){return Boolean.parseBoolean(System.getProperty("grovecraft.townEconomy","true"))||Boolean.getBoolean("grovecraft.townFieldAcceptance");}
    public static int observedCash(){return reportedCash;}
    public static boolean hasPending(ServerPlayer p){return state(p).pending!=null;}
    public static int mailCount(ServerPlayer p){var mail=p.getAttached(MAIL);return mail==null?0:mail.stream().mapToInt(ItemStack::getCount).sum();}
    public static int escrowCount(ServerPlayer p){var items=p.getAttached(ESCROW);return items==null?0:items.stream().mapToInt(ItemStack::getCount).sum();}
    public static State state(ServerPlayer p){
        String saved=p.getAttached(STATE);State s=saved==null?new State():JSON.fromJson(saved,State.class);
        if(s==null||saved!=null&&s.token==null)throw new IllegalStateException("Town ledger missing identity");
        if(s.token==null)s.token=UUID.randomUUID().toString().replace("-","").substring(0,12);
        if(!s.token.matches("[a-f0-9]{12}")||s.next<1||s.used==null||s.markets==null)throw new IllegalStateException("Town ledger invalid");
        if(saved==null)store(p,s);return s;
    }
    private static void store(ServerPlayer p,State s){p.setAttached(STATE,JSON.toJson(s));}
    public static long day(ServerPlayer p){return p.level().getServer().clockManager().getInstance(p.level().registryAccess()
        .lookupOrThrow(Registries.WORLD_CLOCK).getOrThrow(WorldClocks.OVERWORLD)).totalTicks()/24000;}
    private static void refresh(State s,long day){if(s.pending==null&&day>s.day){s.day=day;s.earned=0;s.used.clear();}}
    private static Market market(Villager v){int x=v.getBlockX(),z=v.getBlockZ();
        for(var tag:v.entityTags())if(tag.startsWith("grovecraft_job:"))try{var parts=tag.substring(15).split(":");x=Integer.parseInt(parts[0]);z=Integer.parseInt(parts[2]);break;}catch(RuntimeException ignored){}
        String key="m:"+Math.floorDiv(x,64)+":"+Math.floorDiv(z,64);
        return new Market(key,v.getX(),v.getY(),v.getZ());}
    private static boolean eligible(ServerPlayer p,Villager v,boolean menu){var sky=new Link.SkyState();
        return Link.active()&&!Link.hostSuspended()&&p.isAlive()&&!p.isSleeping()&&v.isAlive()&&v.level()==p.level()
            &&v.entityTags().contains("grovecraft_settler")&&p.distanceToSqr(v)<25&&Link.readSkyState(sky)
            &&sky.worldId==0&&GroveTownRules.safe(sky.flags,menu);}
    private static void say(ServerPlayer p,String text){p.sendSystemMessage(Component.literal(text));}
    public static void register(){
        if(!enabled())return;
        ServerLifecycleEvents.SERVER_STARTED.register(server->{reportedCash=-1;cashAt=nextSend=0;});
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(server.getTickCount()%5!=0||!Link.active()||Link.hostSuspended()||server.getPlayerList().getPlayers().isEmpty())return;
            var p=server.getPlayerList().getPlayers().getFirst();var sky=new Link.SkyState();
            if(!Link.readSkyState(sky)||!GroveTownRules.safe(sky.flags,false)||p.isSleeping()||!p.isAlive())return;
            try{
                deliverMail(p);State s=state(p);if(s.pending==null)return;
                if(s.pending.status==1||s.pending.status==0){finish(p,s);return;}
                if(s.pending.status==-2||System.nanoTime()<nextSend)return;
                nextSend=System.nanoTime()+750_000_000L;
                if(!durable(p,s))return;
                long token=Long.parseLong(s.token,16);
                Link.pushEvent(48,s.pending.id,s.pending.amount,(float)(token&0xffffff),(float)(token>>>24),0,0);
            }catch(Exception e){LibertyCraft.LOG.error("[GroveCraft] Town ledger paused; escrow retained",e);}
        });
    }
    /** Verify the pending marker in the atomic vanilla player file before GTA can pay. */
    private static boolean durable(ServerPlayer p,State s){
        p.level().getServer().getPlayerList().saveAll();
        Path file=p.level().getServer().getWorldPath(LevelResource.PLAYER_DATA_DIR).resolve(p.getUUID()+".dat");
        try{
            String nbt=NbtIo.readCompressed(file,NbtAccounter.unlimitedHeap()).toString().replace("\\\"","\"");
            return nbt.contains("\"token\":\""+s.token+"\"")&&nbt.contains("\"pending\":{\"id\":"+s.pending.id+",");
        }catch(Exception e){LibertyCraft.LOG.error("Town escrow save not verified; no payment sent",e);return false;}
    }
    public static void receive(ServerPlayer p,int action,int id,int result,int cash){
        if(action==502){reportedCash=result==1?Math.clamp(id,0,99999999):-1;cashAt=System.nanoTime();return;}
        if(!enabled())return;
        State s=state(p);if(s.pending==null||s.pending.id!=id||(result!=0&&result!=1&&result!=-2))return;
        s.pending.status=result;store(p,s);p.level().getServer().getPlayerList().saveAll();
        if(result==-2)say(p,"Pago pendiente de revisión: tus materiales siguen protegidos en depósito.");
    }
    private static boolean accepts(ItemStack stack,Item item){return item==Items.OAK_LOG?stack.is(ItemTags.LOGS):stack.is(item);}
    public static int count(ServerPlayer p,Item item){int count=0;for(int i=0;i<36;i++){var stack=p.getInventory().getItem(i);if(accepts(stack,item))count+=stack.getCount();}return count;}
    private static void reserve(ServerPlayer p,Item item,int amount,List<ItemStack> escrow){
        for(int i=0;i<36&&amount>0;i++){var stack=p.getInventory().getItem(i);if(!accepts(stack,item))continue;
            int take=Math.min(amount,stack.getCount());escrow.add(p.getInventory().removeItem(i,take));amount-=take;}
        if(amount!=0)throw new IllegalStateException("Escrow count changed");
    }
    private static void give(ServerPlayer p,ItemStack item){var stack=item.copy();p.getInventory().add(stack);
        if(!stack.isEmpty()){var saved=p.getAttached(MAIL);var mail=saved==null?new ArrayList<ItemStack>():new ArrayList<>(saved);
            mail.add(stack.copy());p.setAttached(MAIL,List.copyOf(mail));say(p,"Inventario lleno: entrega guardada hasta que liberes espacio.");}}
    private static void deliverMail(ServerPlayer p){var mail=p.getAttached(MAIL);if(mail==null||mail.isEmpty())return;
        var remaining=new ArrayList<ItemStack>();boolean changed=false;
        for(var original:mail){var stack=original.copy();p.getInventory().add(stack);changed|=stack.getCount()!=original.getCount();if(!stack.isEmpty())remaining.add(stack);}
        if(changed){p.setAttached(MAIL,List.copyOf(remaining));p.getInventory().setChanged();p.level().getServer().getPlayerList().saveAll();}}
    private static void finish(ServerPlayer p,State s){
        var q=s.pending;
        if(q.status==0){var escrow=p.getAttached(ESCROW);if(escrow!=null)for(var item:escrow)give(p,item);say(p,"GTA no pudo pagar: materiales devueltos, compra cancelada.");}
        else{
            if(q.amount>0)s.earned+=q.amount;
            if(q.job){s.job=null;s.lastJobDay=s.day;s.reputation=Math.min(20,s.reputation+1);say(p,"Encargo entregado: +$"+q.amount+". Reputación "+s.reputation+"/20.");}
            else{
                s.used.merge(q.market+":"+q.slot,1,Integer::sum);
                var offer=OFFERS.stream().filter(o->o.slot==q.slot).findFirst().orElseThrow();
                if(!offer.sale)give(p,new ItemStack(offer.item,offer.count));
                say(p,offer.sale?"Venta completada: +$"+q.amount:"Compra completada: $"+(-q.amount)+".");
            }
        }
        p.setAttached(ESCROW,List.of());s.pending=null;store(p,s);p.getInventory().setChanged();p.containerMenu.broadcastChanges();
        p.level().getServer().getPlayerList().saveAll();
    }
    public static boolean open(ServerPlayer p,Villager v){
        if(!enabled()||!eligible(p,v,false))return false;
        State s=state(p);refresh(s,day(p));Market m=market(v);
        if(s.markets.size()<128||s.markets.containsKey(m.key))s.markets.put(m.key,m);store(p,s);
        p.closeContainer();p.openMenu(new SimpleMenuProvider((id,inv,player)->new Board(id,inv,p,v,m),Component.literal("Mercado GTA · encargos")));return true;
    }
    private static ItemStack button(Item item,String name,String...lines){var stack=new ItemStack(item);stack.set(DataComponents.CUSTOM_NAME,Component.literal(name));
        stack.set(DataComponents.LORE,new ItemLore(Arrays.stream(lines).map(Component::literal).map(c->(Component)c).toList()));return stack;}
    public static final class Board extends ChestMenu {
        private final ServerPlayer owner;private final Villager villager;private final Market market;private boolean closed;
        Board(int id,Inventory inv,ServerPlayer owner,Villager v,Market market){super(MenuType.GENERIC_9x3,id,inv,new SimpleContainer(27),3);
            this.owner=owner;this.villager=v;this.market=market;State s=state(owner);
            getSlot(0).set(button(Items.GOLD_INGOT,reportedCash>=0&&System.nanoTime()-cashAt<5_000_000_000L?"Dinero GTA: $"+reportedCash:"Conectando dinero GTA",
                "Ventas hoy: $"+s.earned+" / $1000","Reputación: "+s.reputation+" / 20","3 encargos: descuento del 10%","Guarda también tu partida de GTA después de comerciar"));
            for(var offer:OFFERS){int price=offer.sale?offer.dollars:GroveTownRules.price(offer.dollars,s.reputation);
                int remaining=offer.stock-s.used.getOrDefault(market.key+":"+offer.slot,0);
                getSlot(offer.slot).set(button(offer.item,(offer.sale?"Vender ":"Comprar ")+offer.count+" · $"+price,
                    "Operaciones disponibles hoy: "+Math.max(0,remaining),"Clic izquierdo para confirmar"));}
            getSlot(20).set(button(Items.OAK_LOG,"Encargo local · $140","Entregar 16 troncos y 8 trigo","Una entrega de encargo por día"));
            var destination=destination(s,market);
            getSlot(23).set(button(Items.MINECART,"Entrega entre mercados · $200",destination==null?"Explora un segundo mercado a 300m o más":
                "16 troncos hasta X:"+(int)destination.x+" Z:"+(int)destination.z,"Puedes usar el equipaje del vehículo (G)"));
            if(s.job!=null){getSlot(21).set(button(Items.PAPER,"Entregar encargo activo",s.job.kind==0?"16 troncos y 8 trigo en mercado de origen":
                "16 troncos en X:"+(int)s.job.x+" Z:"+(int)s.job.z,"Troncos: "+count(owner,Items.OAK_LOG)+" · Trigo: "+count(owner,Items.WHEAT)));
                getSlot(22).set(button(Items.BARRIER,"Cancelar encargo","Sin cargo; no consume materiales"));}
            if(s.pending!=null)getSlot(18).set(button(Items.CLOCK,"Pago pendiente","No se permite otro hasta recibir confirmación"));
        }
        @Override public boolean stillValid(Player p){return p==owner&&!closed&&eligible(owner,villager,true);}
        @Override public ItemStack quickMoveStack(Player p,int index){return ItemStack.EMPTY;}
        @Override public void clicked(int index,int mouse,ContainerInput type,Player player){
            if(!stillValid(player)||mouse!=0||type!=ContainerInput.PICKUP)return;
            if(OFFERS.stream().noneMatch(o->o.slot==index)&&index!=20&&index!=21&&index!=22&&index!=23)return;
            closed=true;owner.closeContainer();choose(owner,villager,market,index);
        }
    }
    private static Market destination(State s,Market origin){return s.markets.values().stream()
        .filter(m->!m.key.equals(origin.key)&&GroveTownRules.route(m.x,m.z,origin.x,origin.z))
        .min(Comparator.comparingDouble(m->Math.hypot(m.x-origin.x,m.z-origin.z))).orElse(null);}
    public static void choose(ServerPlayer p,Villager v,Market m,int slot){
        if(!eligible(p,v,true))return;State s=state(p);refresh(s,day(p));
        if(s.pending!=null){say(p,"Hay un pago pendiente; materiales protegidos en depósito.");return;}
        if(slot==22){s.job=null;store(p,s);say(p,"Encargo cancelado.");return;}
        if(slot==20||slot==23){
            if(s.job!=null){say(p,"Termina o cancela tu encargo activo.");return;}
            if(s.lastJobDay>=s.day){say(p,"Ya completaste un encargo hoy. Vuelve mañana.");return;}
            Market target=slot==20?m:destination(s,m);if(target==null){say(p,"Descubre otro mercado a 300m–2500m para ofrecer una ruta.");return;}
            s.job=new Job();s.job.origin=m.key;s.job.target=target.key;s.job.x=target.x;s.job.z=target.z;s.job.kind=slot==20?0:1;
            store(p,s);p.level().getServer().getPlayerList().saveAll();say(p,slot==20?"Encargo aceptado: 16 troncos y 8 trigo aquí, recompensa $140.":
                "Ruta aceptada: 16 troncos hasta X:"+(int)target.x+" Z:"+(int)target.z+", recompensa $200.");return;
        }
        int amount;List<ItemStack> escrow=new ArrayList<>();boolean job=slot==21;
        if(job){
            if(s.job==null||!s.job.target.equals(m.key)){say(p,"Entrega en el mercado indicado por tu encargo.");return;}
            amount=s.job.kind==0?140:200;
            if(count(p,Items.OAK_LOG)<16||(s.job.kind==0&&count(p,Items.WHEAT)<8)){say(p,"Faltan materiales del encargo.");return;}
        }else{
            var offer=OFFERS.stream().filter(o->o.slot==slot).findFirst().orElse(null);if(offer==null)return;
            if(s.used.getOrDefault(m.key+":"+slot,0)>=offer.stock){say(p,"Este producto se agotó hoy. Vuelve mañana.");return;}
            if(offer.sale&&count(p,offer.item)<offer.count){say(p,"No tienes suficientes materiales.");return;}
            amount=offer.sale?offer.dollars:-GroveTownRules.price(offer.dollars,s.reputation);
        }
        if(amount>0&&!GroveTownRules.canEarn(s.earned,amount)){say(p,"Límite de ventas/encargos de $1000 por día alcanzado.");return;}
        if(s.next==Integer.MAX_VALUE){say(p,"Libro de pagos completo; no se ha consumido nada.");return;}
        if(job){reserve(p,Items.OAK_LOG,16,escrow);if(s.job.kind==0)reserve(p,Items.WHEAT,8,escrow);}
        else{var offer=OFFERS.stream().filter(o->o.slot==slot).findFirst().orElseThrow();if(offer.sale)reserve(p,offer.item,offer.count,escrow);}
        s.pending=new Pending();s.pending.id=s.next++;s.pending.amount=amount;s.pending.slot=slot;s.pending.market=m.key;s.pending.job=job;
        p.setAttached(ESCROW,List.copyOf(escrow));store(p,s);p.getInventory().setChanged();
        durable(p,s);say(p,"Confirmando operación con GTA…");
    }
}
