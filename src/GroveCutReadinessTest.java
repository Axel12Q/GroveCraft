package dev.libertycraft.world;

import java.util.HashSet;
import net.minecraft.core.BlockPos;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class GroveCutReadinessTest {
    @Test void emptySectionsDoNotBlockOrdinaryBuilding(){
        assertTrue(GroveCutReadiness.ready(0,0,0,null,(x,y,z)->false));
        assertTrue(GroveCutReadiness.ready(0,0,0,new long[64],(x,y,z)->false));
    }
    @Test void aSingleMissingNeighbourMustPreserveTheOldLining(){
        var bits=new long[64];bits[0]=1;var regions=new HashSet<Long>();
        assertTrue(GroveCutReadiness.ready(-1,1,-1,bits,(x,y,z)->{regions.add(BlockPos.asLong(x,y,z));return true;}));
        assertEquals(8,regions.size()); // a cut exactly at 8-metre boundaries
        for(long missing:regions)
            assertFalse(GroveCutReadiness.ready(-1,1,-1,bits,(x,y,z)->BlockPos.asLong(x,y,z)!=missing));
    }
    @Test void allDugCellsAreCoveredAndDenseHolesDeduplicateQueries(){
        var bits=new long[64];java.util.Arrays.fill(bits,-1L);var regions=new HashSet<Long>();
        assertTrue(GroveCutReadiness.ready(-2,-1,3,bits,(x,y,z)->{
            assertTrue(regions.add(BlockPos.asLong(x,y,z)),"No repeated region lookup per section");return true;
        }));
        assertEquals(64,regions.size());
        assertFalse(GroveCutReadiness.ready(-2,-1,3,bits,(x,y,z)->x!=-2));
    }
}
