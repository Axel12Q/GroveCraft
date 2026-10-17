package dev.libertycraft;

import dev.libertycraft.world.HostTri;
import java.util.*;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;

/** Exact projected footprint of a native facade within one removed cell.
 * A thin native shell has no neighbouring volume to provide a rear cap. */
public final class GroveArchitectureCap {
    private GroveArchitectureCap(){}
    public static List<double[]> footprint(HostTri t,BlockPos p,Direction entry){
        int axis=entry.getAxis()==Direction.Axis.X?0:entry.getAxis()==Direction.Axis.Y?1:2;
        double normal=axis==0?t.nx:axis==1?t.ny:t.nz;
        if(t.terrain||!t.diggable||Math.abs(normal)<.7)return List.of();
        double[] origin={p.getX(),p.getY(),p.getZ()};
        double[] min={t.minX,t.minY,t.minZ},max={t.maxX,t.maxY,t.maxZ};
        if(max[axis]<origin[axis]-.005||min[axis]>origin[axis]+1.005)return List.of();
        int a=(axis+1)%3,b=(axis+2)%3;
        var solid=surface(t,p);
        List<double[]> poly=new ArrayList<>();
        for(var v:solid)poly.add(new double[]{v[a],v[b]});
        if(poly.size()<3)return List.of();
        double area=0;for(int i=0;i<poly.size();i++){
            var v=poly.get(i);var w=poly.get((i+1)%poly.size());area+=v[0]*w[1]-w[0]*v[1];
        }
        return Math.abs(area)<1e-8?List.of():poly;
    }
    public static List<double[]> surface(HostTri t,BlockPos p){
        double[] origin={p.getX(),p.getY(),p.getZ()};
        double[][] vertices={{t.ax,t.ay,t.az},{t.bx,t.by,t.bz},{t.cx,t.cy,t.cz}};
        List<double[]> solid=new ArrayList<>();
        for(var v:vertices)solid.add(new double[]{v[0]-origin[0],v[1]-origin[1],v[2]-origin[2]});
        for(int dim=0;dim<3;dim++){
            solid=clip(solid,dim,0,true);solid=clip(solid,dim,1,false);
        }
        // The analytic result is inside the unit cell. Floating-point plane
        // intersections can leave -1e-16 at a boundary; the host correctly
        // rejects negative chamber bounds, so canonicalize before publication.
        for(var point:solid)for(int dim=0;dim<3;dim++)point[dim]=Math.clamp(point[dim],0,1);
        return solid;
    }
    private static List<double[]> clip(List<double[]> in,int dim,double edge,boolean greater){
        List<double[]> out=new ArrayList<>();if(in.isEmpty())return out;
        var last=in.getLast();boolean was=greater?last[dim]>=edge:last[dim]<=edge;
        for(var current:in){
            boolean inside=greater?current[dim]>=edge:current[dim]<=edge;
            if(inside!=was){double f=(edge-last[dim])/(current[dim]-last[dim]);
                double[] point=new double[last.length];
                for(int i=0;i<point.length;i++)point[i]=last[i]+f*(current[i]-last[i]);
                point[dim]=edge;
                out.add(point);}
            if(inside)out.add(current);last=current;was=inside;
        }
        return out;
    }
}
