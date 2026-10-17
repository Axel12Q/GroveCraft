package dev.libertycraft;
import dev.libertycraft.link.Proto;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class GroveCargoTest {
    @Test void ownGuestMenuMustNotCloseItsCargo() {
        assertTrue(GroveCargoRules.safeFlags(Proto.SKY_IN_GAME|(1<<19),true));
        assertFalse(GroveCargoRules.safeFlags(Proto.SKY_IN_GAME|(1<<19),false));
    }
    @Test void gtaPauseMissionVehicleAndLoadingAlwaysInvalidateCargo() {
        for(int bit:new int[]{Proto.SKY_MENU_OPEN,Proto.SKY_LOADING,Proto.SKY_IN_VEHICLE,1<<16,1<<21,1<<22,1<<23,1<<24})
            assertFalse(GroveCargoRules.safeFlags(Proto.SKY_IN_GAME|bit,true));
        assertFalse(GroveCargoRules.safeFlags(0,true));
    }
}
