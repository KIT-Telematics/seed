#include "seed_bulk_client.h"

Define_Module(SEEDBulkClient);

void SEEDBulkClient::send(const char* address, int port, int64 size)
{
  Enter_Method("SEEDBulkClient::send(%s, %i, %i)", address, port, size);
  int connId = connect(address, port);
  sizes.emplace(connId, size);
}

void SEEDBulkClient::socketEstablished(int connId, void *ptr)
{
  SEEDBase::socketEstablished(connId, ptr);
  GenericAppMsg *msg = new GenericAppMsg("data");
  msg->setByteLength(sizes.find(connId)->second/8);
  sendPacket(connId, msg);
  close(connId);
}

void SEEDBulkClient::socketClosed(int connId, void *ptr)
{
  sizes.erase(connId);
  SEEDBase::socketClosed(connId, ptr);
}
