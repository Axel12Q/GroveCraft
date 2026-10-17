package dev.libertycraft;
import static org.junit.jupiter.api.Assertions.*;
import org.junit.jupiter.api.Test;
class GroveOreRulesTest {
    @Test void shallowAndUnknownGroundCannotExposeRareOre(){
        for(int x=-100;x<100;x++)for(int z=-100;z<100;z++){
            assertEquals("stone",GroveOreRules.ore(7,x,12,z,4));
            assertEquals("stone",GroveOreRules.ore(7,x,12,z,Double.NaN));
            String shallow=GroveOreRules.ore(7,x,12,z,9);
            assertFalse(shallow.equals("diamond_ore")||shallow.equals("iron_ore")||shallow.equals("gold_ore"));
        }
    }
    @Test void deepVeinsAreStableAndDiamondsRemainScarce(){
        int diamonds=0;
        for(int x=-150;x<150;x++)for(int z=-150;z<150;z++){
            var ore=GroveOreRules.ore(19,x,-14,z,35);assertEquals(ore,GroveOreRules.ore(19,x,-14,z,35));
            if(ore.equals("diamond_ore"))diamonds++;
        }
        assertTrue(diamonds>0&&diamonds<90,"diamond density below 0.1 percent");
    }
}
