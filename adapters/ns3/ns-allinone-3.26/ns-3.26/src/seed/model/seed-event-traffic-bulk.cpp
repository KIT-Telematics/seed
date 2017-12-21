#include <string>
#include <map>
#include "seed-event-traffic-bulk.h"
#include "seed-event.h"

using namespace std;

namespace seed
{
    SeedBulkTrafficEvent::SeedBulkTrafficEvent(EventType eventType, double startTime,
            string source,
            string destination,
            long size)
        : SeedEvent(eventType, startTime),
        source (source),
        destination (destination),
        size (size)
    {}

    string
        SeedBulkTrafficEvent::getSource()
        {
            // return string ("foo");
            return this->source;
        }

    string
        SeedBulkTrafficEvent::getDestination()
        {
            return destination;
        }

    long
        SeedBulkTrafficEvent::getSize()
        {
            return size;
        }
}
