#ifndef SEED_APPLICATIONS_SEED_DROP_SERVER_H
#define SEED_APPLICATIONS_SEED_DROP_SERVER_H

#include "seed_base.h"
#include <unordered_map>
#include <unordered_set>

class INET_API SEEDDropServer : public SEEDBase
{
  public:
    int addListenerOnce();

    virtual void socketClosed(int connId, void *yourPtr) override;

  private:
    const static int startIndex = 5000;
    const static int endIndex = 6000;

    std::unordered_map<int, int> portMap;
    std::unordered_set<int> portSet;
    int portIndex = 0;

    int allocatePort();
    void deallocatePort(int port);
};

#endif
