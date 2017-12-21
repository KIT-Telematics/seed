#ifndef SEED_APPLICATIONS_SEED_BASE_H
#define SEED_APPLICATIONS_SEED_BASE_H

#include "GenericAppMsg_m.h"
#include "TCPSocket.h"
#include "TCPSocketMap.h"
#include <unordered_map>

class INET_API SEEDBase : public cSimpleModule, public TCPSocket::CallbackInterface
{
  protected:
    TCPSocketMap socketMap;
    std::unordered_map<int, TCPSocket*> socketMapConn;

    /**
     * Initialization. Should be redefined to perform or schedule a connect().
     */
    virtual void initialize(int stage);

    /**
     * For self-messages it invokes handleTimer(); messages arriving from TCP
     * will get dispatched to the socketXXX() functions.
     */
    virtual void handleMessage(cMessage *msg);

    /** @name Utility functions */
    //@{
    /** Issues an active OPEN to the address/port given as module parameters */
    virtual int connect(const char *connectAddress, int connectPort);

    virtual int listenOnce(int connectPort);

    /** Issues CLOSE command */
    virtual void close(int connId);

    /** Sends the given packet */
    virtual void sendPacket(int connId, cPacket *pkt);

    /** Invoked from handleMessage(). Should be redefined to handle self-messages. */
    virtual void handleTimer(cMessage *msg);

    /** @name TCPSocket::CallbackInterface callback methods */
    //@{
    /** Does nothing but update statistics/status. Redefine to perform or schedule first sending. */
    virtual void socketEstablished(int connId, void *yourPtr);

    /**
     * Does nothing but update statistics/status. Redefine to perform or schedule next sending.
     * Beware: this function deletes the incoming message, which might not be what you want.
     */
    virtual void socketDataArrived(int connId, void *yourPtr, cPacket *msg, bool urgent);

    /** Since remote TCP closed, invokes close(). Redefine if you want to do something else. */
    virtual void socketPeerClosed(int connId, void *yourPtr);

    /** Does nothing but update statistics/status. Redefine if you want to do something else, such as opening a new connection. */
    virtual void socketClosed(int connId, void *yourPtr);

    /** Does nothing but update statistics/status. Redefine if you want to try reconnecting after a delay. */
    virtual void socketFailure(int connId, void *yourPtr, int code);

    /** Redefine to handle incoming TCPStatusInfo. */
    virtual void socketStatusArrived(int connId, void *yourPtr, TCPStatusInfo *status);
    //@}
  private:
    int createSocket(const char *connectAddress, int localPort);

    std::unordered_map<int, simtime_t> socketStart;
};

#endif
