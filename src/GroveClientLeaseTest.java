package dev.libertycraft;
import static org.junit.jupiter.api.Assertions.*;
import java.nio.file.*;
import java.util.concurrent.TimeUnit;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
public class GroveClientLeaseTest {
    @TempDir Path temp;
    @Test void secondProcessCannotTakeBridgeAndNormalExitReleasesLease() throws Exception {
        Path probe=temp.resolve("LeaseProbe.java");
        Files.writeString(probe,"import dev.libertycraft.GroveClientLease; class LeaseProbe { public static void main(String[] args) throws Exception { GroveClientLease.acquire(); System.out.println(\"owned\"); System.out.flush(); System.in.read(); } }");
        String java=Path.of(System.getProperty("java.home"),"bin","java.exe").toString();
        String cp=Path.of(GroveClientLease.class.getProtectionDomain().getCodeSource().getLocation().toURI()).toString();
        var command=new ProcessBuilder(java,"-cp",cp,probe.toString()).directory(temp.toFile()).redirectErrorStream(true);
        Process first=command.start();
        try {
            assertEquals("owned",first.inputReader().readLine());
            Process second=command.start();assertTrue(second.waitFor(15,TimeUnit.SECONDS));assertNotEquals(0,second.exitValue());
            first.getOutputStream().close();assertTrue(first.waitFor(10,TimeUnit.SECONDS));assertEquals(0,first.exitValue());
            Process third=command.start();assertEquals("owned",third.inputReader().readLine());
            third.getOutputStream().close();assertTrue(third.waitFor(10,TimeUnit.SECONDS));assertEquals(0,third.exitValue());
        } finally {first.getOutputStream().close();first.waitFor(10,TimeUnit.SECONDS);}
    }
}
