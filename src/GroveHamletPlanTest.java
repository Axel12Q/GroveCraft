package dev.libertycraft;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import java.util.*;
class GroveHamletPlanTest {
    private static final GroveEcologyRules.Site SITE=new GroveEcologyRules.Site("s:0:0",0,0,GroveEcologyRules.Kind.HAMLET,0);
    @Test void separateHousePlatformsFollowGentleHill(){
        var p=GroveHamletPlan.fit(SITE,20,(x,z)->20+x*.18);
        assertTrue(p.valid(),p.failure());assertNotEquals(p.west(),p.east());
        assertTrue(p.cells().size()<768);
        for(var c:p.cells())assertTrue(p.base()+c.y()>=Math.floor(p.ground().get(new GroveHamletPlan.Column(c.x(),c.z()))));
    }
    @Test void missingGroundOutsideBlueprintDoesNotRejectButMissingSupportDoes(){
        assertTrue(GroveHamletPlan.fit(SITE,20,(x,z)->x==10&&z==10?Double.NaN:20).valid());
        assertEquals("suelo no apto",GroveHamletPlan.fit(SITE,20,(x,z)->x==-8&&z==-4?Double.NaN:20).failure());
    }
    @Test void cliffsAndAbruptEntranceStepsFailClosed(){
        assertFalse(GroveHamletPlan.fit(SITE,20,(x,z)->20+x*.7).valid());
        assertFalse(GroveHamletPlan.fit(SITE,20,(x,z)->x<=-3?22:20).valid());
    }
    @Test void stableBoundedSearchKeepsOriginalOwnership(){
        Set<String> positions=new HashSet<>();
        for(int i=0;i<=24;i++){
            var p=GroveHamletPlan.nearby(SITE,i);assertEquals(SITE.key(),p.key());
            assertTrue(Math.abs(p.x())<=48&&Math.abs(p.z())<=48);
            assertTrue(positions.add(p.x()+":"+p.z()));assertEquals(p,GroveHamletPlan.nearby(SITE,i));
        }
        assertThrows(IllegalArgumentException.class,()->GroveHamletPlan.nearby(SITE,25));
    }
}
