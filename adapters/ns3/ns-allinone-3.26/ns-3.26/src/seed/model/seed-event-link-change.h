#ifndef EVENT_LINK_CHANGE_H
#define EVENT_LINK_CHANGE_H

#include <string>
#include <map>
#include "seed-event-types.h"
#include "seed-event.h"

using namespace std;

namespace seed
{
    class SeedLinkChangeEvent : public SeedEvent
    {
        public:
            SeedLinkChangeEvent(EventType eventType, double startTime,
                    string link,
                    bool state
                    );
            string getLink();
            bool getState();

        protected:
            string link;
            bool state;
    };
}


#endif //EVENT_LINK_CHANGE
