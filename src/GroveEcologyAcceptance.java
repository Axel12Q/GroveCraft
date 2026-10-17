package dev.libertycraft;

import java.nio.file.*;
import java.util.*;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.core.BlockPos;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.entity.BarrelBlockEntity;
import dev.libertycraft.world.HostDig;

/** Opt-in real Minecraft test with an explicitly synthetic, flat native-ground provider.
 * Refuses production worlds; two launches prove the ledger survives real world save/reload. */
public final class GroveEcologyAcceptance {
    private static boolean finished;
    private static void require(boolean ok,String message) { if(!ok)throw new IllegalStateException(message); }
    public static void register() {
        if(!Boolean.getBoolean("grovecraft.ecologyAcceptance"))return;
        require(LibertyCraft.WORLD_NAME.equals("GroveCraftGtaTestEcologyAcceptance"),"Isolated ecology world required");
        ServerTickEvents.END_SERVER_TICK.register(server -> {
            if(finished||server.getTickCount()<100||server.getPlayerList().getPlayers().isEmpty())return;
            ServerLevel level=server.getPlayerList().getPlayers().getFirst().level();
            var flat=new GroveEcology.Terrain() {
                public double grass(int x,int z,double y){return 20.;}
                public boolean known(int x,int y,int z){return true;}
                public boolean occupied(BlockPos p){return false;}
            };
            Path marker=server.getWorldPath(net.minecraft.world.level.storage.LevelResource.ROOT).resolve("ecology-acceptance-marker");
            Path report=FabricLoader.getInstance().getGameDir().resolve("grovecraft-ecology-acceptance.json");
            try {
                var oak=new GroveEcologyRules.Site("acceptance:OAK",2495,1720,GroveEcologyRules.Kind.OAK,1);
                BlockPos log=new BlockPos(2495,20,1720);
                boolean reload=Files.exists(marker);
                if(reload) {
                    require(level.getBlockState(log).isAir(),"Harvest stays absent after real reload");
                    require(GroveEcology.generate(level,oak,20,flat).equals("visited"),"Persistent ledger forbids regeneration");
                    var barrel=(BarrelBlockEntity)level.getBlockEntity(new BlockPos(2561,21,1754));
                    require(barrel!=null&&barrel.isEmpty(),"Loot remains empty after reload");
                    var adaptive=new GroveEcologyRules.Site("acceptance:adaptive",2410,1685,GroveEcologyRules.Kind.HAMLET,31);
                    require(GroveEcology.generate(level,adaptive,20,flat).equals("visited"),"Adaptive village ledger persisted");
                    require(level.getBlockState(new BlockPos(2406,20,1682)).is(Blocks.BARREL),"Lower house survived reload");
                    require(level.getBlockState(new BlockPos(2417,22,1682)).is(Blocks.BARREL),"Upper house survived reload");
                    var owner=new GroveEcologyRules.Site("acceptance:relocated",2400,1760,GroveEcologyRules.Kind.HAMLET,32);
                    require(GroveEcology.generate(level,owner,20,flat).equals("visited"),"Relocation ownership persisted at original anchor");
                } else {
                    var player=server.getPlayerList().getPlayers().getFirst();
                    require(player.getInventory().getItem(0).is(net.minecraft.world.item.Items.WRITTEN_BOOK),"Expedition field guide");
                    require(player.getInventory().getItem(0).get(net.minecraft.core.component.DataComponents.WRITTEN_BOOK_CONTENT).pages().size()==7,"Readable guide pages");
                    require(player.getInventory().getItem(1).is(net.minecraft.world.item.Items.BREAD)&&player.getInventory().getItem(1).getCount()==3,"Small survival start");
                    for(int slot=2;slot<36;slot++)require(player.getInventory().getItem(slot).isEmpty(),"No creative kit");
                    int i=0;
                    for(var kind:GroveEcologyRules.Kind.values()) {
                        var site=new GroveEcologyRules.Site("acceptance:"+kind,2495+(i%3)*32,1720+(i/3)*32,kind,i+1);
                        require(GroveEcology.generate(level,site,20,flat).equals("generated"),"Generate "+kind);
                        require(GroveEcology.generate(level,site,20,flat).equals("visited"),"No duplicates "+kind);
                        i++;
                    }
                    level.destroyBlock(log,false);
                    // RUIN is index five: barrel offset +2,+1,+2.
                    var barrel=(BarrelBlockEntity)level.getBlockEntity(new BlockPos(2561,21,1754));
                    require(barrel!=null&&!barrel.isEmpty(),"Finite real barrel contents");barrel.clearContent();barrel.setChanged();
                    require(level.getBlockState(new BlockPos(2495,21,1752)).is(Blocks.WATER),"Sealed irrigation source");
                    require(level.getBlockState(new BlockPos(2496,22,1752)).is(Blocks.WHEAT),"Living crop");
                    require(level.getBlockState(new BlockPos(2521,21,1749)).getValue(net.minecraft.world.level.block.AbstractBedBlock.PART)==net.minecraft.world.level.block.state.properties.BedPart.FOOT,"Real intact bed");
                    var reject=new GroveEcologyRules.Site("acceptance:reject",2470,1770,GroveEcologyRules.Kind.OAK,17);
                    var unknown=new GroveEcology.Terrain() {
                        public double grass(int x,int z,double y){return 20;}
                        public boolean known(int x,int y,int z){return false;}
                        public boolean occupied(BlockPos p){return false;}
                    };
                    require(GroveEcology.generate(level,reject,20,unknown).equals("geometría pendiente"),"Unknown ground fails closed");
                    var blocked=new GroveEcology.Terrain() {
                        public double grass(int x,int z,double y){return 20;}
                        public boolean known(int x,int y,int z){return true;}
                        public boolean occupied(BlockPos p){return p.getY()==22;}
                    };
                    require(GroveEcology.generate(level,reject,20,blocked).equals("obstáculo nativo"),"Native obstacle protected");
                    BlockPos protectedBlock=new BlockPos(2470,20,1770);
                    level.setBlock(protectedBlock,Blocks.DIAMOND_BLOCK.defaultBlockState(),3);
                    require(GroveEcology.generate(level,reject,20,flat).equals("zona modificada"),"Player construction protected");
                    require(level.getBlockState(protectedBlock).is(Blocks.DIAMOND_BLOCK),"Protected block unchanged");
                    level.setBlock(protectedBlock,Blocks.AIR.defaultBlockState(),3);HostDig.markDug(level,0,protectedBlock);
                    require(GroveEcology.generate(level,reject,20,flat).equals("zona modificada"),"Excavation protected");
                    var hillside=new GroveEcologyRules.Site("acceptance:slope",2440,1720,GroveEcologyRules.Kind.BIRCH,12);
                    var slope=new GroveEcology.Terrain() {
                        public double grass(int x,int z,double y){return 20+(x-2440)*.7;}
                        public boolean known(int x,int y,int z){return true;}
                        public boolean occupied(BlockPos p){return p.getY()<=Math.floor(grass(p.getX(),p.getZ(),20));}
                    };
                    require(GroveEcology.generate(level,hillside,20,slope).equals("generated"),"Tree rooted on mountain slope");
                    require(level.getBlockState(new BlockPos(2440,20,1720)).is(Blocks.BIRCH_LOG),"Trunk anchored to local surface");
                    var tilted=new GroveEcologyRules.Site("acceptance:foundation",2440,1770,GroveEcologyRules.Kind.CAMP,13);
                    var campSlope=new GroveEcology.Terrain() {
                        public double grass(int x,int z,double y){return 20.4+(x-2440)*.1;}
                        public boolean known(int x,int y,int z){return true;}
                        public boolean occupied(BlockPos p){return p.getY()<=Math.floor(grass(p.getX(),p.getZ(),20));}
                    };
                    require(GroveEcology.generate(level,tilted,20,campSlope).equals("generated"),"Cabin gentle slope");
                    require(level.getBlockState(new BlockPos(2437,21,1770)).is(Blocks.OAK_PLANKS),"Level cabin floor");
                    require(level.getBlockState(new BlockPos(2437,20,1770)).is(Blocks.COBBLESTONE),"Slope foundation closes floor gap");
                    var steep=new GroveEcology.Terrain() {
                        public double grass(int x,int z,double y){return 20+(x-2470)*.5;}
                        public boolean known(int x,int y,int z){return true;}
                        public boolean occupied(BlockPos p){return false;}
                    };
                    require(GroveEcology.generate(level,new GroveEcologyRules.Site("steep",2470,1810,GroveEcologyRules.Kind.CAMP,20),20,steep).equals("pendiente"),"Steep structure rejected");
                    var adaptive=new GroveEcologyRules.Site("acceptance:adaptive",2410,1685,GroveEcologyRules.Kind.HAMLET,31);
                    var villageSlope=new GroveEcology.Terrain(){
                        public double grass(int x,int z,double y){return 20+(x-2410)*.18;}
                        public boolean known(int x,int y,int z){return true;}
                        public boolean occupied(BlockPos p){return p.getY()<=Math.floor(grass(p.getX(),p.getZ(),20));}
                    };
                    require(GroveEcology.generate(level,adaptive,20,villageSlope).equals("generated"),"Village follows local platforms on a slope above old1.5m limit");
                    require(level.getBlockState(new BlockPos(2406,20,1682)).is(Blocks.BARREL),"Lower house barrel");
                    require(level.getBlockState(new BlockPos(2417,22,1682)).is(Blocks.BARREL),"Upper house barrel");
                    var owner=new GroveEcologyRules.Site("acceptance:relocated",2400,1760,GroveEcologyRules.Kind.HAMLET,32);
                    var moved=GroveHamletPlan.nearby(owner,1);
                    require(GroveEcology.generateAt(level,owner,moved,20,flat).equals("generated"),"Village relocated");
                    require(GroveEcology.generate(level,owner,20,flat).equals("visited"),"Relocated village not duplicated at rejected anchor");
                    Files.writeString(marker,"six vanilla features, harvested oak, looted barrel\n");
                }
                Files.writeString(report,"{\"passed\":true,\"real_minecraft\":true,\"synthetic_ground\":true,\"reload\":"+reload+"}");finished=true;
            }catch(Exception error) {
                finished=true;LibertyCraft.LOG.error("[GroveCraft] Ecology acceptance failed",error);
                try{Files.writeString(report,"{\"passed\":false,\"reason\":\""+error.toString().replace('"','\'')+"\"}");}catch(Exception ignored){}
            }
        });
    }
}
