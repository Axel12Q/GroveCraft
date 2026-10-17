package dev.libertycraft;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class GroveNativeTreeRulesTest {
    @Test void sevenDaysAndDistanceBothRequired(){
        assertFalse(GroveNativeTreeRules.regrow(167999,0,20000));
        assertFalse(GroveNativeTreeRules.regrow(168000,0,96*96));
        assertTrue(GroveNativeTreeRules.regrow(168000,0,97*97));
        assertFalse(GroveNativeTreeRules.regrow(0,100,20000));
    }
    @Test void packedFallPreservesCountAndDirectionWithSignBit(){
        int packed=(7<<29)|(192<<21)|(16<<16)|(short)404;
        assertEquals(16,GroveNativeTreeRules.count(packed));assertEquals(28,GroveNativeTreeRules.length(packed));
        assertEquals(1.5*Math.PI,GroveNativeTreeRules.heading(packed),1e-9);
        assertEquals(404,(short)packed);
    }
    @Test void dropsCoverTrunkInsteadOfRootPile(){
        assertEquals(.875,GroveNativeTreeRules.along(0,16,28));
        assertEquals(27.125,GroveNativeTreeRules.along(15,16,28));
    }
}
