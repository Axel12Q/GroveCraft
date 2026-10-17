#pragma once
namespace grove {
// SA's door/carjack/enter/exit/arrest states follow its three death states.
// A numeric "state >= DIE" incorrectly treats entering a car as a new life.
inline bool dyingState(int state){return state>=54&&state<=56;}
}
