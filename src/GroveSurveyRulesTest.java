package dev.libertycraft;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import java.util.*;
class GroveSurveyRulesTest {
    @Test void relocatedStructuresArePrioritizedByActualParcelWithoutChangingIdentity(){
        var shifted=new GroveEcologyRules.Site("hamlet",200,0,GroveEcologyRules.Kind.HAMLET,1);
        var tree=new GroveEcologyRules.Site("tree",48,0,GroveEcologyRules.Kind.OAK,2);
        var actual=new GroveEcologyRules.Site(shifted.key(),104,0,shifted.kind(),shifted.seed());
        var ordered=GroveSurveyRules.order(List.of(shifted,tree),0,0,0,0,s->s==shifted?actual:s);
        assertSame(shifted,ordered.getFirst());assertEquals(200,ordered.getFirst().x());
        assertEquals(2,ordered.stream().map(GroveEcologyRules.Site::key).distinct().count());
    }
    @Test void fastVehiclesKeepBoundedForwardPreparationButTeleportsReset(){
        assertArrayEquals(new double[]{96,0},GroveSurveyRules.lookAhead(96,0));
        assertArrayEquals(new double[]{128,96},GroveSurveyRules.lookAhead(240,180),.000001);
        assertArrayEquals(new double[]{0,0},GroveSurveyRules.lookAhead(1000,0));
        assertArrayEquals(new double[]{0,0},GroveSurveyRules.lookAhead(Double.NaN,0));
    }
    @Test void prepareVisibleDistanceBeforeNearPopIn(){
        assertTrue(GroveSurveyRules.rank(104,0,0,0)<GroveSurveyRules.rank(16,0,0,0));
        assertTrue(GroveSurveyRules.rank(120,0,96,0)<GroveSurveyRules.rank(-120,0,96,0));
    }
    @Test void nearbyFallbackSurvivesUnknownFarGeometry(){
        var sites=new ArrayList<GroveEcologyRules.Site>();
        for(int x:new int[]{8,96,112,128,144})sites.add(new GroveEcologyRules.Site("w:"+x,x,0,GroveEcologyRules.Kind.OAK,0));
        var order=GroveSurveyRules.order(sites,0,0,0,0);
        assertEquals(5,order.size());assertEquals(5,order.stream().map(GroveEcologyRules.Site::key).distinct().count());
        assertEquals(8,order.get(2).x());
    }
    @Test void sparseExpansionPreservesOlderSitesAndHasAccessibleOutpost(){
        var old=GroveEcologyRules.site(0,3,-1,false);
        var p=GroveEcologyRules.expansionSite(0,3,-1);
        assertEquals(p,GroveEcologyRules.expansionSite(0,3,-1));assertTrue(p.key().startsWith("p:"));
        assertEquals(old,GroveEcologyRules.site(0,3,-1,false));
        var plan=GroveEcologyRules.plan(new GroveEcologyRules.Site("p",0,0,GroveEcologyRules.Kind.OUTPOST,0));
        assertFalse(plan.stream().anyMatch(c->c.x()==0&&c.z()==3&&(c.y()==1||c.y()==2)));
        assertEquals(2,plan.stream().filter(c->c.block().equals("barrel")).count());
        assertTrue(plan.stream().anyMatch(c->c.block().equals("stairs_north")&&c.y()==7));
        assertFalse(plan.stream().anyMatch(c->c.x()==3&&c.y()==9&&c.z()==-3));
    }
    @Test void depotWorkstationHasWalkableEntry(){
        var plan=GroveEcologyRules.plan(new GroveEcologyRules.Site("p",0,0,GroveEcologyRules.Kind.WAYSTATION,0));
        assertTrue(plan.stream().anyMatch(c->c.x()==-2&&c.y()==0&&c.z()==-3&&c.block().equals("cobble_stairs_south")));
        assertFalse(plan.stream().anyMatch(c->c.x()==-2&&c.y()==1&&c.z()==-3));
        assertTrue(plan.stream().anyMatch(c->c.x()==-1&&c.y()==0&&c.z()==4&&c.block().equals("smithing_table")));
        assertEquals(1,plan.stream().filter(c->c.block().equals("smithing_table")).count());
        assertTrue(plan.stream().anyMatch(c->c.y()==-2&&c.z()==6&&c.block().equals("oak_stairs")));
    }
}
