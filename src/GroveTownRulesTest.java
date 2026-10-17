package dev.libertycraft;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class GroveTownRulesTest {
    @Test void boundedDailyIncome(){assertTrue(GroveTownRules.canEarn(920,80));assertFalse(GroveTownRules.canEarn(921,80));assertFalse(GroveTownRules.canEarn(0,-1));assertFalse(GroveTownRules.canEarn(Integer.MAX_VALUE,80));}
    @Test void pricesNeverCreateFreeGoods(){assertEquals(40,GroveTownRules.price(40,2));assertEquals(36,GroveTownRules.price(40,3));assertEquals(1,GroveTownRules.price(1,100));}
    @Test void deliveryMustRequireMeaningfulButReasonableTravel(){assertFalse(GroveTownRules.route(0,0,299,0));assertTrue(GroveTownRules.route(0,0,300,0));assertFalse(GroveTownRules.route(0,0,2501,0));assertFalse(GroveTownRules.route(Double.NaN,0,400,0));}
    @Test void pausedAndDeathActionsCannotSpend(){int base=9|(1<<17);assertTrue(GroveTownRules.safe(base|(1<<19),true));assertFalse(GroveTownRules.safe(base|(1<<19),false));for(int bit:new int[]{1<<4,1<<16,1<<21,1<<22,1<<23,1<<24,1<<2})assertFalse(GroveTownRules.safe(base|bit,true));}
}
