package dev.libertycraft;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import java.nio.file.*;
import java.util.Map;
import static org.junit.jupiter.api.Assertions.*;
class GroveReleasePathsTest {
    @TempDir Path temp;
    @Test void configurationRequiresExplicitInstallerMarker()throws Exception{
        assertFalse(GroveReleasePaths.installed(temp));
        Files.writeString(temp.resolve("installed-v1.txt"),"wrong\n");assertFalse(GroveReleasePaths.installed(temp));
        Files.writeString(temp.resolve("installed-v1.txt"),"GroveCraftRelease1\n");assertTrue(GroveReleasePaths.installed(temp));
    }
    @Test void localDirectoryMustBeAbsolute(){
        assertNull(GroveReleasePaths.home(Map.of()));assertNull(GroveReleasePaths.home(Map.of("LOCALAPPDATA","relative")));
        assertEquals(temp.resolve("GroveCraft"),GroveReleasePaths.home(Map.of("LOCALAPPDATA",temp.toString())));
    }
}
