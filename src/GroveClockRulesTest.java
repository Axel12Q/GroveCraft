package dev.libertycraft;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class GroveClockRulesTest {
    @Test void firstNightDoesNotGetClampedToDawn(){assertEquals(16000,GroveClockRules.correctedTicks(0,-8000));}
    @Test void firstDawnWrapDoesNotLoseNight(){assertEquals(20000,GroveClockRules.correctedTicks(8000,-12000));}
    @Test void existingDaysKeepNearestCorrection(){assertEquals(40000,GroveClockRules.correctedTicks(42000,-2000));}
}
