package dev.libertycraft;
import dev.libertycraft.link.Link;
import dev.libertycraft.world.HostDig;
import dev.libertycraft.world.city.BlockyCity;
import net.minecraft.core.BlockPos;
import net.minecraft.world.level.Level;
import net.minecraft.world.phys.shapes.*;
/** Temporary grass-floor support until the detailed native region arrives.
 * Does not fabricate buildings or fill mined cells. Detailed collision wins. */
public final class GroveStaticGround {
    private GroveStaticGround(){}
    public static boolean available(){return GroveEcology.surfaceAtlas()!=null&&Link.active();}
    public static boolean dug(BlockPos pos){
        var level=GroveEcology.atlasLevel();if(level==null||!level.hasChunkAt(pos)||!Link.active())return false;
        var sky=new Link.SkyState();return Link.readSkyState(sky)&&HostDig.isDug(level,sky.worldId,pos);
    }
    private static double height(Level level,BlockPos pos){
        if(!available()||!BlockyCity.isMirror(level)||!level.hasChunkAt(pos))return Double.NaN;
        var sky=new Link.SkyState();if(!Link.readSkyState(sky)||sky.worldId!=0||sky.loading())return Double.NaN;
        if(HostDig.isDug(level,0,pos))return Double.NaN;
        double h=GroveEcology.surfaceAtlas().ground(pos.getX(),pos.getZ());
        return Double.isFinite(h)&&Math.abs(h-pos.getY())<=6?h:Double.NaN;
    }
    public static VoxelShape shape(Level level,BlockPos pos){
        double h=height(level,pos);if(!Double.isFinite(h)||h<=pos.getY())return null;
        return Shapes.box(0,0,0,1,Math.min(1,h-pos.getY()),1);
    }
    public static boolean supported(Level level,BlockPos feet){
        double h=height(level,feet);if(!Double.isFinite(h)||h>feet.getY()+1||h<feet.getY()-5)return false;
        for(int y=feet.getY();y>=Math.floor(h);y--)if(HostDig.isDug(level,0,new BlockPos(feet.getX(),y,feet.getZ())))return false;
        return true;
    }
    public static float top(BlockPos pos){
        var level=GroveEcology.atlasLevel();if(level==null)return 0;
        double h=height(level,pos);return Double.isFinite(h)?(float)Math.clamp(h-pos.getY(),0,1):0;
    }
}
