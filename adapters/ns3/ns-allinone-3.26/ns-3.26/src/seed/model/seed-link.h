#ifndef LINK_H
#define LINK_H value
#include <string>
#include <map>
#include "seed-node-types.h"

using namespace std;

namespace seed
{
    class SeedLink
    {
        public:
            SeedLink(string name, string groups, string source,
                    string destination, string bandwidth,
                    string delay, double errorRate, string state);
            string getName();
            string getGroups();
            string getSource();
            string getDestination();
            string getBandwidth();
            string getDelay();
            double getErrorRate();
            string getState();


        protected:
            string name;
            string groups;
            string source;
            string destination;
            string bandwidth;
            string delay;
            double errorRate;
            string state;

    };
}
#endif /* ifndef LINK_H */
