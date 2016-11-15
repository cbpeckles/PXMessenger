#ifndef PXMCLIENT_H
#define PXMCLIENT_H

#include <QWidget>
#include <QUuid>
#include <QDebug>

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include <event2/event.h>

#include "pxmdefinitions.h"

#ifdef __unix__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

class PXMClient : public QObject
{
    Q_OBJECT
public:
    PXMClient();
    /*!
     * \brief setlocalUUID
     *
     * Inform this class of the uuid for our host, retrieved from the ini file.
     * Should not change.  OBSOLETE
     * \param uuid
     */
    void setlocalUUID(QString uuid);
public slots:
    /*!
     * \brief sendMsg
     *
     * Sends a message to the already connected socket.  This function must be
     * called after connectToPeer.  The format for any tcp packet begins with
     * a 4 byte non-inclusive length of the packet.  After this, all packets
     * contain the uuid of the sender.  Then a variable length type character is
     * inserted.  Type always begins with a "/".  After the type, the actual
     * message is appended to the end of the packet.  The sending of data is
     * done by the recursiveSend() function to ensure partial sends are resent.
     * \param socketfd Socket descriptor to send on
     * \param msg message to send.
     * \param type type of packet to send.  Current types are "/msg", "/uuid"
     * 				"/request", "/ip".  See PXMServer for more details.
     * \param uuid Our UUID to send out.
     * \param theiruuid The recipients uuid used for our own recording purposes
     * \return emits a resultsOfTCPSend()
     * \see man send
     */
    void sendMsg(evutil_socket_t socketfd, const char *msg, const char *type, const char *uuid, const char *theiruuid);
    /*!
     * \brief sendMsgSlot
     *
     * Slot for calling sendMsg.  QString to char* converstion
     * \param s socket to send on
     * \param msg message to send
     * \param type packet type to send as
     * \param uuid our UUID
     * \param theiruuid recipient UUID, only used to report results, not
     * 			included in packet
     * \see sendMsg()
     */
    void sendMsgSlot(evutil_socket_t s, QString msg, QString type, QUuid uuid, QString theiruuid);
    /*!
     * \brief sendUDP
     *
     * Sends a udp packet to the multicast group defined by MULTICAST_ADDRESS.
     * Important:These messages are currently send twice to improves odds that
     * one of them gets their.
     * \param msg Message to send
     * \param port port to send to in the multicast group
     */
    void sendUDP(const char *msg, unsigned short port);
    /*!
     * \brief connectToPeer
     *
     * Establish connection to peer.
     * \param socketfd Socket to connect on
     * \param ipaddr IP address to connect to
     * \param service port number to connect to
     * \return emits resultOfConnectionAttempt() with the socket number and a
     * 			bool variable containing the result.
     * \see man connect
     */
    int connectToPeer(evutil_socket_t socketfd, sockaddr_in socketAddr);
private:


    int recursiveSend(evutil_socket_t socketfd, const char *msg, int len, int count);
signals:
    void resultOfConnectionAttempt(evutil_socket_t, bool);
    void resultOfTCPSend(evutil_socket_t, QString, QString, bool);
};

#endif // PXMCLIENT_H
