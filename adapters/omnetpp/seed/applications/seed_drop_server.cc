#include "seed_drop_server.h"

Define_Module(SEEDDropServer);

int SEEDDropServer::addListenerOnce()
{
  Enter_Method("SEEDDropClient::addListenerOnce()");
  int port = allocatePort();
  int connId = listenOnce(port);
  return port;
}

void SEEDDropServer::socketClosed(int connId, void *ptr)
{
  deallocatePort(socketMapConn.find(connId)->second->getLocalPort());
  SEEDBase::socketClosed(connId, ptr);
}

int SEEDDropServer::allocatePort()
{
  if (portSet.size() == endIndex - startIndex)
    throw cRuntimeError("Port range exhausted");;
  for (; !portSet.insert(startIndex + portIndex).second;
    portIndex = (portIndex + 1) % (endIndex - startIndex));
  return startIndex + portIndex;
}

void SEEDDropServer::deallocatePort(int port)
{
  auto it = portSet.find(port);
  if (it != portSet.end()) portSet.erase(it);
}
