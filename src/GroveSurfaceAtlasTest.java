package dev.libertycraft;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import static org.junit.jupiter.api.Assertions.*;
import java.nio.*;
import java.nio.file.*;
class GroveSurfaceAtlasTest {
    @TempDir Path folder;
    static Path cache(Path path,int x0,int z0,int size,float[] ground,float[] top)throws Exception{
        var bytes=ByteBuffer.allocate(24+8*size*size).order(ByteOrder.LITTLE_ENDIAN);
        bytes.putInt(0x31415347).putInt(1).putInt(x0).putInt(z0).putInt(2).putInt(size);
        for(float h:ground)bytes.putFloat(h);for(float h:top)bytes.putFloat(h);
        Files.write(path,bytes.array());return path;
    }
    @Test void interpolateSlopesWithoutInventingGroundInMissingColumns()throws Exception{
        float[] heights={10,12,Float.NaN,14,16,18,18,20,22};
        var atlas=GroveSurfaceAtlas.open(cache(folder.resolve("surface"),0,0,3,heights,heights));
        assertEquals(13,atlas.ground(1,1));assertEquals(12,atlas.ground(2,0));
        assertFalse(atlas.known(3,0));assertFalse(atlas.known(-1,0));
        assertTrue(atlas.occupied(1,13,1));assertFalse(atlas.occupied(1,14,1));
        assertThrows(java.io.IOException.class,()->GroveSurfaceAtlas.open(Files.write(folder.resolve("bad"),new byte[3])));
    }
    @Test void resolvedPlacementsSurviveReloadAndRejectAnotherTerrainCache()throws Exception{
        int n=64;float[] g=new float[n*n];java.util.Arrays.fill(g,20);
        var atlas=GroveSurfaceAtlas.open(cache(folder.resolve("surface"),600,-900,n,g,g));
        var plan=GroveWorldPlan.loadOrCreate(folder,91);plan.resolve(folder,atlas);assertTrue(plan.resolvedCount()>0);
        var reloaded=GroveWorldPlan.loadOrCreate(folder,91);reloaded.resolve(folder,atlas);
        assertEquals(plan.resolvedCount(),reloaded.resolvedCount());
        for(var s:plan.all())assertEquals(plan.placement(s),reloaded.placement(s));
        g[0]=19;var other=GroveSurfaceAtlas.open(cache(folder.resolve("other"),600,-900,n,g,g));
        assertThrows(java.io.IOException.class,()->GroveWorldPlan.loadOrCreate(folder,91).resolve(folder,other));
    }
}
