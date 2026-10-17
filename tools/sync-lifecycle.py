"""Owned lifecycle/seat adaptation; shared header padding is reserved by GroveCraft."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
guest = root / 'build/guest'
p = guest / 'src/main/java/dev/libertycraft/link/Link.java'
s = p.read_text(encoding='utf-8')
if 'public static boolean hostSuspended()' not in s:
    s = s.replace('public static boolean hostAlive() {', '''public static boolean hostSuspended() {
        MemorySegment s = shm;
        if (s == null) return false;
        long presented = (long) LONG.getAcquire(s, 0x28L);
        long elapsed = tickCount() - presented;
        return (int) INT.getAcquire(s, 0x20L) != 0 || presented == 0 || elapsed < 0 || elapsed > 1000;
    }

    public static boolean hostAlive() {''')
    s = s.replace('out.seq = seq1;', 'out.seq = seq1;\n                if (hostSuspended()) out.flags |= 1 << 1;')
    p.write_text(s, encoding='utf-8')
if 'GroveClientLease.acquire();' not in s:
    s=s.replace('PLATFORM.announce();','dev.libertycraft.GroveClientLease.acquire();\n        PLATFORM.announce();')
    p.write_text(s,encoding='utf-8')
# The shared bridge also has a process claim, including clients from other folders.
p = guest / 'src/main/java/dev/libertycraft/link/Link.java'
s = p.read_text(encoding='utf-8')
if 'private static void groveClaimGuest' not in s:
    s = s.replace('public static synchronized void announceRunning() {', """private static void groveClaimGuest(MemorySegment s) {
        for (int attempt=0; attempt<8; attempt++) {
            int owner=(int) INT.getAcquire(s, OFF_HEADER + H_MC_PID);
            if(owner==MC_PID)return;
            if(owner!=0 && ProcessHandle.of(owner).map(ProcessHandle::isAlive).orElse(false))
                throw new IllegalStateException("Another Minecraft process already owns GroveCraft's bridge");
            if(INT.compareAndSet(s, OFF_HEADER + H_MC_PID, owner, MC_PID))return;
        }
        throw new IllegalStateException("GroveCraft guest claim contended");
    }

    public static synchronized void announceRunning() {""")
    s=s.replace('s.set(JAVA_INT, OFF_HEADER + H_MC_PID, MC_PID);','groveClaimGuest(s);')
    s=s.replace('mapped.set(JAVA_INT, OFF_HEADER + H_MC_PID, MC_PID);','groveClaimGuest(mapped);')
    p.write_text(s,encoding='utf-8')
p = guest / 'src/client/java/dev/libertycraft/client/GtaMenuPause.java'
s = (root / 'vendor/libertycraft/fabric/src/client/java/dev/libertycraft/client/GtaMenuPause.java').read_text(encoding='utf-8')
s = s.replace('boolean want = menu || scene;', 'boolean want = menu || scene || (Link.hostPid() != 0 && (!Link.hostAlive() || Link.hostSuspended()));')
p.write_text(s, encoding='utf-8')
p = guest / 'src/main/java/dev/libertycraft/world/HostDrive.java'
s = p.read_text(encoding='utf-8')
s = s.replace('private static volatile MountType mountType = MountType.BOAT;', 'private static volatile MountType mountType = MountType.NONE;')
s = s.replace('MountType type = (flags & FLAG_IN_VEHICLE) != 0 ? mountType : MountType.NONE;', 'MountType type = MountType.NONE; // GTA owns the actual seat; no temporary Minecraft entity.')
s = s.replace('mountType = MountType.parse(value, MountType.BOAT);', 'mountType = MountType.NONE; // Keep native vehicles authoritative, including legacy config files.')
p.write_text(s, encoding='utf-8')
print('GroveCraft: paired pause, fresh-frame fence and native seats without ghost boats.')
