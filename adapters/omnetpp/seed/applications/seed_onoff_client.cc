#include "seed_onoff_client.h"

Define_Module(SEEDOnOffClient);

void SEEDOnOffClient::send(
  const char* address,
  int port,
  std::function<double(double)> onTime,
  std::function<double(double)> offTime,
  double dataRate,
  int64 maxSize,
  int64 packetSize
)
{
  Enter_Method("SEEDOnOffClient::send(%s, %i)", address, port);
  int connId = connect(address, port);

  SEEDOnOffClientContext sEEDOnOffClientContext;
  sEEDOnOffClientContext.onTime = onTime;
  sEEDOnOffClientContext.offTime = offTime;
  sEEDOnOffClientContext.interPacket = packetSize/dataRate;
  sEEDOnOffClientContext.maxSize = maxSize/8;
  sEEDOnOffClientContext.packetSize = packetSize/8;
  sEEDOnOffClientContext.on = false;
  sEEDOnOffClientContext.onOffMessage = new OnOffMessage();
  sEEDOnOffClientContext.onOffMessage->setConnId(connId);
  sEEDOnOffClientContext.sendPacketMessage = new SendPacketMessage();
  sEEDOnOffClientContext.sendPacketMessage->setConnId(connId);

  context.emplace(connId, sEEDOnOffClientContext);
}

void SEEDOnOffClient::handleTimer(cMessage *msg)
{
  if (dynamic_cast<OnOffMessage *>(msg) != NULL)
  {
    OnOffMessage *oOM = (OnOffMessage *) msg;
    int connId = oOM->getConnId();
    auto it = context.find(connId);
    it->second.on = !it->second.on;
    if (it->second.on)
    {
      scheduleAt(simTime() + it->second.onTime(dblrand()), oOM);
      scheduleAt(simTime(), it->second.sendPacketMessage);
    }
    else
    {
      scheduleAt(simTime() + it->second.offTime(dblrand()), oOM);
      cancelEvent(it->second.sendPacketMessage);
    }
  }
  else if (dynamic_cast<SendPacketMessage *>(msg) != NULL)
  {
    SendPacketMessage *spM = (SendPacketMessage *) msg;
    int connId = spM->getConnId();
    auto it = context.find(connId);
    it->second.maxSize -= it->second.packetSize;
    if (it->second.maxSize < 0)
    {
      cancelEvent(it->second.onOffMessage);
      close(connId);
    }
    else
    {
      GenericAppMsg *msg = new GenericAppMsg("data");
      msg->setByteLength(it->second.packetSize);
      sendPacket(connId, msg);
      scheduleAt(simTime() + it->second.interPacket, spM);
    }
  }
}

void SEEDOnOffClient::socketEstablished(int connId, void *ptr)
{
  SEEDBase::socketEstablished(connId, ptr);
  scheduleAt(simTime(), context.find(connId)->second.onOffMessage);
}

void SEEDOnOffClient::socketClosed(int connId, void *ptr)
{
  auto it = context.find(connId);
  delete it->second.onOffMessage;
  delete it->second.sendPacketMessage;
  context.erase(it);
  SEEDBase::socketClosed(connId, ptr);
}
