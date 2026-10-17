package dev.libertycraft;

import dev.libertycraft.link.Link;
import dev.libertycraft.link.Proto;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.minecraft.core.BlockPos;
import net.minecraft.world.level.block.Blocks;

/** Opt-in acceptance fixture in the isolated development world, never enabled in normal play. */
public final class GroveAcceptance {
    private static int stage, wait;
    private static BlockPos base;
    private static int bread;
    private static BlockPos mine;
    private static int mineTicks;
    public static void register() {
        if (!Boolean.getBoolean("grovecraft.acceptance")) return;
        ServerTickEvents.END_SERVER_TICK.register(server -> {
            if (stage == 3 || server.getPlayerList().getPlayers().isEmpty()) return;
            Link.SkyState state = new Link.SkyState();
            if (!Link.active() || !Link.readSkyState(state) || (state.flags & Proto.SKY_IN_GAME) == 0) return;
            var player = server.getPlayerList().getPlayers().getFirst();
            if (player.position().distanceToSqr(new net.minecraft.world.phys.Vec3(state.x, state.y, state.z)) > 4) return;
            var level = player.level();
            try {
                if (stage == 0) {
                    if (++wait < 60) return; // wait for client/server movement and inventory synchronization
                    base = BlockPos.containing(state.x + 3, state.y, state.z);
                    level.setBlockAndUpdate(base, Blocks.AIR.defaultBlockState());
                    level.setBlockAndUpdate(base.above(), Blocks.AIR.defaultBlockState());
                    GroveGameplay.act(player, 6, 0, 0, 0);
                    // Acceptance has its own world. Explicit fixture supplies make repeat runs independent.
                    player.getInventory().setItem(0, new net.minecraft.world.item.ItemStack(net.minecraft.world.item.Items.OAK_PLANKS, 16));
                    player.getInventory().setItem(2, new net.minecraft.world.item.ItemStack(net.minecraft.world.item.Items.DIRT, 16));
                    player.getInventory().setItem(5, new net.minecraft.world.item.ItemStack(net.minecraft.world.item.Items.WOODEN_HOE));
                    player.getInventory().setItem(6, new net.minecraft.world.item.ItemStack(net.minecraft.world.item.Items.WHEAT_SEEDS, 16));
                    player.getInventory().setItem(7, new net.minecraft.world.item.ItemStack(net.minecraft.world.item.Items.BREAD, 16));
                    GroveGameplay.act(player, 102, 0, 0, 0);
                    int count = player.getMainHandItem().getCount();
                    GroveGameplay.act(player, 1, base.getX(), base.getY(), base.getZ());
                    require(level.getBlockState(base).is(Blocks.DIRT), "actual dirt placement");
                    require(player.getMainHandItem().getCount() == count - 1, "survival resource consumption");
                    GroveGameplay.act(player, 105, 0, 0, 0);
                    GroveGameplay.act(player, 3, base.getX(), base.getY(), base.getZ());
                    require(level.getBlockState(base).is(Blocks.FARMLAND), "vanilla hoe creates farmland");
                    stage = 4; wait = 0;
                } else if (stage == 4 && ++wait > 30) {
                    LibertyCraft.LOG.info("[GroveCraft] Crop light {} soil {}", level.getRawBrightness(base.above(), 0), level.getBlockState(base));
                    GroveGameplay.act(player, 106, 0, 0, 0);
                    GroveGameplay.act(player, 3, base.getX(), base.getY(), base.getZ());
                    require(level.getBlockState(base.above()).is(Blocks.WHEAT), "vanilla wheat seeds plant crop");
                    var other = base.south(2);
                    level.setBlockAndUpdate(other, Blocks.AIR.defaultBlockState());
                    level.setBlockAndUpdate(other.west(), Blocks.AIR.defaultBlockState());
                    level.setBlockAndUpdate(other.above(), Blocks.AIR.defaultBlockState());
                    for (var item : level.getEntitiesOfClass(net.minecraft.world.entity.item.ItemEntity.class,
                            new net.minecraft.world.phys.AABB(other).inflate(1))) item.discard();
                    GroveGameplay.act(player, 100, 0, 0, 0);
                    GroveGameplay.act(player, 1, other.getX(), other.getY(), other.getZ());
                    require(level.getBlockState(other).is(Blocks.OAK_PLANKS), "plank placement");
                    int beforeSide = player.getMainHandItem().getCount();
                    GroveGameplay.act(player, 204, other.getX(), other.getY(), other.getZ());
                    require(level.getBlockState(other.west()).is(Blocks.OAK_PLANKS), "west-face placement");
                    require(player.getMainHandItem().getCount() == beforeSide - 1, "side placement consumes item");
                    mine = other;
                    level.setBlockAndUpdate(mine, Blocks.STONE.defaultBlockState());
                    player.getInventory().setItem(0, net.minecraft.world.item.ItemStack.EMPTY);
                    GroveGameplay.act(player, 2, mine.getX(), mine.getY(), mine.getZ());
                    require(!level.getBlockState(mine).isAir(), "mining is not instant");
                    stage = 5; wait = 0;
                } else if (stage == 5) {
                    if (++wait < 10) GroveGameplay.act(player, 2, mine.getX(), mine.getY(), mine.getZ());
                    else {
                        GroveGameplay.act(player, 9, 0, 0, 0);
                        require(!level.getBlockState(mine).isAir(), "interrupted mining preserves block");
                        require(GroveGameplay.miningFraction() == 0, "abort resets cracks and progress");
                        stage = 6; wait = 0;
                    }
                } else if (stage == 6) {
                    GroveGameplay.act(player, 2, mine.getX(), mine.getY(), mine.getZ());
                    ++wait;
                    if (level.getBlockState(mine).isAir()) {
                        mineTicks = wait;
                        require(wait >= 150, "vanilla bare-hand stone mining time");
                        require(level.getEntitiesOfClass(net.minecraft.world.entity.item.ItemEntity.class,
                                new net.minecraft.world.phys.AABB(mine).inflate(1)).stream()
                                .noneMatch(item -> item.getItem().is(net.minecraft.world.item.Items.COBBLESTONE)), "wrong tool gives no stone drop");
                        stage = 7; wait = 0;
                    } else require(wait < 800, "bare-hand mining eventually completes");
                } else if (stage == 7 && ++wait > 10) {
                    level.setBlockAndUpdate(mine, Blocks.STONE.defaultBlockState());
                    player.getInventory().setItem(0, new net.minecraft.world.item.ItemStack(net.minecraft.world.item.Items.WOODEN_PICKAXE));
                    stage = 8; wait = 0;
                } else if (stage == 8) {
                    GroveGameplay.act(player, 2, mine.getX(), mine.getY(), mine.getZ());
                    ++wait;
                    if (level.getBlockState(mine).isAir()) {
                        require(wait > 1 && wait < mineTicks, "correct pickaxe is faster, still timed");
                        require(player.getMainHandItem().getDamageValue() == 1, "vanilla tool durability");
                        require(level.getEntitiesOfClass(net.minecraft.world.entity.item.ItemEntity.class,
                                new net.minecraft.world.phys.AABB(mine).inflate(1)).stream()
                                .anyMatch(item -> item.getItem().is(net.minecraft.world.item.Items.COBBLESTONE)), "real collectible cobblestone drop");
                        stage = 9; wait = 0;
                    } else require(wait < 200, "pickaxe mining eventually completes");
                } else if (stage == 9 && ++wait > 8) {
                    // Explicit test fixture: make the player hungry, then let actual item-use ticks eat bread.
                    player.getFoodData().setFoodLevel(12);
                    GroveGameplay.act(player, 107, 0, 0, 0);
                    bread = player.getMainHandItem().getCount();
                    java.nio.file.Files.writeString(net.fabricmc.loader.api.FabricLoader.getInstance().getGameDir()
                            .resolve("grovecraft-eat-request"), "send native input type 32 action 4");
                    stage = 1; wait = 0;
                } else if (stage == 1 && ++wait > 80) {
                    require(player.getFoodData().getFoodLevel() > 12, "vanilla food replenishes hunger");
                    require(player.getMainHandItem().getCount() == bread - 1, "one meal consumes exactly one item");
                    player.getFoodData().setFoodLevel(20);
                    bread = player.getMainHandItem().getCount();
                    java.nio.file.Files.writeString(net.fabricmc.loader.api.FabricLoader.getInstance().getGameDir()
                            .resolve("grovecraft-eat-request"), "full meal");
                    stage = 10; wait = 0;
                } else if (stage == 10 && ++wait > 80) {
                    require(player.getFoodData().getFoodLevel() == 20, "overeating keeps hunger capped");
                    require(player.getMainHandItem().getCount() == bread - 1, "full-hunger meal consumes exactly one item");
                    player.getFoodData().setFoodLevel(6);
                    stage = 11; wait = 0;
                } else if (stage == 11 && ++wait > 25) {
                    player.getFoodData().setFoodLevel(20);
                    GroveGameplay.act(player, 5, 0, 0, 0);
                    var path = net.fabricmc.loader.api.FabricLoader.getInstance().getGameDir().resolve("grovecraft-acceptance.json");
                    java.nio.file.Files.writeString(path, "{\"passed\":true,\"fixture_host\":true,\"real_minecraft\":true,\"placement\":true,\"resource_consumption\":true,\"farming\":true,\"timed_mining\":true,\"mining_abort\":true,\"wrong_tool_no_drop\":true,\"pickaxe_drop_and_durability\":true,\"eating\":true,\"overeating\":true,\"one_item_per_meal\":true,\"save_requested\":true}");
                    LibertyCraft.LOG.info("[GroveCraft] Acceptance passed in real Minecraft against a fixture host");
                    stage = 3;
                }
            } catch (Exception error) {
                stage = 3;
                LibertyCraft.LOG.error("[GroveCraft] Acceptance failed", error);
                try { java.nio.file.Files.writeString(net.fabricmc.loader.api.FabricLoader.getInstance().getGameDir()
                        .resolve("grovecraft-acceptance.json"), "{\"passed\":false,\"fixture_host\":true}"); } catch (Exception ignored) {}
            }
        });
    }
    private static void require(boolean condition, String message) {
        if (!condition) throw new IllegalStateException(message);
    }
}
