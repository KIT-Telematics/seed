#include <string>
#include <map>
#include "seed-node-types.h"
#include "seed-link.h"

using namespace std;

namespace seed
{
    SeedLink::SeedLink(string name, string groups, string source,
            string destination, string bandwidth,
            string delay, double errorRate, string state)
        : name (name),
        groups (groups),
        source (source),
        destination (destination),
        bandwidth (bandwidth),
        delay (delay),
        errorRate (errorRate),
        state (state)
    {}

    string
        SeedLink::getName()
        {
            return name;
        }


    string
        SeedLink::getGroups()
        {
            return groups;
        }


    string
        SeedLink::getSource()
        {
            return source;
        }

    string
        SeedLink::getDestination()
        {
            return destination;
        }

    string
        SeedLink::getBandwidth()
        {
            return bandwidth;
        }

    string
        SeedLink::getDelay()
        {
            return delay;
        }

    double
        SeedLink::getErrorRate()
        {
            return errorRate;
        }

    string
        SeedLink::getState()
        {
            return state;
        }
}
