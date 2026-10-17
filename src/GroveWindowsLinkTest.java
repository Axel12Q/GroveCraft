package dev.libertycraft.link;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import static org.junit.jupiter.api.Assumptions.*;

/** Runs only against the separate Python synthetic host; no game is launched. */
public class GroveWindowsLinkTest {
    @Test
    void exchangesStateWithWindowsHost() {
        assumeTrue(Boolean.getBoolean("grovecraft.integration"));
        Link.poll();
        assertTrue(Link.active(), "Native Windows shared memory handshake");
        Link.SkyState sky = new Link.SkyState();
        assertTrue(Link.readSkyState(sky));
        assertEquals(2495.0, sky.x);
        assertEquals(13.0, sky.y);
        assertEquals(1687.0, sky.z);
        assertEquals(2, sky.seq);
        Link.McState state = new Link.McState();
        state.x = sky.x + 1;
        state.y = sky.y;
        state.z = sky.z;
        state.frameCounter = 42;
        Link.writeMcState(state);
    }
}
