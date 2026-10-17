package dev.libertycraft;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import java.util.*;
class GroveStructureMapTest {
    @Test void markersPreserveRelocatedCoordinatesAndSurviveLedgerReload(){
        var map=new GroveStructureMap();
        var point=new GroveStructureMap.Point("s:-1:2",1,239,22,-199);
        map.remember(point);map.remember(point);
        var reloaded=new GroveStructureMap();reloaded.restore(map.ledger());
        assertEquals(List.of(point),reloaded.nearest(0,0));assertEquals(1,reloaded.size());
        assertEquals(List.of(),new GroveStructureMap().nearest(0,0));
    }
    @Test void malformedOrNonStructureLedgerRowsCannotInventMarkers(){
        var map=new GroveStructureMap();map.restore(List.of("s:0:0","felled:w:1:2","map-v1:x\t0\t0\t0\t0",
            "map-v1:x\t1\t2147483647\t0\t0","map-v1:x\t1\t0\t1001\t0","map-v1:x\t1\tNaN\t0\t0"));
        assertEquals(0,map.size());assertEquals(0,GroveStructureMap.kind(GroveEcologyRules.Kind.OAK));
        assertEquals(0,GroveStructureMap.kind(GroveEcologyRules.Kind.BIRCH));
        for(var kind:GroveEcologyRules.Kind.values())if(kind!=GroveEcologyRules.Kind.OAK&&kind!=GroveEcologyRules.Kind.BIRCH)
            assertTrue(GroveStructureMap.kind(kind)>0);
    }
    @Test void selectNearest64WithoutDeletingDistantDiscoveries(){
        var map=new GroveStructureMap();for(int i=0;i<80;i++)map.remember(new GroveStructureMap.Point("s:"+i,1,i*10,20,0));
        var selected=map.nearest(0,0);assertEquals(64,selected.size());assertTrue(selected.stream().allMatch(p->p.x()<=630));
        assertEquals(80,map.size());assertTrue(map.nearest(790,0).stream().anyMatch(p->p.x()==790));
        assertEquals(selected,map.nearest(.1,0)); // Stable order prevents handle churn while moving.
    }
}
