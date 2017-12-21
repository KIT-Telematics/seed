#ifndef SEED_APPLICATIONS_SEED_ONOFF_CLIENT_H
#define SEED_APPLICATIONS_SEED_ONOFF_CLIENT_H

#include "seed_base.h"
#include "seed_onoff_client_message_m.h"
#include <functional>
#include <unordered_map>

struct SEEDOnOffClientContext
{
  std::function<double(double)> onTime;
  std::function<double(double)> offTime;
  double interPacket;
  int64 maxSize;
  int64 packetSize;
  bool on;
  OnOffMessage *onOffMessage;
  SendPacketMessage *sendPacketMessage;
};

class INET_API SEEDOnOffClient : public SEEDBase
{
  public:
    void send(
      const char* address,
      int port,
      std::function<double(double)> onTime,
      std::function<double(double)> offTime,
      double dataRate,
      int64 maxSize,
      int64 packetSize
    );

    virtual void handleTimer(cMessage *msg) override;
    virtual void socketEstablished(int connId, void *ptr) override;
    virtual void socketClosed(int connId, void *yourPtr) override;

  private:
    std::unordered_map<int, SEEDOnOffClientContext> context;
};

#endif
