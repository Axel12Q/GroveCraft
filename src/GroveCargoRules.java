package dev.libertycraft;
import dev.libertycraft.link.Proto;
/** Menu ownership rules, independent of Minecraft registries. */
public final class GroveCargoRules {
    private GroveCargoRules() {}
    public static boolean safeFlags(int flags,boolean ownMenu) {
        return (flags&Proto.SKY_IN_GAME)!=0
            &&(flags&(Proto.SKY_LOADING|Proto.SKY_MENU_OPEN|Proto.SKY_IN_VEHICLE|(1<<16)|(1<<21)|(1<<22)|(1<<23)|(1<<24)))==0
            &&(ownMenu||(flags&(1<<19))==0);
    }
}
