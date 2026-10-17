package dev.libertycraft;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import static org.junit.jupiter.api.Assertions.*;
import java.nio.file.*;
import java.util.*;
class GroveWorldPlanTest {
    @TempDir Path folder;
    @Test void resolvedParcelsCrossSpatialBoundariesWithoutMovingLedgerOwners()throws Exception{
        long seed=77;
        var east=new GroveEcologyRules.Site("east",191,0,GroveEcologyRules.Kind.HAMLET,9);
        var west=new GroveEcologyRules.Site("west",-193,-1,GroveEcologyRules.Kind.OUTPOST,10);
        var rejected=new GroveEcologyRules.Site("rejected",239,0,GroveEcologyRules.Kind.OAK,11);
        var text=new StringBuilder("1\t"+seed+"\n");
        for(var s:List.of(east,west,rejected))text.append(s.key()).append('\t').append(s.x()).append('\t').append(s.z())
            .append('\t').append(s.kind()).append('\t').append(s.seed()).append('\n');
        java.nio.file.Files.writeString(folder.resolve("grovecraft-world-plan-v1.tsv"),text);
        float[] flat={20,20,20,20};
        var atlas=GroveSurfaceAtlas.open(GroveSurfaceAtlasTest.cache(folder.resolve("atlas"),0,0,2,flat,flat));
        java.nio.file.Files.writeString(folder.resolve("grovecraft-world-placements-v2.tsv"),
            "2\t"+seed+"\t"+atlas.identity()+"\neast\t239\t0\t20\nwest\t-145\t47\t20\nrejected\t239\t0\tNaN\n");
        for(int reload=0;reload<2;reload++){
            var plan=GroveWorldPlan.loadOrCreate(folder,seed);plan.resolve(folder,atlas);
            assertEquals(List.of(east),plan.nearby(239,0,8));
            assertEquals(List.of(west),plan.nearby(-145,47,8));
            assertTrue(plan.nearby(191,0,8).isEmpty());assertTrue(plan.nearby(-193,-1,8).isEmpty());
            assertEquals(east,plan.nearby(239,0,8).getFirst()); // Stable ledger, not shifted owner.
            assertEquals(239,plan.location(east).x());
            var expected=plan.all().stream().filter(s->{var p=plan.placement(s);return Double.isFinite(p.height())
                &&Math.hypot(p.site().x()-200,p.site().z())<=176;}).toList();
            assertEquals(new HashSet<>(expected),new HashSet<>(plan.nearby(200,0,176)));
            assertEquals(2,plan.resolvedCount());
            var markers=new GroveStructureMap();markers.addPlanned(plan);
            assertEquals(2,markers.size()); // Both destinations before any chunk is visited.
            assertTrue(markers.nearest(0,0).stream().anyMatch(p->p.x()==239&&p.kind()==1));
            markers.remember(new GroveStructureMap.Point("east",1,220,21,7));
            markers.addPlanned(plan);
            assertTrue(markers.nearest(0,0).stream().anyMatch(p->p.key().equals("east")&&p.x()==220));
            assertFalse(markers.contains("rejected"));
        }
    }
    @Test void worldPlanSurvivesReloadWithoutDuplicateIdentities()throws Exception{
        var first=GroveWorldPlan.loadOrCreate(folder,812);var again=GroveWorldPlan.loadOrCreate(folder,812);
        assertTrue(first.size()>1000);assertEquals(first.all(),again.all());
        assertEquals(first.size(),new HashSet<>(first.all().stream().map(GroveEcologyRules.Site::key).toList()).size());
        assertThrows(java.io.IOException.class,()->GroveWorldPlan.loadOrCreate(folder,813));
    }
    @Test void spatialQueriesMatchWholeMapAndPreservePreviousSiteCoordinates(){
        var plan=GroveWorldPlan.create(914);double x=650,z=-900,radius=176;
        var expected=plan.all().stream().filter(s->Math.hypot(s.x()-x,s.z()-z)<=radius).toList();
        assertEquals(new HashSet<>(expected),new HashSet<>(plan.nearby(x,z,radius)));
        for(var s:plan.all())if(s.key().startsWith("w:")){
            var a=s.key().split(":");assertEquals(s,GroveEcologyRules.woodlandSite(914,Integer.parseInt(a[1]),Integer.parseInt(a[2])));
            assertFalse(GroveWorldPlan.reserved(914,s));
        }
    }
}
