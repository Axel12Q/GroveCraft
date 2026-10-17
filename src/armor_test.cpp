#include "ArmorRules.h"
#include <cassert>
#include <limits>
int main(){
    assert(grove::armorDamage(40,0,0,0)==40);
    assert(std::abs(grove::armorDamage(40,15,0,0)-22.4f)<.001f);
    assert(grove::armorDamage(40,20,8,0)<grove::armorDamage(40,20,0,0));
    assert(grove::armorDamage(40,15,0,4)<grove::armorDamage(40,15,0,0));
    assert(grove::armorDamage(40,31,0,0)==40);
    assert(grove::armorDamage(40,std::numeric_limits<float>::quiet_NaN(),0,0)==40);
    assert(grove::armorWeapon(0)&&grove::armorWeapon(8)&&grove::armorWeapon(24));
    assert(!grove::armorWeapon(16)&&!grove::armorWeapon(37)&&!grove::armorWeapon(54));
}
