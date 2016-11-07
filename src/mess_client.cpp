#include <mess_client.h>

MessengerClient::MessengerClient()
{

}
void MessengerClient::setLocalHostname(char *hostname)
{
    localHostname = hostname;
}
void MessengerClient::setlocalUUID(QString uuid)
{
    localUUID = uuid;
}
void MessengerClient::udpSendSlot(QString msg)
{
    this->udpSend(msg.toStdString().c_str());
}
void MessengerClient::udpSend(const char* msg)
{
    int len;
    int port2 = 13649;
    struct sockaddr_in broadaddr;
    //struct in_addr localInterface;
    int socketfd2;

    memset(&broadaddr, 0, sizeof(broadaddr));
    broadaddr.sin_family = AF_INET;
    broadaddr.sin_addr.s_addr = inet_addr("239.192.13.13");
    broadaddr.sin_port = htons(port2);

    if ( (socketfd2 = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
        perror("socket:");

    len = strlen(msg);

    for(int i = 0; i < 1; i++)
    {
        sendto(socketfd2, msg, len+1, 0, (struct sockaddr *)&broadaddr, sizeof(broadaddr));
    }
#ifdef _WIN32
    closesocket(socketfd2);
#else
    close(socketfd2);
#endif
}
void MessengerClient::connectToPeerSlot(int s, QString ipaddr, QString service)
{
    this->c_connect(s, ipaddr.toStdString().c_str(), service.toStdString().c_str());
}
/**
 * @brief 			This function connects a socket to a specific ip address.
 * @param socketfd	socket to connect on
 * @param ipaddr	ip address to connect socket to
 * @return			-1 on failure to connect, socket descriptor on success
 */
int MessengerClient::c_connect(int socketfd, const char *ipaddr, const char *service)
{
    int status;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(ipaddr, service, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", (char*)gai_strerror(status));
        return 2;
    }

    if( (status = ::connect(socketfd, res->ai_addr, res->ai_addrlen)) < 0 )
    {
        std::cout << strerror(errno) << std::endl;
        freeaddrinfo(res);
        emit resultOfConnectionAttempt(socketfd, status);
#ifdef _WIN32
        closesocket(socketfd);
#else
        close(socketfd);
#endif
        return 1;
    }
    qDebug() << "Successfully connected to" << ipaddr << "on port" << service << "on socket" << socketfd;
    emit resultOfConnectionAttempt(socketfd, status);
    freeaddrinfo(res);
    return 0;
}
/**
 *	@brief			This will send a message to the socket, should check beforehand to make sure its connected.
 * 					The size of the final message is sent in the first three characters
 *  @param socketfd the socket descriptor to send to, does not check if is connected or not
 *  @param msg 		message to send
 *  @param host 	hostname of current computer to display before msg
 *  @param type		type of message to be sending.  Valid types are /msg /hostname /request
 *   				/namerequest /ip
 *  @return 		number of bytes that were sent, should be equal to strlen(full_mess).
 *   				-5 if socket is not connected
 */
int MessengerClient::send_msg(int socketfd, const char *msg, const char *host, const char *type, const char *uuid, const char *theiruuid)
{
    int packetLen, bytes_sent, sendcount = 0;
    char msgLen[3];
    bool print = false;

    //Combine strings into final message (host): (msg)\0

    if(!strcmp(type, "/msg") )
    {
        packetLen = strlen(uuid) + strlen(type) + strlen(host) + strlen(msg) + 2;
        print = true;
    }
    else if(!strcmp(type,"/global"))
    {
        packetLen = strlen(uuid) + strlen(type) + strlen(host) + strlen(msg) + 2;
    }
    else if(!strcmp(type,"/uuid"))
    {
        packetLen = strlen(uuid) + strlen(type) + strlen(host);
    }
    else
    {
        packetLen = strlen(uuid) + strlen(type) + strlen(msg);
    }

    sprintf(msgLen, "%03d", packetLen);
    int packetLenLength = 3;
    //account for the numbers we just added to the front of the final message
    packetLen += packetLenLength;

    char full_mess[packetLen] = {};
    char humanReadableMessage[packetLen-strlen(type)] = {};
    strncpy(full_mess, msgLen, 3);
    strcat(full_mess, uuid);

    strcat(full_mess, type);
    if(!strcmp(type, "/uuid"))
    {
        strcat(full_mess, host);
    }
    if(!strcmp(type, "/msg") || !strcmp(type,"/global"))
    {
        strcat(full_mess, host);
        strcat(humanReadableMessage, host);
        full_mess[strlen(host) + strlen(type) + strlen(uuid) + packetLenLength] = ':';
        full_mess[strlen(host) + strlen(type) + strlen(uuid) + packetLenLength + 1] = ' ';
        humanReadableMessage[strlen(host)] = ':';
        humanReadableMessage[strlen(host) + 1] = ' ';
    }
    strcat(full_mess, msg);
    strcat(humanReadableMessage, msg);

    bytes_sent = this->partialSend(socketfd, full_mess, packetLen, sendcount);

    if(bytes_sent > 0)
    {
        if(bytes_sent >= packetLen)
        {
            emit resultOfTCPSend(0, QString::fromUtf8(theiruuid), QString::fromUtf8(humanReadableMessage), print);
            return bytes_sent;
        }
        else
        {
            std::cout << "Partial Send has failed not all bytes sent" << std::endl;
            emit resultOfTCPSend(bytes_sent, QString::fromUtf8(theiruuid), QString::fromUtf8(humanReadableMessage), print);
            return bytes_sent;
        }
    }
    else
    {
        emit resultOfTCPSend(-1, QString::fromUtf8(theiruuid), QString::fromUtf8(humanReadableMessage), print);
#ifdef _WIN32
        return -5;
#else
        switch (errno)
        {
        case EPIPE:
            return -5;
        }
#endif

    }
    return -5;
}
void MessengerClient::sendMsgSlot(int s, QString msg, QString host, QString type, QUuid uuid, QString theiruuid)
{
    this->send_msg(s, msg.toStdString().c_str(), host.toStdString().c_str(), type.toStdString().c_str(), uuid.toString().toStdString().c_str(), theiruuid.toStdString().c_str());
}
/**
 * @brief 			Recursively sends all data in case the kernel fails to do so in one pass
 * @param socketfd	Socket to send to
 * @param msg		final formatted message to send
 * @param len		length of message to send
 * @param count		Only attempt to resend 5 times so as not to hang the program if something goes wrong
 * @return 			-1 on error, total bytes sent otherwise
 */
int MessengerClient::partialSend(int socketfd, const char *msg, int len, int count)
{
    int status2 = 0;
#ifdef _WIN32
    int status = send(socketfd, msg, len, 0);
#else
    int status = send(socketfd, msg, len, MSG_NOSIGNAL);
#endif

    if( (status <= 0) )
    {
        perror("send:");
        return -1;
    }

    if( ( status != len ) && ( count < 10 ) )
    {
        int len2 = len - status;
        char msg2[len2];
        strncpy(msg2, &msg[status], len2);
        count++;

        status2 = partialSend(socketfd, msg2, len2, count);
        if(status2 <= 0)
            return -1;
    }
    return status + status2;
}
/**
 * @brief 			Slot function for signal called from mess_serv class.  Sends hostname to socket
 * @param s			Socket to send our hostname to
 */
void MessengerClient::sendNameSlot(int s, QString uuid,QString theiruuid)
{
    this->send_msg(s, localHostname, "", "/hostname", uuid.toStdString().c_str(), theiruuid.toStdString().c_str());
    return;
}
