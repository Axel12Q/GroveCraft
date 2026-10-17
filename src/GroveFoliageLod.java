package dev.libertycraft;
import java.util.BitSet;
import java.util.function.IntUnaryOperator;

/** Conservative visual LOD: merge only fully occupied, identical 2x2x2 leaf
 * cells. Never expands a silhouette into empty space or alters collision. */
public final class GroveFoliageLod {
    private GroveFoliageLod(){}
    public static boolean coarse(double distance,boolean previous){
        return Double.isFinite(distance)&&distance>(previous?96:128);
    }
    public static int index(int x,int y,int z){return x+16*z+256*y;}
    public static BitSet merged(IntUnaryOperator material){
        var mask=new BitSet(4096);
        for(int y=0;y<16;y+=2)for(int z=0;z<16;z+=2)for(int x=0;x<16;x+=2){
            int id=material.applyAsInt(index(x,y,z));if(id==0)continue;
            boolean full=true;
            for(int dy=0;dy<2&&full;dy++)for(int dz=0;dz<2&&full;dz++)for(int dx=0;dx<2;dx++)
                if(material.applyAsInt(index(x+dx,y+dy,z+dz))!=id){full=false;break;}
            if(full)for(int dy=0;dy<2;dy++)for(int dz=0;dz<2;dz++)for(int dx=0;dx<2;dx++)mask.set(index(x+dx,y+dy,z+dz));
        }
        return mask;
    }
}
