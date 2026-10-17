package dev.libertycraft;
import org.junit.jupiter.api.Test;
import net.minecraft.world.phys.AABB;
import static org.junit.jupiter.api.Assertions.*;

class GroveMeleeRulesTest {
    private AABB zombie(double z,double y){return new AABB(-.3,y,z-.3,.3,y+1.95,z+.3);}
    private final AABB carFront=new AABB(-.95,0,.55,.95,1.5,2.45);
    @Test void wideVehicleDoesNotGrantDistantMelee(){assertFalse(GroveMeleeRules.reachable(zombie(4.5,0),carFront));}
    @Test void mobCanStrikeActualVehicleExterior(){assertTrue(GroveMeleeRules.reachable(zombie(3.4,0),carFront));}
    @Test void mobBelowBridgeCannotStrikeCarAbove(){assertFalse(GroveMeleeRules.reachable(zombie(2, -3),carFront));}
}
