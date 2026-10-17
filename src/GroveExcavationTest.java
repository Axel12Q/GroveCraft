package dev.libertycraft.world;

import static dev.libertycraft.link.Proto.*;
import static org.junit.jupiter.api.Assertions.assertEquals;
import org.junit.jupiter.api.Test;

class GroveExcavationTest {
    @Test void virtualHoleWallUsesTerrainMaskRatherThanArchitecture()throws Exception{
        var field=HostClip.class.getDeclaredField("STONE_WALL");field.setAccessible(true);
        var triangle=(HostTri)field.get(null);
        org.junit.jupiter.api.Assertions.assertTrue(triangle.diggable);
        org.junit.jupiter.api.Assertions.assertTrue(triangle.terrain,"Virtual soil walls must open the terrain mask, never the building mask");
    }
    @Test void clippedFacadeCoordinatesStayExactlyInsideTheirCell(){
        // Host rejects negative bounds: a real adjacent wall produced -1.11e-16
        // after clipping, keeping its visual cut and collider from being enabled.
        var random=new java.util.Random(91);
        for(int sample=0;sample<10000;sample++){
            float[] coordinates=new float[9];
            for(int j=0;j<9;j++)coordinates[j]=random.nextFloat()*20-10;
            var tri=new HostTri(coordinates,0,TRI_DIGGABLE|(DIG_STONE<<TRI_MATERIAL_SHIFT));
            for(var point:dev.libertycraft.GroveArchitectureCap.surface(tri,net.minecraft.core.BlockPos.ZERO))
                for(double component:point)org.junit.jupiter.api.Assertions.assertTrue(component>=0&&component<=1,
                    "Cell-local clipping must produce valid wire bounds, got "+component+" in sample "+sample);
        }
    }
    @Test void collisionAndTerrainMasksEncodeBothNewCutsAndExplicitClears(){
        long[] bits=new long[64];bits[0]=3;
        var packet=dev.libertycraft.link.GroveRenderBatch.maskPacket(REN_DUG,1,2,3,0,bits);
        assertEquals(2,packet.header().getInt(12));assertEquals(512,packet.body().remaining());
        var clear=dev.libertycraft.link.GroveRenderBatch.maskPacket(REN_SOLIDS,1,2,3,0,null);
        assertEquals(0,clear.header().getInt(12));assertEquals(0,clear.body().remaining());
    }
    @Test void distantRoofOrTreeCanopyNeverTurnsTheAirBelowIntoStone() throws Exception {
        for(int material:new int[]{DIG_STONE,DIG_OAK_LOG}){
            var probe=new HostDig.Probe();
            var roof=new HostTri(new float[]{-4,8,-4,-4,8,4,4,8,-4},0,
                TRI_DIGGABLE|(material<<TRI_MATERIAL_SHIFT));
            // Supply the original vertical column without populating the global
            // native transport. The query is beyond the near-surface radius.
            var column=HostDig.Probe.class.getDeclaredField("column");column.setAccessible(true);
            column.set(probe,java.util.List.of(roof));
            assertEquals(HostDig.KEEP,probe.test(0,0,0));
        }
    }
    @Test void facadeTransactionReservesWholeMeshAndMasksIncludingWrap(){
        var packets=java.util.List.of(
            new dev.libertycraft.link.GroveRenderBatch.Packet(2,java.nio.ByteBuffer.allocate(16),java.nio.ByteBuffer.allocate(24)),
            new dev.libertycraft.link.GroveRenderBatch.Packet(8,java.nio.ByteBuffer.allocate(24),java.nio.ByteBuffer.allocate(0)));
        assertEquals(80,dev.libertycraft.link.GroveRenderBatch.required(0,256,packets));
        assertEquals(112,dev.libertycraft.link.GroveRenderBatch.required(224,256,packets));
        assertEquals(112,dev.libertycraft.link.GroveRenderBatch.required(480,256,packets));
    }
    @Test void oversizedFacadeTransactionCannotPublishPartialPackets(){
        var packets=java.util.List.of(new dev.libertycraft.link.GroveRenderBatch.Packet(2,
            java.nio.ByteBuffer.allocate(16),java.nio.ByteBuffer.allocate(128)));
        assertEquals(-1,dev.libertycraft.link.GroveRenderBatch.required(0,256,packets));
    }
    @Test void thinFacadeCapPreservesItsPartialFootprint(){
        var tri=new HostTri(new float[]{-.2f,0,.71f,.5f,0,.71f,.5f,1,.71f},0,
            TRI_DIGGABLE|(DIG_STONE<<TRI_MATERIAL_SHIFT));
        var poly=dev.libertycraft.GroveArchitectureCap.footprint(tri,net.minecraft.core.BlockPos.ZERO,net.minecraft.core.Direction.NORTH);
        org.junit.jupiter.api.Assertions.assertFalse(poly.isEmpty());
        for(var v:poly){org.junit.jupiter.api.Assertions.assertTrue(v[0]>=0&&v[0]<=.5&&v[1]>=0&&v[1]<=1);}
        assertEquals(java.util.List.of(),dev.libertycraft.GroveArchitectureCap.footprint(tri,new net.minecraft.core.BlockPos(0,0,2),net.minecraft.core.Direction.NORTH));
        assertEquals(java.util.List.of(),dev.libertycraft.GroveArchitectureCap.footprint(tri,net.minecraft.core.BlockPos.ZERO,net.minecraft.core.Direction.EAST));
    }
    @Test void slopedFacadeCapClipsDepthBeforeProjecting(){
        var tri=new HostTri(new float[]{0,0,.8f,1,0,1.2f,0,1,.8f},0,
            TRI_DIGGABLE|(DIG_STONE<<TRI_MATERIAL_SHIFT));
        var poly=dev.libertycraft.GroveArchitectureCap.footprint(tri,net.minecraft.core.BlockPos.ZERO,net.minecraft.core.Direction.NORTH);
        org.junit.jupiter.api.Assertions.assertFalse(poly.isEmpty());
        for(var v:poly)org.junit.jupiter.api.Assertions.assertTrue(v[0]<=.50001,"Outside cut depth must not be projected into cap");
    }
    private HostDig.Probe probe(float wallBottom, float wallTop) {
        var probe = new HostDig.Probe();
        probe.land.add(new HostTri(new float[]{-4,0,-4, -4,0,4, 4,0,-4}, 0,
            TRI_DIGGABLE | TRI_TERRAIN | (DIG_GRASS << TRI_MATERIAL_SHIFT)));
        // Outward +X: a query on its -X side lies behind a protective wall.
        probe.tris.add(new HostTri(new float[]{.25f,wallBottom,-1, .25f,wallTop,-1,
            .25f,wallBottom,1}, 0, 0));
        return probe;
    }
    @Test void outdoorAirIsNotInsideTheFartherBuriedWall() {
        assertEquals(HostDig.AIR, probe(-2,-.1f).test(0,.05,0));
    }
    @Test void actualBuildingWallStillBlocksTheBlast() {
        assertEquals(HostDig.KEEP, probe(0,2).test(0,1,0));
    }
    @Test void undergroundRemainsItsLandMaterial() {
        assertEquals(DIG_GRASS, probe(-2,-.1f).test(0,-1,0));
    }
    @Test void unknownOutdoorGeometryDoesNotInventStone() {
        assertEquals(HostDig.AIR, new HostDig.Probe().test(0,20,0));
    }
    @Test void diggableArchitectureNeverBecomesProceduralStone() {
        var p=new HostDig.Probe();
        p.tris.add(new HostTri(new float[]{.25f,0,-1,.25f,2,-1,.25f,0,1},0,
            TRI_DIGGABLE | (DIG_STONE << TRI_MATERIAL_SHIFT)));
        assertEquals(HostDig.KEEP,p.test(0,1,0));
        assertEquals(HostDig.AIR,p.test(.5,1,0));
    }
}
