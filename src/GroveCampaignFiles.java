package dev.libertycraft;
import java.nio.file.*;
import java.nio.channels.FileChannel;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.*;
import java.io.IOException;
/** Immutable, checked world snapshots. Only new private destinations are copied. */
public final class GroveCampaignFiles {
    private GroveCampaignFiles(){}
    public static String id(String s){if(s==null||!s.matches("[0-9a-f]{32}"))throw new IllegalArgumentException("Invalid campaign identity");return s;}
    public static String world(String campaign,String work){return "GroveCraft_"+id(campaign)+"_"+id(work).substring(0,16);}
    public static Path child(Path root,String name){var base=root.toAbsolutePath().normalize();var p=base.resolve(name).normalize();
        if(!p.startsWith(base)||p.equals(base))throw new IllegalArgumentException("Path escapes paired storage");return p;}
    public static void atomic(Path path,String text)throws IOException{
        Files.createDirectories(path.getParent());var temp=path.resolveSibling(path.getFileName()+".tmp");
        byte[] data=text.getBytes(StandardCharsets.UTF_8);
        try(var out=FileChannel.open(temp,StandardOpenOption.CREATE,StandardOpenOption.TRUNCATE_EXISTING,StandardOpenOption.WRITE)){
            var buffer=java.nio.ByteBuffer.wrap(data);while(buffer.hasRemaining())out.write(buffer);out.force(true);}
        for(int attempt=0;;attempt++){
            try{Files.move(temp,path,StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);break;}
            catch(AccessDeniedException e){if(attempt>=39)throw e;try{Thread.sleep(5);}catch(InterruptedException interrupted){Thread.currentThread().interrupt();throw new IOException(interrupted);}}
        }
    }
    private static String hash(Path p)throws Exception{var digest=MessageDigest.getInstance("SHA-256");try(var in=Files.newInputStream(p)){
        byte[] data=new byte[65536];for(int n;(n=in.read(data))!=-1;)digest.update(data,0,n);}return HexFormat.of().formatHex(digest.digest());}
    private static List<Path> files(Path root)throws IOException{try(var walk=Files.walk(root)){
        var all=walk.sorted().toList();for(var p:all)if(Files.isSymbolicLink(p))throw new IOException("Snapshot refuses symlinks: "+p);
        return all.stream().filter(Files::isRegularFile).filter(p->!p.getFileName().toString().equals("session.lock")).toList();}}
    public static void copy(Path source,Path target)throws IOException{
        if(Files.exists(target))throw new IOException("Preserve existing world/snapshot: "+target);
        var items=files(source);Files.createDirectories(target);
        for(var p:items){var dest=child(target,source.relativize(p).toString());Files.createDirectories(dest.getParent());Files.copy(p,dest);}
    }
    public static void snapshot(Path source,Path directory)throws Exception{
        if(!Files.isRegularFile(source.resolve("level.dat")))throw new IOException("World is not saved yet");
        var target=directory.resolve("world");copy(source,target);StringBuilder list=new StringBuilder();
        for(var p:files(target))list.append(hash(p)).append(' ').append(Base64.getEncoder().encodeToString(target.relativize(p).toString().getBytes(StandardCharsets.UTF_8))).append('\n');
        atomic(directory.resolve("files.sha256"),list.toString());verify(directory);
    }
    public static void verify(Path directory)throws Exception{
        var world=directory.resolve("world");var seen=new HashSet<Path>();
        for(var line:Files.readAllLines(directory.resolve("files.sha256"))){var pieces=line.split(" ",2);
            if(pieces.length!=2||!pieces[0].matches("[0-9a-f]{64}"))throw new IOException("Invalid snapshot manifest");
            String name=new String(Base64.getDecoder().decode(pieces[1]),StandardCharsets.UTF_8);var p=child(world,name);
            if(!seen.add(p)||Files.isSymbolicLink(p)||!Files.isRegularFile(p)||!hash(p).equals(pieces[0]))throw new IOException("Snapshot failed verification: "+name);
        }
        if(!seen.contains(child(world,"level.dat"))||!seen.equals(new HashSet<>(files(world).stream().map(p->p.toAbsolutePath().normalize()).toList())))
            throw new IOException("Incomplete snapshot manifest");
    }
    public static void restore(Path directory,Path destination)throws Exception{verify(directory);copy(directory.resolve("world"),destination);}
}
