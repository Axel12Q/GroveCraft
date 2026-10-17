package dev.libertycraft;
import java.nio.*;
import java.nio.channels.FileChannel;
import java.nio.file.*;
/** Read-only private map cache prepared outside the games from installed COL/IPL. */
public final class GroveSurfaceAtlas {
    private final FloatBuffer ground,top;private final int x0,z0,step,size;
    private final String identity;
    public String identity(){return identity;}
    private GroveSurfaceAtlas(int x0,int z0,int step,int size,FloatBuffer ground,FloatBuffer top,String identity){
        this.x0=x0;this.z0=z0;this.step=step;this.size=size;this.ground=ground;this.top=top;
        this.identity=identity;
    }
    public static GroveSurfaceAtlas open(Path file)throws java.io.IOException{
        try(var ch=FileChannel.open(file,StandardOpenOption.READ)){
            if(ch.size()<24)throw new java.io.IOException("Truncated surface cache");
            var bytes=ch.map(FileChannel.MapMode.READ_ONLY,0,ch.size()).order(ByteOrder.LITTLE_ENDIAN);
            if(bytes.getInt()!=0x31415347||bytes.getInt()!=1)throw new java.io.IOException("Unsupported surface cache");
            int x=bytes.getInt(),z=bytes.getInt(),step=bytes.getInt(),n=bytes.getInt();
            if(step<1||step>8||n<1||n>6000||ch.size()!=24L+8L*n*n)throw new java.io.IOException("Invalid surface cache dimensions");
            var ground=bytes.slice(24,n*n*4).order(ByteOrder.LITTLE_ENDIAN).asFloatBuffer();
            var top=bytes.slice(24+n*n*4,n*n*4).order(ByteOrder.LITTLE_ENDIAN).asFloatBuffer();
            String identity;
            try{var hash=java.security.MessageDigest.getInstance("SHA-256");hash.update(bytes.duplicate().clear());identity=java.util.HexFormat.of().formatHex(hash.digest());}
            catch(java.security.NoSuchAlgorithmException e){throw new java.io.IOException(e);}
            return new GroveSurfaceAtlas(x,z,step,n,ground,top,identity);
        }
    }
    private double sample(FloatBuffer values,int x,int z){
        double xx=(x-x0)/(double)step,zz=(z-z0)/(double)step;int ix=(int)Math.floor(xx),iz=(int)Math.floor(zz);
        if(ix<0||iz<0||ix>=size-1||iz>=size-1)return Double.NaN;
        double u=xx-ix,v=zz-iz,sum=0;
        for(int dx=0;dx<2;dx++)for(int dz=0;dz<2;dz++){
            double w=(dx==0?1-u:u)*(dz==0?1-v:v);if(w==0)continue;
            float h=values.get((iz+dz)*size+ix+dx);if(!Float.isFinite(h))return Double.NaN;sum+=h*w;
        }
        return sum;
    }
    public double ground(int x,int z){return sample(ground,x,z);}
    public boolean known(int x,int z){return Double.isFinite(sample(top,x,z));}
    public boolean occupied(int x,int y,int z){
        double h=sample(top,x,z);return !Double.isFinite(h)||y<=Math.floor(h);
    }
}
