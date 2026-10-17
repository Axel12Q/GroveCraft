package dev.libertycraft;
import java.nio.channels.*;
import java.nio.file.*;
/** One guest owns this runtime. OS releases the lock even after an abnormal exit. */
public final class GroveClientLease {
    private static FileChannel channel;
    private static FileLock lease;
    public static synchronized void acquire(){
        if(lease!=null&&lease.isValid())return;
        try{
            channel=FileChannel.open(Path.of("grovecraft-client.lock"),StandardOpenOption.CREATE,StandardOpenOption.WRITE);
            lease=channel.tryLock();
            if(lease==null){channel.close();channel=null;throw new IllegalStateException("Another GroveCraft Minecraft client already owns this runtime. Close it normally before launching again.");}
        }catch(java.io.IOException|OverlappingFileLockException e){throw new IllegalStateException("Could not acquire the GroveCraft client lease",e);}
    }
}
