package dev.libertycraft;
import java.nio.file.*;
import java.util.Map;
/** Shared installer settings work even when the official launcher was already
 * running and cannot inherit a new shell's environment variables. */
public final class GroveReleasePaths {
    private GroveReleasePaths(){}
    public static Path home(Map<String,String> env){
        String local=env.get("LOCALAPPDATA");if(local==null||local.isBlank())return null;
        var base=Path.of(local);if(!base.isAbsolute())return null;
        return base.resolve("GroveCraft");
    }
    public static boolean installed(Path home){
        if(home==null)return false;
        try{return Files.readAllLines(home.resolve("installed-v1.txt")).getFirst().equals("GroveCraftRelease1");}
        catch(Exception e){return false;}
    }
    public static Path installedHome(){var home=home(System.getenv());return installed(home)?home:null;}
    public static String campaignRoot(){
        String explicit=System.getenv("GROVECRAFT_CAMPAIGN_ROOT");
        if(explicit!=null&&!explicit.isBlank())return explicit;
        if(System.getProperty("grovecraft.testWorld")!=null)return null;
        var home=installedHome();return home==null?null:home.resolve("campaigns").toString();
    }
    public static String surfaceAtlas(){
        String explicit=System.getProperty("grovecraft.surfaceAtlas");if(explicit!=null)return explicit;
        var home=installedHome();return home!=null&&Files.isRegularFile(home.resolve("surface.bin"))?home.resolve("surface.bin").toString():null;
    }
}
