#ifndef SEED_EVENT_TYPES_H
#define SEED_EVENT_TYPES_H

namespace seed
{
    enum class EventType
    {
        TRAFFIC_BULK,
        TRAFFIC_ON_OFF,
        LINK_CHANGE_STATE,
        UNDEFINED
    };

    // string eventMap[]
    // {
    //     "bulk-event",
    //     "traffic-on-off-event",
    //     "link-state-change-event",
    //     "undefined"
    // }
    //
    // int s2iSeedEvent(string eventName)
    // {
    //     int i = 0;
    //     for(auto const &it : eventMap)
    //     {
    //         if(it == "eventName")
    //         {
    //             return i;
    //         }
    //         i++;
    //     }
    //     return -1;
    // }
}

#endif /* ifndef SYMBOL */
