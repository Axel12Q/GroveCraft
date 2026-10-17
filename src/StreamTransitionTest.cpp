#include "StreamTransition.h"
#include <cassert>
#include <limits>
int main(){
    assert(grove::streamOpacity(100,100,100)==0);
    assert(grove::streamOpacity(100,700,100)==127);
    assert(grove::streamOpacity(100,1300,100)==255);
    assert(grove::streamOpacity(100,100,10)==255);
    assert(grove::streamOpacity(100,99,100)==255);
    assert(grove::streamOpacity(100,100,std::numeric_limits<float>::quiet_NaN())==255);
    unsigned previous=0;for(unsigned t=100;t<=1400;++t){auto value=grove::streamOpacity(100,t,100);assert(value>=previous);previous=value;}
}
