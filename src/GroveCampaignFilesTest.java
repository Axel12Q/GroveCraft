package dev.libertycraft;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import java.nio.file.*;
import static org.junit.jupiter.api.Assertions.*;
class GroveCampaignFilesTest {
    @TempDir Path directory;
    @Test void snapshotRestoresExactResourcesAndNeverOverwritesAWorld()throws Exception{
        var live=directory.resolve("live");Files.createDirectories(live.resolve("players/data"));
        Files.writeString(live.resolve("level.dat"),"saved-level");Files.writeString(live.resolve("players/data/p.dat"),"logs32 wheat32");
        Files.writeString(live.resolve("session.lock"),"live-owner");var snapshot=directory.resolve("snapshot");
        GroveCampaignFiles.snapshot(live,snapshot);Files.writeString(live.resolve("players/data/p.dat"),"logs64 after-save");
        var restored=directory.resolve("restored");GroveCampaignFiles.restore(snapshot,restored);
        assertEquals("logs32 wheat32",Files.readString(restored.resolve("players/data/p.dat")));assertFalse(Files.exists(restored.resolve("session.lock")));
        assertThrows(Exception.class,()->GroveCampaignFiles.restore(snapshot,restored));
        assertEquals("logs64 after-save",Files.readString(live.resolve("players/data/p.dat")));
    }
    @Test void corruptionAndExtraFilesCannotSilentlyLoad()throws Exception{
        var live=directory.resolve("live");Files.createDirectories(live);Files.writeString(live.resolve("level.dat"),"original");
        var snapshot=directory.resolve("snapshot");GroveCampaignFiles.snapshot(live,snapshot);
        Files.writeString(snapshot.resolve("world/level.dat"),"corrupt");assertThrows(Exception.class,()->GroveCampaignFiles.verify(snapshot));
        Files.writeString(snapshot.resolve("world/level.dat"),"original");GroveCampaignFiles.verify(snapshot);
        Files.writeString(snapshot.resolve("world/unexpected.dat"),"extra");assertThrows(Exception.class,()->GroveCampaignFiles.verify(snapshot));
    }
    @Test void identityAndPathsCannotEscapeStorage(){
        assertThrows(Exception.class,()->GroveCampaignFiles.id("../another-world"));assertThrows(Exception.class,()->GroveCampaignFiles.child(directory,"../outside"));
        String a="0123456789abcdef0123456789abcdef",b="11111111111111111111111111111111";
        assertNotEquals(GroveCampaignFiles.world(a,a),GroveCampaignFiles.world(a,b));assertNotEquals(GroveCampaignFiles.world(a,a),GroveCampaignFiles.world(b,a));
    }
}
