package dev.libertycraft;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class GroveFoliageLodTest {
    @Test void fullFoliageReducesEightCubesWithoutInventingVolume(){
        var mask=GroveFoliageLod.merged(i->7);assertEquals(4096,mask.cardinality());
        assertEquals(512,mask.cardinality()/8);
    }
    @Test void holesTrunksAndMixedLeavesKeepTheirOriginalGeometry(){
        int[] cells=new int[4096];for(int y=0;y<2;y++)for(int z=0;z<2;z++)for(int x=0;x<2;x++)cells[GroveFoliageLod.index(x,y,z)]=7;
        assertEquals(8,GroveFoliageLod.merged(i->cells[i]).cardinality());
        cells[0]=0;assertEquals(0,GroveFoliageLod.merged(i->cells[i]).cardinality());
        cells[0]=8;assertEquals(0,GroveFoliageLod.merged(i->cells[i]).cardinality());
    }
    @Test void hysteresisPreventsRemeshingAtTheBoundary(){
        assertFalse(GroveFoliageLod.coarse(120,false));assertTrue(GroveFoliageLod.coarse(120,true));
        assertTrue(GroveFoliageLod.coarse(129,false));assertFalse(GroveFoliageLod.coarse(95,true));
        assertFalse(GroveFoliageLod.coarse(Double.NaN,true));
    }
}
