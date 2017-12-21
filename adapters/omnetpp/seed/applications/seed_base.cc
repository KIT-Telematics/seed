#include "seed_base.h"

#include "IPvXAddressResolver.h"

void SEEDBase::initialize(int stage)
{
    cSimpleModule::initialize(stage);
}

void SEEDBase::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        handleTimer(msg);
    else
      socketMap.findSocketFor(msg)->processMessage(msg);
}

int SEEDBase::createSocket(const char *connectAddress, int localPort)
{
  auto socket = new TCPSocket();

  socketMap.addSocket(socket);
  socketMapConn.emplace(socket->getConnectionId(), socket);
  //socket.readDataTransferModePar(*this);
  socket->bind(*connectAddress ?
    IPvXAddressResolver().resolve(connectAddress) : IPvXAddress(), localPort);

  socket->setCallbackObject(this);
  socket->setOutputGate(gate("tcpOut"));
  socket->setDataTransferMode(TCP_TRANSFER_BYTECOUNT);

  return socket->getConnectionId();
}

int SEEDBase::connect(const char *connectAddress, int connectPort)
{
    int connId = createSocket("", -1);
    TCPSocket *socket = socketMapConn.find(connId)->second;

    IPvXAddress destination;
    IPvXAddressResolver().tryResolve(connectAddress, destination);
    if (destination.isUnspecified())
        EV << "cannot resolve destination address: " << connectAddress << endl;
    else
        socket->connect(destination, connectPort);

    return connId;
}

int SEEDBase::listenOnce(int connectPort)
{
    int connId = createSocket("", connectPort);
    TCPSocket *socket = socketMapConn.find(connId)->second;
    socket->listenOnce();

    return connId;
}

void SEEDBase::close(int connId)
{
    auto it = socketMapConn.find(connId);
    if (it == socketMapConn.end()) return;
    it->second->close();
}

void SEEDBase::sendPacket(int connId, cPacket *msg)
{
    socketMapConn.find(connId)->second->send(msg);
}

void SEEDBase::handleTimer(cMessage *)
{ }

void SEEDBase::socketEstablished(int connId, void *)
{
    socketStart.emplace(connId, simTime());
}

void SEEDBase::socketDataArrived(int, void *, cPacket *msg, bool)
{
    delete msg;
}

void SEEDBase::socketPeerClosed(int connId, void *)
{
    auto start_it = socketStart.find(connId);
    recordScalar("fct", simTime() - start_it->second);
    socketStart.erase(start_it);
    if (socketMapConn.find(connId)->second->getState() ==
      TCPSocket::PEER_CLOSED)
        close(connId);
}

void SEEDBase::socketClosed(int connId, void *)
{
    auto it = socketMapConn.find(connId);
    delete it->second;
    socketMap.removeSocket(it->second);
    socketMapConn.erase(it);
}

void SEEDBase::socketFailure(int, void *, int code)
{ }

void SEEDBase::socketStatusArrived(int, void *, TCPStatusInfo *status)
{
  delete status;
}
