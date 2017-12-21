#ifndef EVENT_BULK_TRAFFIC_H
#define EVENT_BULK_TRAFFIC_H

#include <string>
#include <map>
#include "seed-event-types.h"
#include "seed-event.h"

using namespace std;

namespace seed
{
    class SeedBulkTrafficEvent : public SeedEvent
    {
        public:
            SeedBulkTrafficEvent(EventType eventType, double startTime,
                    string source,
                    string destination,
                    long size
                );
            string getSource();
            string getDestination();
            long getSize();

        protected:
            string source;
            string destination;
            long size;
    };
}
#endif //EVENT_BULK_TRAFFIC_H
