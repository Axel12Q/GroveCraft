package dev.libertycraft;

import java.nio.file.*;
import dev.libertycraft.link.Link;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.core.BlockPos;
import net.minecraft.world.entity.item.FallingBlockEntity;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.phys.AABB;

/** Opt-in isolated world; real vanilla falling entities over explicitly synthetic ground. */
public final class GroveBugAcceptance {
    private static boolean finished;
    private static boolean shieldFinished;
    private static int fallTick, expectedLogs, expectedDisplays;
    private static AABB fallBox;
    private static void require(boolean ok,String why){if(!ok)throw new IllegalStateException(why);}
    public static void register(){
        if(!Boolean.getBoolean("grovecraft.bugAcceptance"))return;
        require(LibertyCraft.WORLD_NAME.startsWith("GroveCraftGtaTestBugsAcceptance"),"Isolated bug acceptance world required");
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(fallBox==null||server.getPlayerList().getPlayers().isEmpty())return;
            var level=server.getPlayerList().getPlayers().getFirst().level();
            try {
                int age=server.getTickCount()-fallTick;
                var items=level.getEntitiesOfClass(net.minecraft.world.entity.item.ItemEntity.class,fallBox);
                int logs=items.stream().filter(e->e.getItem().is(net.minecraft.tags.ItemTags.LOGS)).mapToInt(e->e.getItem().getCount()).sum();
                if(age<44)require(logs==0,"No wood drops before tree lands");
                else if(age>44) {
                    require(level.getEntitiesOfClass(net.minecraft.world.entity.Display.BlockDisplay.class,fallBox).isEmpty(),"Fall visuals disappear on landing");
                    require(logs==expectedLogs,"Each log drops exactly once after landing: "+logs+" expected "+expectedLogs);
                    Files.writeString(FabricLoader.getInstance().getGameDir().resolve("grovecraft-tree-order-acceptance.json"),
                        "{\"passed\":true,\"real_minecraft\":true,\"synthetic_ground\":true,\"early_drops\":0,\"logs_after_landing\":"+logs+",\"falling_displays\":"+expectedDisplays+"}");
                    fallBox=null;
                }
            }catch(Exception e){
                LibertyCraft.LOG.error("[GroveCraft] Tree ordering acceptance failed",e);fallBox=null;
                try{Files.writeString(FabricLoader.getInstance().getGameDir().resolve("grovecraft-tree-order-acceptance.json"),"{\"passed\":false,\"reason\":\""+e.toString().replace('"','\'')+"\"}");}catch(Exception ignored){}
            }
        });
        ServerTickEvents.END_SERVER_TICK.register(server->{
            if(shieldFinished||server.getPlayerList().getPlayers().isEmpty())return;
            var player=server.getPlayerList().getPlayers().getFirst();var item=player.getMainHandItem();
            var sky=new Link.SkyState();
            if(!item.is(net.minecraft.world.item.Items.SHIELD)||!player.isUsingItem()||player.getTicksUsingItem()<10
                ||!Link.readSkyState(sky)||!GroveEcologyRules.safe(sky.flags,sky.worldId,sky.inGame(),sky.loading()))return;
            shieldFinished=true;
            Path report=FabricLoader.getInstance().getGameDir().resolve("grovecraft-shield-acceptance.json");
            try{
                var type=net.minecraft.core.registries.BuiltInRegistries.ENTITY_TYPE.getValue(net.minecraft.resources.Identifier.withDefaultNamespace("zombie"));
                var attacker=(net.minecraft.world.entity.Mob)type.create(player.level(),net.minecraft.world.entity.EntitySpawnReason.COMMAND);
                require(attacker!=null,"Synthetic shield attacker");
                var facing=player.getLookAngle();
                attacker.setPos(player.getX()+facing.x*1.1,player.getY(),player.getZ()+facing.z*1.1);
                int before=item.getDamageValue();
                player.hurtServer(player.level(),player.damageSources().mobAttack(attacker),3);
                require(item.getDamageValue()>before,"Vanilla shield blocking absorbs attack and spends durability");
                Files.writeString(report,"{\"passed\":true,\"real_gta_mouse_use\":true,\"synthetic_attacker\":true,\"durability_spent\":"+(item.getDamageValue()-before)+"}");
            }catch(Exception e){
                LibertyCraft.LOG.error("[GroveCraft] Shield acceptance failed",e);
                try{Files.writeString(report,"{\"passed\":false,\"reason\":\""+e.toString().replace('"','\'')+"\"}");}catch(Exception ignored){}
            }
        });
        ServerTickEvents.END_SERVER_TICK.register(server->{
            var sky=new Link.SkyState();
            if(finished||server.getTickCount()<100||server.getPlayerList().getPlayers().isEmpty()
                ||!Link.readSkyState(sky)||!GroveEcologyRules.safe(sky.flags,sky.worldId,sky.inGame(),sky.loading()))return;
            finished=true;var player=server.getPlayerList().getPlayers().getFirst();var level=player.level();
            Path report=FabricLoader.getInstance().getGameDir().resolve("grovecraft-bug-acceptance.json");
            try{
                var site=GroveEcologyRules.woodlandSite(level.getSeed(),Math.floorDiv(player.blockPosition().getX(),16)+2,Math.floorDiv(player.blockPosition().getZ(),16)+2);
                int base=player.blockPosition().getY()+8;
                BlockPos root=new BlockPos(site.x(),base,site.z());
                var cells=GroveEcologyRules.plan(site);
                for(var c:cells)level.getChunkAt(root.offset(c.x(),c.y(),c.z()));
                var flat=new GroveEcology.Terrain(){
                    public double grass(int x,int z,double y){return base;}
                    public boolean known(int x,int y,int z){return true;}
                    public boolean occupied(BlockPos p){return false;}
                };
                require(GroveEcology.generate(level,site,base,flat).equals("generated"),"Natural tree generated");
                var state=level.getBlockState(root);
                level.setBlock(root,Blocks.AIR.defaultBlockState(),18);
                int fallen=GroveEcology.toppleNaturalTree(player,root,state);
                require(fallen==cells.size(),"Entire remaining tree falls together");
                var box=new AABB(root).inflate(8);
                int visible=level.getEntitiesOfClass(net.minecraft.world.entity.Display.BlockDisplay.class,box).size();
                require(visible==fallen,"Coherent falling tree displays: "+visible+" of "+fallen);
                fallTick=server.getTickCount();fallBox=new AABB(root).inflate(24);expectedDisplays=fallen;
                expectedLogs=(int)cells.stream().filter(c->c.block().endsWith("_log")).count();
                for(var c:cells)require(level.getBlockState(root.offset(c.x(),c.y(),c.z())).isAir(),"No suspended canopy");
                require(GroveEcology.generate(level,site,base,flat).equals("visited"),"Harvested tree never regenerates");
                BlockPos building=root.offset(10,0,0);level.setBlock(building,Blocks.OAK_LOG.defaultBlockState(),18);
                level.setBlock(building.above(4),Blocks.OAK_LOG.defaultBlockState(),18);
                require(GroveEcology.toppleNaturalTree(player,building,Blocks.OAK_LOG.defaultBlockState())==0,"Player log structure preserved");
                Files.writeString(report,"{\"passed\":true,\"real_minecraft\":true,\"synthetic_ground\":true,\"falling_blocks\":"+fallen+",\"player_structures_preserved\":true}");
            }catch(Exception e){
                LibertyCraft.LOG.error("[GroveCraft] Bug acceptance failed",e);
                try{Files.writeString(report,"{\"passed\":false,\"reason\":\""+e.toString().replace('"','\'')+"\"}");}catch(Exception ignored){}
            }
        });
    }
}
