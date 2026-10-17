package dev.libertycraft;

/** The first Minecraft day must wrap to tonight, never clamp tonight to dawn. */
public final class GroveClockRules {
    private GroveClockRules(){}
    public static long correctedTicks(long now,long correction){
        long next=now+correction;
        return next<0?next+24000:next;
    }
}
