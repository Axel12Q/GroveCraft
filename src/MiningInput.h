#pragma once
namespace grove {
// Empty Minecraft slots must not steal an equipped GTA weapon's mouse fire.
constexpr bool ownsMiningInput(int count, bool nativeUnarmed, bool explicitMining) {
    return count > 0 || nativeUnarmed || explicitMining;
}
}
