package dev.libertycraft;
import net.minecraft.world.phys.AABB;

public final class GroveMeleeRules {
    private GroveMeleeRules(){}
    public static boolean reachable(AABB a,AABB b){
        double dx=Math.max(0,Math.max(a.minX-b.maxX,b.minX-a.maxX));
        double dz=Math.max(0,Math.max(a.minZ-b.maxZ,b.minZ-a.maxZ));
        double dy=Math.max(0,Math.max(a.minY-b.maxY,b.minY-a.maxY));
        return dx*dx+dz*dz<=.75*.75&&dy<=.5;
    }
}
