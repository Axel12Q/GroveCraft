#include "DeviceLease.h"
#include <cassert>
struct Device{unsigned refs=1,adds{},releases{};void AddRef(){++refs;++adds;}void Release(){assert(refs);--refs;++releases;}};
int main(){
    Device movie,game;
    {
        grove::DeviceLease<Device> lease;lease.reset(&movie);lease.reset(&movie);
        assert(movie.refs==2&&movie.adds==1);
        movie.Release();assert(movie.refs==1); // Movie owner leaves; cached pointer remains live.
        lease.reset(&game);assert(movie.refs==0&&game.refs==2);
        lease.reset();assert(game.refs==1);lease.reset();assert(game.releases==1);
        lease.reset(&game);
    }
    assert(game.refs==1&&game.adds==2&&game.releases==2);
}
