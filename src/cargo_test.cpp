#include "CargoRules.h"
#include <cassert>
#include <limits>
int main(){
    using grove::cargoAccessible;
    assert(cargoAccessible(422,false,false,1000,0,1,0));
    assert(cargoAccessible(505,false,false,1000,0,1,0));
    assert(!cargoAccessible(481,false,false,1000,0,1,0));
    assert(!cargoAccessible(487,false,false,1000,0,1,0));
    assert(!cargoAccessible(422,true,false,1000,0,1,0));
    assert(!cargoAccessible(422,false,true,1000,0,1,0));
    assert(!cargoAccessible(422,false,false,200,0,1,0));
    assert(!cargoAccessible(422,false,false,1000,.03,1,0));
    assert(!cargoAccessible(422,false,false,1000,0,3,0));
    assert(!cargoAccessible(422,false,false,1000,0,1,3));
    assert(!cargoAccessible(422,false,false,1000,std::numeric_limits<float>::quiet_NaN(),1,0));
}
