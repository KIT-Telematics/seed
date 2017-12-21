#include "seed-event.h"

using namespace std;

namespace seed
{
    SeedEvent::SeedEvent(EventType eventType, double startTime)
        : startTime (startTime),
        eventType (eventType)
    {}

    // SeedEvent::~SeedEvent()
    // {
    //
    // }

    double
        SeedEvent::getStartTime()
        {
            return this->startTime;
        }

    EventType
        SeedEvent::getType()
        {
            return this->eventType;
        }

    // SeedEvent::SeedEvent(const SeedEvent &obj) {
    //         cout << "Copy constructor allocating ptr." << endl;
    //     }
}
