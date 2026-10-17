package dev.libertycraft;

import java.util.*;

/** Stable plans in Minecraft coordinates (x, y, -GTA y). No game state or random tick order. */
public final class GroveEcologyRules {
    private GroveEcologyRules() {}
    public static boolean grassIsTop(double grass,double highest) {
        return Double.isFinite(grass)&&Double.isFinite(highest)&&highest-grass<=.002;
    }
    public enum Kind { OAK, BIRCH, CAMP, FARM, HAMLET, RUIN, OUTPOST, WAYSTATION }
    public record Site(String key, int x, int z, Kind kind, long seed) {}
    public record Cell(int x, int y, int z, String block) {}
    /** Supplemental grid: keep the original t: ledger and locations unchanged. */
    public static Site woodlandSite(long seed,int gx,int gz) {
        long h=mix(seed ^ (long)gx*0x632be59bd9b4e019L ^ (long)gz*0x9e3779b97f4a7c15L ^ 917);
        return new Site("w:"+gx+":"+gz,gx*16+4+Math.floorMod(h>>>8,8),gz*16+4+Math.floorMod(h>>>24,8),
            (h&3)==0?Kind.BIRCH:Kind.OAK,h);
    }
    /** Separate sparse grid preserves every previous tree/settlement identity. */
    public static Site expansionSite(long seed,int gx,int gz){
        long h=mix(seed^(long)gx*0x632be59bd9b4e019L^(long)gz*0x9e3779b97f4a7c15L^0x70696c6cL);
        return new Site("p:"+gx+":"+gz,gx*384+160+Math.floorMod(h>>>8,64),
            gz*384+160+Math.floorMod(h>>>24,64),(h&1)==0?Kind.OUTPOST:Kind.WAYSTATION,h);
    }
    /** Mountains and countryside, excluding the three urban footprints and protected hubs. */
    public static boolean woodland(int x,int z,int margin) {
        int y=-z;
        boolean mountain=inside(x,y,-2990,-2880,-1214,-1120,margin)
            || inside(x,y,-1214,-2880,40,-770,margin)
            || inside(x,y,40,-1000,2900,590,margin)
            || inside(x,y,-1200,590,1450,2800,margin)
            || inside(x,y,-2990,1665,-1200,2800,margin);
        if(!mountain)return countryside(x,z,margin);
        // Keep towns, farm compounds, bridges and mission areas reserved.
        int[][] reserved={{720,-540,250},{1300,310,280},{210,-100,250},{-790,-1500,200},
            {-1500,2600,250},{-1500,920,240},{-550,2600,240},{400,2500,500},{200,1900,450},
            {-950,1950,240},{-650,1450,220},{-1300,500,220},{-2190,-2300,260},{-1560,-2750,220}};
        for(int[] p:reserved)if(Math.hypot(x-p[0],y-p[1])<p[2]+margin)return false;
        // LS hillside urban fringe; LV's southern urban fringe; SF already outside rectangles.
        if(x>650&&y<-770 || x>1200&&y>500)return false;
        return true;
    }
    public static long mix(long v) {
        v = (v ^ (v >>> 30)) * 0xbf58476d1ce4e5b9L;
        v = (v ^ (v >>> 27)) * 0x94d049bb133111ebL;
        return v ^ (v >>> 31);
    }
    public static Site site(long seed, int gx, int gz, boolean tree) {
        long h = mix(seed ^ (long)gx * 0x632be59bd9b4e019L ^ (long)gz * 0x9e3779b97f4a7c15L ^ (tree ? 71 : 193));
        int grid = tree ? 32 : 192, jitter = tree ? 16 : 80;
        Kind k = tree ? ((h & 3) == 0 ? Kind.BIRCH : Kind.OAK) : switch (Math.floorMod(h, 8)) {
            case 0 -> Kind.HAMLET; case 1, 2 -> Kind.FARM; case 3, 4, 5 -> Kind.CAMP; default -> Kind.RUIN;
        };
        return new Site((tree ? "t:" : "s:") + gx + ":" + gz,
            gx * grid + grid / 2 + Math.floorMod(h >>> 8, jitter) - jitter / 2,
            gz * grid + grid / 2 + Math.floorMod(h >>> 24, jitter) - jitter / 2, k, h);
    }
    public static int radius(Kind k) { return switch(k) { case OAK,BIRCH -> 3; case HAMLET -> 10; case FARM,WAYSTATION -> 6; case OUTPOST -> 5; default -> 4; }; }
    public static int height(Kind k) { return k==Kind.OUTPOST?10:k == Kind.OAK || k == Kind.BIRCH ? 7 : 6; }
    public static boolean safe(int flags,int world,boolean inGame,boolean loading) {
        // Survey the countryside during travel/tutorial missions too. Placement
        // still requires native grass, free plots and the protected-area policy.
        return inGame&&!loading&&world==0&&(flags&((1<<19)|(1<<21)|(1<<1)|(1<<2)|(1<<5)))==0;
    }

    /** Conservative countryside reserves. Cities, airports and major rural mission hubs are excluded.
     * This is a placement policy, not a claim to identify every mission script's future route. */
    public static boolean countryside(int x, int z, int margin) {
        int gy = -z;
        boolean rural = inside(x,gy,-1850,-2150,-650,-1100,margin)
            || inside(x,gy,300,-768,1800,500,margin)
            || inside(x,gy,-1700,450,1500,2800,margin);
        if (!rural) return false;
        int[][] hubs = {{720,-540,250},{1300,310,280},{210,-100,250},{-790,-1500,200},
            {-1500,2600,250},{-1500,920,240},{-550,2600,240},{400,2500,500},{200,1900,450},
            {-950,1950,240},{-650,1450,220},{-1300,500,220}};
        for(int[] h:hubs) if(Math.hypot(x-h[0],gy-h[1]) < h[2]+margin) return false;
        // Footprints checked against the user's installed data/info.zon (rounded outward).
        int[][] protectedPlots={{967,-451,1177,-217},{-1210,-1318,-908,-787},
            {-377,826,124,1221},{-937,2611,-715,2848},{-465,2123,115,2581},
            {-969,1929,-481,2156},{-927,1398,-719,1635},{1546,208,1746,421},
            {-1120,1178,-862,1352},{-1795,-731,-947,579}};
        for(int[] plot:protectedPlots) if(x>=plot[0]-margin-40 && x<=plot[2]+margin+40
            && gy>=plot[1]-margin-40 && gy<=plot[3]+margin+40)return false;
        return true;
    }
    private static boolean inside(int x,int y,int a,int b,int c,int d,int m) {
        return x>=a+m && x<=c-m && y>=b+m && y<=d-m;
    }
    /** Air only above surveyed grass. Every plan is preflighted before the first write. */
    public static List<Cell> plan(Site site) {
        LinkedHashMap<String,Cell> out = new LinkedHashMap<>();
        switch(site.kind) {
            case OAK,BIRCH -> {
                String wood=site.kind==Kind.BIRCH?"birch":"oak";
                for(int y=0;y<5;y++) put(out,0,y,0,wood+"_log");
                for(int y=3;y<=5;y++) for(int x=-2;x<=2;x++) for(int z=-2;z<=2;z++)
                    if(!(Math.abs(x)==2&&Math.abs(z)==2) && !(x==0&&z==0&&y<5)) put(out,x,y,z,wood+"_leaves");
                put(out,0,6,0,wood+"_leaves");put(out,1,6,0,wood+"_leaves");put(out,-1,6,0,wood+"_leaves");
                put(out,0,6,1,wood+"_leaves");put(out,0,6,-1,wood+"_leaves");
            }
            case CAMP -> {
                floor(out,-3,-3,3,3,"oak_planks");
                for(int x:new int[]{-3,3}) for(int z:new int[]{-3,3}) for(int y=1;y<=3;y++) put(out,x,y,z,"oak_fence");
                for(int x=-3;x<=3;x++) for(int z=-3;z<=3;z++) put(out,x,4,z,"oak_slab");
                put(out,-2,1,0,"crafting_table");put(out,-2,1,1,"furnace");put(out,2,1,0,"barrel");
                put(out,0,1,2,"lantern");
                put(out,0,0,4,"oak_stairs");
            }
            case FARM -> farm(out,0,0);
            case HAMLET -> {
                house(out,-8,-4);house(out,3,-4);farm(out,0,6);
                for(int z=-4;z<=3;z++) put(out,0,0,z,"cobblestone");
                for(int x=-6;x<=5;x++) put(out,x,0,3,"cobblestone");
                put(out,0,1,-3,"lantern");put(out,-1,1,-3,"composter");
                put(out,-1,0,-3,"cobblestone");
                put(out,4,1,-2,"smithing_table"); // East house is the workshop; west keeps its furnace.
            }
            case OUTPOST -> {
                for(int y:new int[]{0,4,8})floor(out,-3,-3,3,3,"dark_oak_planks");
                for(int x:new int[]{-3,3})for(int z:new int[]{-3,3})for(int y=1;y<=8;y++)put(out,x,y,z,"dark_oak_log");
                for(int x=-2;x<=2;x++)for(int y=1;y<=3;y++){
                    put(out,x,y,-3,y==2&&Math.abs(x)==1?"glass_pane":"cobblestone");
                    if(x!=0||y==3)put(out,x,y,3,"cobblestone");
                }
                for(int z=-2;z<=2;z++)for(int y=1;y<=3;y++)for(int x:new int[]{-3,3})put(out,x,y,z,"cobblestone");
                for(int x=-3;x<=3;x++)for(int z=-3;z<=3;z++)if(Math.abs(x)==3||Math.abs(z)==3)put(out,x,9,z,"dark_oak_fence");
                for(int step=0;step<7;step++){
                    int z=3-step;for(int y=0;y<=step;y++)put(out,4,y,z,"dark_oak_log");
                    put(out,4,step+1,z,"stairs_north");
                }
                for(int y=0;y<=7;y++)put(out,3,y,-4,"dark_oak_log");
                put(out,3,8,-4,"stairs_south");
                out.remove("3:9:-3"); // Stair entrance onto the lookout.
                put(out,0,0,4,"dark_oak_planks");
                put(out,-2,1,-2,"barrel");put(out,2,5,-2,"barrel");
                put(out,0,5,0,"lantern");
                put(out,0,9,-3,"black_banner");
            }
            case WAYSTATION -> {
                floor(out,-3,-3,3,3,"cobblestone");
                // A villager approaching its workstation needs a half-block first step.
                put(out,-2,0,-3,"cobble_stairs_south");put(out,-1,0,-3,"cobble_stairs_south");
                for(int x:new int[]{-3,3})for(int z:new int[]{-3,3})for(int y=1;y<=3;y++)put(out,x,y,z,"oak_log");
                for(int x=-3;x<=3;x++)for(int z=-3;z<=3;z++)put(out,x,4,z,"oak_slab");
                put(out,-1,0,4,"smithing_table");put(out,-2,1,-1,"furnace");
                put(out,2,1,-2,"barrel");put(out,2,1,0,"barrel");
                put(out,0,1,-3,"lantern");put(out,-2,1,2,"bed_foot");put(out,-2,1,3,"bed_head");
                put(out,0,0,4,"oak_stairs");
                put(out,0,-1,5,"oak_stairs");put(out,0,-2,6,"oak_stairs");
            }
            case RUIN -> {
                floor(out,-3,-3,3,3,"mossy_cobblestone");
                for(int x=-3;x<=3;x++) for(int y=1;y<=1+Math.floorMod(mix(site.seed+x),3);y++) put(out,x,y,-3,"mossy_cobblestone");
                for(int z=-2;z<=3;z++) put(out,-3,1,z,"cobblestone");
                put(out,2,1,2,"barrel");put(out,-2,1,-2,"furnace");
            }
        }
        return List.copyOf(out.values());
    }
    private static void house(Map<String,Cell> o,int x,int z) {
        floor(o,x,z,x+5,z+5,"oak_planks");
        for(int dx=0;dx<=5;dx++) for(int dz=0;dz<=5;dz++) {
            for(int y=1;y<=3;y++) if(dx==0||dx==5||dz==0||dz==5) {
                if(dz==5&&dx==2&&y<=2) continue; // unobstructed doorway
                boolean corner=(dx==0||dx==5)&&(dz==0||dz==5);
                boolean window=y==2&&((dx==0||dx==5)&&dz==2||dz==0&&(dx==2||dx==3));
                put(o,x+dx,y,z+dz,corner?"oak_log":window?"glass_pane":"oak_planks");
            }
            int rise=Math.min(dx,5-dx);
            put(o,x+dx,4+rise,z+dz,rise==2?"oak_slab":dx<3?"stairs_east":"stairs_west");
            if(dz==0||dz==5)for(int y=4;y<4+rise;y++)put(o,x+dx,y,z+dz,"oak_planks");
        }
        put(o,x+1,1,z+1,"crafting_table");put(o,x+1,1,z+2,"furnace");put(o,x+4,1,z+1,"barrel");
        put(o,x+4,1,z+4,"lantern");put(o,x+2,1,z+1,"bed_foot");put(o,x+2,1,z+2,"bed_head");
        put(o,x+2,0,z+6,"oak_stairs");
    }
    private static void farm(Map<String,Cell> o,int x,int z) {
        // Raised, sealed irrigation bed; never edits or digs the native terrain.
        for(int dx=-3;dx<=3;dx++) for(int dz=-2;dz<=2;dz++) {
            put(o,x+dx,0,z+dz,"dirt");
            boolean rim=Math.abs(dx)==3||Math.abs(dz)==2;
            put(o,x+dx,1,z+dz,rim?"oak_log":dx==0?"water":"farmland");
            if(!rim&&dx!=0) put(o,x+dx,2,z+dz,"wheat");
        }
        put(o,x+4,0,z,"oak_planks");put(o,x+4,1,z,"barrel");put(o,x-4,1,z,"composter");
        put(o,x-4,0,z,"oak_planks");
        put(o,x+3,2,z+2,"lantern");
        put(o,x+5,0,z+1,"stairs_west");put(o,x+4,0,z+1,"oak_planks");put(o,x+4,1,z+1,"stairs_west");
    }
    private static void floor(Map<String,Cell> o,int x0,int z0,int x1,int z1,String block) {
        for(int x=x0;x<=x1;x++) for(int z=z0;z<=z1;z++) put(o,x,0,z,block);
    }
    private static void put(Map<String,Cell> o,int x,int y,int z,String b) { o.put(x+":"+y+":"+z,new Cell(x,y,z,b)); }
}
