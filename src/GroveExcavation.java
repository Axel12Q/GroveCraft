package dev.libertycraft.client;

import dev.libertycraft.link.Link;
import dev.libertycraft.world.HostClip;
import dev.libertycraft.world.HostRay;
import net.minecraft.client.Minecraft;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.world.phys.Vec3;

/** Native reticle targeting, with upstream timed mining and persistent chunk masks. */
public final class GroveExcavation {
    private static Vec3 target;
    private static long deadline;
    private static HostRay.Hit currentHit;
    private static int directWood,directFace;
    private static BlockPos nativeCell;
    private static long nextDiagnostic;
    public static HostRay.Hit hit() { return currentHit; }
    public static void target(int x, int y, int z) {
        directWood=0;
        nativeCell=null;
        target = new Vec3(x / 256.0, y / 256.0, z / 256.0);
        deadline = System.nanoTime() + 200_000_000L;
    }
    public static void nativeWood(int code,int x,int y,int z){
        target(x,y,z);int packed=code-6000,material=packed&63,face=packed>>>6;
        if(face<6&&dev.libertycraft.GroveNativeTrees.wood(material)){directWood=material;directFace=face;}
    }
    public static void nativeCell(int code,int x,int y,int z){
        if(code<7000||code>=7006||Math.abs((long)x)>5000||Math.abs((long)z)>5000||Math.abs((long)y)>1000)return;
        target(x*256+128,y*256+128,z*256+128);nativeCell=new BlockPos(x,y,z);
    }
    public static void nativePoint(int x,int y,int z){
        if(nativeCell==null)return;
        Vec3 point=new Vec3(x/256.0,y/256.0,z/256.0);
        if(point.distanceToSqr(Vec3.atCenterOf(nativeCell))>1.1)return;
        target=point;
    }
    private static void diagnostic(String reason,HostRay.Hit hit){
        if(nativeCell==null||System.nanoTime()<nextDiagnostic
            ||!"GroveCraftGtaTestCuts20261014".equals(System.getProperty("grovecraft.testWorld")))return;
        nextDiagnostic=System.nanoTime()+1_000_000_000L;
        var mc=Minecraft.getInstance();var tri=hit==null?null:hit.tri();
        dev.libertycraft.LibertyCraft.LOG.info("GROVE_NATIVE_MINE reason={} cell={} target={} hit={} diggable={} terrain={} material={} mode={} destruction={} block={} dug={} architectureCut={}",
            reason,nativeCell,target,hit,tri!=null&&tri.diggable,tri!=null&&tri.terrain,tri==null?-1:tri.material,
            mc.gameMode.getPlayerMode(),dev.libertycraft.world.HostDig.destruction,mc.level.getBlockState(nativeCell),
            dev.libertycraft.world.HostDig.isDug(mc.level,HostDigClient.world(),nativeCell),
            dev.libertycraft.GroveArchitecture.cut(mc.level,HostDigClient.world(),nativeCell));
    }
    public static int[] surfaceCell(HostRay.Hit hit){
        if(hit==currentHit&&nativeCell!=null)return new int[]{nativeCell.getX(),nativeCell.getY(),nativeCell.getZ()};
        return HostRay.surfaceCell(hit);
    }
    public static void stop() { target = null; currentHit = null; nativeCell=null; directWood=0; }
    public static void frame(Minecraft minecraft) {
        currentHit = null;
        if (target == null || System.nanoTime() > deadline || !Link.active()
                || minecraft.player == null || minecraft.level == null || minecraft.gui.screen() != null
                || HostClient.sky().menuOpen() || (HostClient.sky().flags & ((1 << 4) | (1 << 21) | (1 << 26))) != 0) return;
        Vec3 from = minecraft.player.getEyePosition();
        if (from.distanceToSqr(target) > 64) return;
        Vec3 to = target.add(target.subtract(from).normalize().scale(.35));
        HostRay.Hit hit;
        if(directWood>0){
            // CWorld already picked this exact native tree surface. Recasting
            // coarse sampled triangles can move the hit by over a metre and
            // reject a perfectly valid bark hit before its column is ready.
            var n=Direction.values()[directFace];
            Vec3 normal=new Vec3(n.getStepX(),n.getStepY(),n.getStepZ());
            Vec3 u=Math.abs(n.getStepY())==1?new Vec3(1,0,0):new Vec3(0,1,0),v=normal.cross(u);
            var a=target;var b=a.add(u);var c=a.add(v);
            var tri=new dev.libertycraft.world.HostTri(new float[]{(float)a.x,(float)a.y,(float)a.z,
                (float)b.x,(float)b.y,(float)b.z,(float)c.x,(float)c.y,(float)c.z},0,
                dev.libertycraft.link.Proto.TRI_DIGGABLE|(directWood<<dev.libertycraft.link.Proto.TRI_MATERIAL_SHIFT));
            hit=new HostRay.Hit(1,target.x,target.y,target.z,normal.x,normal.y,normal.z,tri);
        }else {
            // Native COL only describes the original ground surface. A wall
            // exposed below it exists in DigWalls; include that same boundary
            // query rather than rejecting a visible face as a raycast miss.
            var picked=HostClip.refine(from,to,net.minecraft.world.phys.BlockHitResult.miss(to,Direction.UP,BlockPos.containing(to)),HostClip.Use.PICK);
            hit=picked instanceof HostClip.HostHitResult nativeHit?nativeHit.hit:null;
        }
        // Terrain snapshots contain the original tree surface. Skip already cut wood
        // cells without opening the generic digWall/stone-interior path.
        for(int i=0;i<8&&hit!=null;i++){
            int[] c=nativeCell==null?HostRay.surfaceCell(hit):new int[]{nativeCell.getX(),nativeCell.getY(),nativeCell.getZ()};
            var pos=new BlockPos(c[0],c[1],c[2]);
            boolean cut=dev.libertycraft.GroveNativeTrees.wood(hit.tri().material)
                ?dev.libertycraft.GroveNativeTrees.cut(minecraft.level,HostDigClient.world(),pos)
                :!hit.tri().terrain&&dev.libertycraft.GroveArchitecture.cut(minecraft.level,HostDigClient.world(),pos);
            if(!cut)break;
            if(directWood>0||nativeCell!=null){diagnostic("already-cut",hit);return;}
            from=new Vec3(hit.x(),hit.y(),hit.z()).add(to.subtract(from).normalize().scale(.03));
            hit=HostClip.cast(from,to);
        }
        if (hit == null || new Vec3(hit.x(), hit.y(), hit.z()).distanceToSqr(target) > 1) {diagnostic("surface-mismatch",hit);return;}
        currentHit = hit;
        var previous = minecraft.hitResult;
        int[] cell = HostRay.placementCell(hit);
        minecraft.hitResult = new HostClip.HostHitResult(new Vec3(hit.x(), hit.y(), hit.z()),
                Direction.values()[HostRay.dominantFace(hit.nx(), hit.ny(), hit.nz())],
                new BlockPos(cell[0], cell[1], cell[2]), hit);
        boolean previousGround = minecraft.player.onGround();
        minecraft.player.setOnGround((HostClient.sky().flags & (1 << 17)) != 0);
        try {
            boolean mining = HostDigClient.attack(minecraft);
            diagnostic(mining?"mining":"attack-rejected",hit);
            Link.pushEvent(34, 0, HostDigClient.groveFraction(), mining ? 1 : 0, 0, 0, 0);
        } finally { minecraft.hitResult = previous; minecraft.player.setOnGround(previousGround); }
    }
}
