package dev.libertycraft;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import java.util.*;

class GroveEcologyRulesTest {
    @Test void denseWoodlandsCoverChiliadAndFlintButNotCitiesOrTowns() {
        for(int[] p:new int[][]{{-2400,-1700},{-1000,-2400},{600,-200},{-2400,1900}})
            assertTrue(GroveEcologyRules.woodland(p[0],-p[1],4),Arrays.toString(p));
        for(int[] p:new int[][]{{2495,-1680},{-2300,500},{2100,1800},{720,-540},{-2190,-2300}})
            assertFalse(GroveEcologyRules.woodland(p[0],-p[1],4),Arrays.toString(p));
    }
    @Test void supplementalGridIsDenseStableAndDoesNotReuseOldLedgerKeys() {
        Set<String> keys=new HashSet<>();
        for(int x=-10;x<10;x++)for(int z=-10;z<10;z++) {
            var s=GroveEcologyRules.woodlandSite(0,x,z);
            assertEquals(s,GroveEcologyRules.woodlandSite(0,x,z));
            assertTrue(s.x()>=x*16+4&&s.x()<x*16+12);
            assertTrue(s.z()>=z*16+4&&s.z()<z*16+12);
            assertTrue(keys.add(s.key()));assertTrue(s.key().startsWith("w:"));
            assertFalse(s.key().equals(GroveEcologyRules.site(0,x,z,true).key()));
        }
    }
    @Test void coincidentBackFaceMaterialDoesNotHideGrassButRoofsStillDo() {
        assertTrue(GroveEcologyRules.grassIsTop(17.449566,17.449567));
        assertFalse(GroveEcologyRules.grassIsTop(17.449566,17.46));
        assertFalse(GroveEcologyRules.grassIsTop(17.449566,24));
        assertFalse(GroveEcologyRules.grassIsTop(Double.NaN,17));
    }
    @Test void planningIsStableAcrossVisitOrdersAndNegativeCoordinates() {
        var a=GroveEcologyRules.site(79,-16,12,true);
        GroveEcologyRules.site(79,30,-22,false);
        assertEquals(a,GroveEcologyRules.site(79,-16,12,true));
        assertNotEquals(a,GroveEcologyRules.site(80,-16,12,true));
        assertTrue(a.x()>=-16*32+8 && a.x()<-16*32+24);
    }
    @Test void wholeBlueprintFitsValidatedEnclosureAndBudget() {
        for(var k:GroveEcologyRules.Kind.values()) {
            var plan=GroveEcologyRules.plan(new GroveEcologyRules.Site("test",0,0,k,17));
            int r=GroveEcologyRules.radius(k),h=GroveEcologyRules.height(k);
            assertFalse(plan.isEmpty());assertTrue(plan.size()<=768,k.toString());
            Set<String> unique=new HashSet<>();
            for(var cell:plan) {
                assertTrue(Math.abs(cell.x())<=r && Math.abs(cell.z())<=r,k+" "+cell);
                assertTrue(cell.y()>=(k==GroveEcologyRules.Kind.WAYSTATION?-2:0) && cell.y()<=h);
                assertTrue(unique.add(cell.x()+":"+cell.y()+":"+cell.z()));
            }
        }
    }
    @Test void protectCitiesAirportsAndRuralMissionHubs() {
        for(int[] gta:new int[][]{{2495,-1680},{1994,-1218},{-2700,500},{2100,1800},{400,2500},{200,1900},{650,-570},{1250,-850},{-790,-1500}})
            assertFalse(GroveEcologyRules.countryside(gta[0],-gta[1],12),Arrays.toString(gta));
        assertTrue(GroveEcologyRules.countryside(-1200,1900,12));
        assertTrue(GroveEcologyRules.countryside(1000,100,12));
        assertFalse(GroveEcologyRules.countryside(-649,1900,12));
    }
    @Test void surveyWhileDrivingAndTravelMissionsButSuspendInMenusAndInteriors() {
        assertTrue(GroveEcologyRules.safe(9,0,true,false));
        assertTrue(GroveEcologyRules.safe(9|(1<<4),0,true,false));
        assertTrue(GroveEcologyRules.safe(9|(1<<4)|(1<<16),0,true,false));
        for(int flag:new int[]{1<<19,1<<21,1<<1,1<<2,1<<5})assertFalse(GroveEcologyRules.safe(9|flag,0,true,false));
        assertFalse(GroveEcologyRules.safe(9,1,true,false));
        assertFalse(GroveEcologyRules.safe(9,0,false,false));
        assertFalse(GroveEcologyRules.safe(9,0,true,true));
    }
    @Test void cabinsHaveOpenDoorwaysAndFiniteLootContainers() {
        var plan=GroveEcologyRules.plan(new GroveEcologyRules.Site("v",0,0,GroveEcologyRules.Kind.HAMLET,6));
        assertEquals(2,plan.stream().filter(c->c.block().equals("bed_head")).count());
        assertEquals(3,plan.stream().filter(c->c.block().equals("barrel")).count());
        assertTrue(plan.stream().anyMatch(c->c.y()==6&&c.block().equals("oak_slab")));
        assertTrue(plan.stream().anyMatch(c->c.y()==3&&c.block().equals("oak_log")));
        for(int x:new int[]{-6,5})for(int y:new int[]{1,2})assertFalse(plan.stream().anyMatch(c->c.x()==x&&c.y()==y&&c.z()==1));
    }
}
