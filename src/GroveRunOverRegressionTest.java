package dev.libertycraft;

import dev.libertycraft.combat.RunOver;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class GroveRunOverRegressionTest {
    private RunOver.Pose car(double x,double z,double yaw){return new RunOver.Pose(x,z,10,12,yaw,2.3,.9);}
    @Test void cowCrossedBetweenFramesStillReceivesImpact(){
        var hit=RunOver.contact(car(0,6,0),car(0,-6,0),.3,0,0,0,0,.45,10,11.5);
        assertTrue(hit.hit());assertEquals(40,hit.speed(),1e-6);assertTrue(RunOver.damage(hit.closing())>20);
    }
    @Test void cowBesideSweptPathIsNotHit(){
        assertFalse(RunOver.contact(car(0,6,0),car(0,-6,0),.3,3,0,3,0,.45,10,11.5).touching());
    }
    @Test void scriptedTeleportDoesNotBecomeRunOver(){
        assertFalse(RunOver.contact(car(0,60,0),car(0,-60,0),.3,0,0,0,0,.45,10,11.5).hit());
    }
}
