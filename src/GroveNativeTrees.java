package dev.libertycraft;

import com.mojang.serialization.Codec;
import dev.libertycraft.link.Link;
import dev.libertycraft.world.HostDig;
import java.util.*;
import net.fabricmc.fabric.api.attachment.v1.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.minecraft.core.BlockPos;
import net.minecraft.resources.Identifier;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.entity.item.ItemEntity;
import net.minecraft.world.item.*;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.LevelEvent;
import dev.libertycraft.world.HostCollision;
import dev.libertycraft.world.HostTri;
import net.minecraft.world.phys.AABB;
import net.minecraft.server.level.ServerLevel;

/** Native wood cuts have a distinct mask, never terrain reveals. Harvested instance keys
 * live in their anchor chunk; acknowledgements hide only that stock tree, not its neighbours. */
public final class GroveNativeTrees {
    private static final AttachmentType<List<String>> FELLED=AttachmentRegistry.<List<String>>builder()
        .persistent(Codec.STRING.listOf()).buildAndRegister(Identifier.fromNamespaceAndPath(LibertyCraft.MOD_ID,"native_trees_v1"));
    public static boolean wood(int material){return material==10||material==11;}
    public static boolean cut(Level level,int world,BlockPos pos){return HostDig.isDug(level,world+256,pos);}
    public static void open(ServerPlayer player,int world,BlockPos pos,int material){
        var level=player.level();if(world<0||world>255||!wood(material)||!HostDig.markDug(level,world+256,pos))return;
        var state=HostDig.materialState(material);
        if(!player.isCreative()){
            var tool=player.getMainHandItem();var used=tool.copy();boolean harvest=player.hasCorrectToolForDrops(state);
            tool.mineBlock(level,state,pos,player);
            if(harvest)state.getBlock().playerDestroy(level,player,pos,state,null,used);
        }
        level.levelEvent(LevelEvent.PARTICLES_AND_SOUND_DESTROY_BLOCK,pos,Block.getId(state));
        LibertyCraft.LOG.info("[GroveCraft] native wood cut {} material {} (no terrain reveal)",pos,material);
    }
    private static int material(int model){
        return switch(model){
            case 615,616,617,618,669,671,672,673,691,700,703,705,708,713,714,715,717,726,727,728,729,730,731,732,733,734,735,736,737,738,767,768,771,776,782,789,792->10;
            case 654,655,656,657,658,659,660,661,664,670,698,719,720,721,722,723,724,725,786,787,788->11;
            default->0;
        };
    }
    public static void fell(ServerPlayer player,int model,int qx,int packed,int qz){
        int count=GroveNativeTreeRules.count(packed),qy=(short)packed;
        double x=qx/16.,y=qy/16.,z=qz/16.,heading=GroveNativeTreeRules.heading(packed),length=GroveNativeTreeRules.length(packed);
        var sky=new Link.SkyState();
        if(material(model)==0||count<2||count>16||Math.abs(x)>4000||Math.abs(z)>4000||y< -60||y>1000
            ||!Link.readSkyState(sky)||!GroveEcologyRules.safe(sky.flags,sky.worldId,sky.inGame(),sky.loading())
            ||player.distanceToSqr(x,y,z)>32*32||!HostDig.destruction)return;
        var level=player.level();var anchor=BlockPos.containing(x,y,z);if(!level.hasChunkAt(anchor))return;
        var chunk=level.getChunkAt(anchor);var ledger=chunk.getAttached(FELLED);
        String key=sky.worldId+":"+model+":"+qx+":"+qy+":"+qz;
        if(ledger==null)ledger=List.of();
        String prior=ledger.stream().filter(a->a.equals(key)||a.startsWith(key+":")).findFirst().orElse(null);
        boolean grown=prior!=null&&prior.endsWith(":1")&&prior.split(":").length==7;
        if(prior==null||grown){
            if(ledger.size()>=256)return;
            var next=new ArrayList<>(ledger);if(prior!=null)next.remove(prior);
            next.add(key+":"+level.getGameTime()+":0");chunk.setAttached(FELLED,List.copyOf(next));
            // The base already dropped one log. These are the remaining harvest, ordinary
            // Minecraft ItemEntities with normal gravity/pickup, never an inventory grant.
            if(!player.isCreative())for(int i=0;i<count;i++){
                double along=GroveNativeTreeRules.along(i,count,length),side=(i%3-1)*.18;
                double px=x+Math.cos(heading)*along-Math.sin(heading)*side,pz=z+Math.sin(heading)*along+Math.cos(heading)*side;
                var tris=new ArrayList<HostTri>();HostCollision.trianglesNear(new AABB(px-.1,y-32,pz-.1,px+.1,y+16,pz+.1),tris);
                double ground=Double.NaN;for(var t:tris)if(t.terrain&&Math.abs(t.ny)>.5){
                    double height=t.heightAt(px,pz);if(Double.isFinite(height)&&(!Double.isFinite(ground)||height>ground))ground=height;
                }
                var drop=new ItemEntity(level,px,(Double.isFinite(ground)?ground:y)+1.1,pz,
                    new ItemStack(material(model)==11?Items.SPRUCE_LOG:Items.OAK_LOG));
                drop.addTag("grovecraft_native_tree_drop");
                GroveGameplay.preserveNativeDrop(drop,(Double.isFinite(ground)?ground:y)+.1);
                drop.setDefaultPickUpDelay();level.addFreshEntity(drop);
                LibertyCraft.LOG.info("[GroveCraft] trunk drop {} at {} {} {}",i,px,drop.getY(),pz);
            }
            LibertyCraft.LOG.info("[GroveCraft] native tree harvested {}: {} log drops along {} m heading {}",key,count,length,heading);
        }
        Link.pushEvent(43,model,sky.worldId,(float)x,(float)y,(float)z,0);
    }
    /** Simulated elapsed time in our opt-in isolated acceptance world only. */
    public static int ageAcceptance(ServerPlayer player){
        if(!Boolean.getBoolean("grovecraft.fellingAcceptance")||!"GroveCraftGtaTestFelling".equals(System.getProperty("grovecraft.testWorld")))return 0;
        int changed=0,cx=player.getBlockX()>>4,cz=player.getBlockZ()>>4;
        for(int dx=-2;dx<=2;dx++)for(int dz=-2;dz<=2;dz++){
            if(!player.level().hasChunk(cx+dx,cz+dz))continue;var chunk=player.level().getChunk(cx+dx,cz+dz);
            var ledger=chunk.getAttached(FELLED);if(ledger==null)continue;var next=new ArrayList<String>();
            for(String entry:ledger){String[] a=entry.split(":");
                if(a.length==7&&a[6].equals("0")){next.add(String.join(":",Arrays.copyOf(a,5))+":"+(player.level().getGameTime()-GroveNativeTreeRules.REGROW_TICKS)+":0");changed++;}
                else next.add(entry);
            }chunk.setAttached(FELLED,List.copyOf(next));
        }
        LibertyCraft.LOG.info("[GroveCraft] isolated fixture aged {} tree records seven days",changed);return changed;
    }
    private static boolean healWood(ServerLevel level,int world,double x,double y,double z){
        int x0=(int)Math.floor(x)-2,x1=(int)Math.floor(x)+2,z0=(int)Math.floor(z)-2,z1=(int)Math.floor(z)+2;
        for(int cx=x0>>4;cx<=x1>>4;cx++)for(int cz=z0>>4;cz<=z1>>4;cz++)
            if(!level.hasChunk(cx,cz))return false;
        for(int cx=x0>>4;cx<=x1>>4;cx++)for(int cz=z0>>4;cz<=z1>>4;cz++){
            var chunk=level.getChunk(cx,cz);var column=chunk.getAttached(HostDig.DUG);if(column==null)continue;
            boolean changed=false;var sections=new ArrayList<HostDig.DugSection>();
            for(var section:column.sections()){
                if(section.world()!=world+256){sections.add(section);continue;}
                long[] bits=section.bits().clone();boolean local=false;
                for(int i=0;i<4096;i++)if((bits[i>>6]&(1L<<(i&63)))!=0){
                    int px=cx*16+(i&15),py=section.sectionY()*16+(i>>8),pz=cz*16+((i>>4)&15);
                    if(px>=x0&&px<=x1&&pz>=z0&&pz<=z1&&py>=Math.floor(y)&&py<=y+45){bits[i>>6]&=~(1L<<(i&63));local=true;}
                }
                changed|=local;sections.add(local?new HostDig.DugSection(section.world(),section.sectionY(),bits):section);
            }
            if(changed)chunk.setAttached(HostDig.DUG,new HostDig.DugColumn(List.copyOf(sections)));
        }return true;
    }
    public static void register(){
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(!Link.active()||server.getTickCount()%10!=0)return;
            var sky=new Link.SkyState();if(!Link.readSkyState(sky)||!sky.inGame()||sky.loading())return;
            for(var p:server.getPlayerList().getPlayers()){
                int cx=p.getBlockX()>>4,cz=p.getBlockZ()>>4,sent=0;
                for(int dx=-8;dx<=8;dx++)for(int dz=-8;dz<=8;dz++){
                    var anchor=new BlockPos((cx+dx)*16,p.getBlockY(),(cz+dz)*16);
                    if(!p.level().hasChunkAt(anchor))continue;var ledger=p.level().getChunkAt(anchor).getAttached(FELLED);
                    if(ledger==null)continue;var next=new ArrayList<String>();boolean changed=false;
                    for(String entry:ledger){
                        String[] a=entry.split(":");if(a.length<5||a.length>7){next.add(entry);continue;}
                        try{
                            int w=Integer.parseInt(a[0]),model=Integer.parseInt(a[1]);
                            float x=Integer.parseInt(a[2])/16f,y=Integer.parseInt(a[3])/16f,z=Integer.parseInt(a[4])/16f;
                            String key=String.join(":",Arrays.copyOf(a,5));long harvested=a.length>=6?Long.parseLong(a[5]):p.level().getGameTime();
                            boolean grown=a.length==7&&a[6].equals("1");double distance=p.distanceToSqr(x,y,z);
                            if(!grown&&w==sky.worldId&&GroveNativeTreeRules.regrow(p.level().getGameTime(),harvested,distance)
                                &&healWood(p.level(),w,x,y,z)){
                                grown=true;LibertyCraft.LOG.info("[GroveCraft] native tree regrown {} after seven days, CJ distant",key);
                            }
                            String update=key+":"+harvested+":"+(grown?1:0);next.add(update);changed|=!update.equals(entry);
                            if(w==sky.worldId&&distance<192*192&&sent++<64)Link.pushEvent(grown?44:43,model,w,x,y,z,0);
                        }catch(NumberFormatException ignored){next.add(entry);}
                    }
                    if(changed)p.level().getChunkAt(anchor).setAttached(FELLED,List.copyOf(next));
                }
            }
        });
    }
}
