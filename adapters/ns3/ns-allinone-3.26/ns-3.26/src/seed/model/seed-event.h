#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <map>
#include "seed-event-types.h"

using namespace std;

namespace seed
{
    class SeedEvent
    {
        public:
            SeedEvent(EventType eventType, double startTime);
            // SeedEvent(const SeedEvent &obj);
            // SeedEvent(const SeedEvent&) = delete;
            // UserQueues& operator=(const UserQueues&) = delete;
            virtual ~SeedEvent() = default;
                // virtual ~SeedEvent();
            virtual double getStartTime();
            virtual EventType getType();

        protected:
            double startTime;
            EventType eventType;

    };
}
#endif //EVENT_H
