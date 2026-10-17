#pragma once
#include <cmath>
namespace grove {
// First slice: common road cars and utility trucks only; bikes/aircraft/boats excluded.
inline bool cargoRoadModel(int model) {
    switch(model) {
    case 400:case 401:case 402:case 404:case 405:case 410:case 411:case 412:
    case 418:case 419:case 421:case 422:case 426:case 436:case 439:case 445:
    case 451:case 455:case 456:case 458:case 466:case 467:case 474:case 475:
    case 478:case 479:case 480:case 482:case 489:case 491:case 492:case 496:
    case 499:case 500:case 505:case 507:case 516:case 517:case 518:case 526:case 527:
    case 529:case 533:case 534:case 535:case 536:case 540:case 542:case 543:
    case 545:case 546:case 547:case 549:case 550:case 551:case 554:case 555:
    case 558:case 559:case 560:case 561:case 562:case 566:case 575:case 576:
    case 579:case 580:case 585:case 587:case 589:case 600:case 602:case 603:
        return true;
    default:return false;
    }
}
inline bool cargoAccessible(int model,bool driver,bool mission,float health,float speed,float rearDistance,float heightDifference) {
    return cargoRoadModel(model)&&!driver&&!mission&&std::isfinite(health)&&health>250
        &&std::isfinite(speed)&&speed>=0&&speed<.02f&&std::isfinite(rearDistance)
        &&rearDistance>=0&&rearDistance<2.8f&&std::isfinite(heightDifference)&&std::abs(heightDifference)<2;
}
}
