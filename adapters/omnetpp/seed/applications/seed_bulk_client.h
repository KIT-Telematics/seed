#ifndef SEED_APPLICATIONS_SEED_BULK_CLIENT_H
#define SEED_APPLICATIONS_SEED_BULK_CLIENT_H

#include "seed_base.h"
#include <unordered_map>

class INET_API SEEDBulkClient : public SEEDBase
{
  public:
    void send(const char* address, int port, int64 size);

    virtual void socketEstablished(int connId, void *ptr) override;
    virtual void socketClosed(int connId, void *yourPtr) override;

  private:
    std::unordered_map<int, int64> sizes;
};

#endif
