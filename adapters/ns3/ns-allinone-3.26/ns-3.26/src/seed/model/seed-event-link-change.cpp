#include <map>
#include "seed-event-link-change.h"

using namespace std;

namespace seed
{
    SeedLinkChangeEvent::SeedLinkChangeEvent(EventType eventType, double startTime,
            string link,
            bool state)
        : SeedEvent(eventType, startTime),
        link (link),
        state (state)
    {}

    string
        SeedLinkChangeEvent::getLink()
        {
            return this->link;
        }

    bool
        SeedLinkChangeEvent::getState()
        {
            return this->state;
        }
}
