#include "includes/netcode.h"

int send_packet(const Client* c, const uint16_t type, const uint16_t length, const void* data) {
    if (c->sock == NET_INVALID_SOCKET) return NET_SEND_ERROR;

#ifdef WIN32
    PacketHeader net_header = { htons(type), htons(length)};

    int sent = 0;
    while (sent < (int)sizeof(PacketHeader)) {
        int n = send(c->sock, (char*)&net_header + sent, (int)sizeof(PacketHeader) - sent, 0);

        if (n > 0) { 
            sent += n;
            continue;
        } 
        
        if (n == 0) {
            return NET_SEND_DISCONNECT;
        } 

        int err = WSAGetLastError();

        if (err == WSAEWOULDBLOCK) {
            fd_set write;
            FD_ZERO(&write);
            FD_SET(c->sock, &write);

            int status = select(0, NULL, &write, NULL, NULL);
            
            if (status < 0) {
                return NET_SEND_ERROR;
            }

            continue; // retry send
        }

        if (err == WSAEINTR) {
            continue; // Interrupted → retry
        }

        return NET_SEND_ERROR;
    }

    sent = 0;
    while (sent < length) {
         int n = send(c->sock, (const char*)data + sent, length - sent, 0);

         if (n > 0) { 
            sent += n;
            continue;
        } 
        
        if (n == 0) {
            return NET_SEND_DISCONNECT;
        } 

        int err = WSAGetLastError();

        if (err == WSAEWOULDBLOCK) {
            fd_set write;
            FD_ZERO(&write);
            FD_SET(c->sock, &write);

            int status = select(0, NULL, &write, NULL, NULL);
            
            if (status < 0) {
                return NET_SEND_ERROR;
            }

            continue; // retry send
        }

        if (err == WSAEINTR) {
            continue; // Interrupted → retry
        }

        return NET_SEND_ERROR;
    }

    return NET_SEND_COMPLETE;
#else
    
#endif
}

int recv_packet(Client* c) {
    if (c->sock == NET_INVALID_SOCKET) return NET_RECV_ERROR;

#ifdef WIN32
    PacketHeader net_header;    

    int received = 0;
    while (received < (int)sizeof(PacketHeader)) {
        int n = recv(c->sock, (char*)&net_header + received, (int)sizeof(PacketHeader) - received, 0);
        
        if (n > 0) { 
            received += n;
            continue;
        } 
        
        if (n == 0) {
            return NET_RECV_DISCONNECT;
        } 

        int err = WSAGetLastError();

        if (err == WSAEWOULDBLOCK) {
            fd_set read;
            FD_ZERO(&read);
            FD_SET(c->sock, &read);

            int status = select(0, &read, NULL, NULL, NULL);
            if (status < 0) {
                return NET_RECV_ERROR;
            }

            continue;
        }

        if (err == WSAEINTR) {
            continue; // Interrupted → retry
        }

        return NET_RECV_ERROR;
    }

    c->packet.type = ntohs(net_header.type);
    c->packet.length = ntohs(net_header.length);

    if (c->packet.length > MAX_PAYLOAD_SIZE) {
        printf("Invalid payload size: %u", c->packet.length);
        return NET_RECV_ERROR;
    }

    int len = c->packet.length;
    received = 0;
    while (received < len) {
        int n = recv(c->sock, c->packet.payload.buffer + received, len - received, 0);
        
        if (n > 0) { 
            received += n;
            continue;
        } 
        
        if (n == 0) {
            return NET_RECV_DISCONNECT;
        } 

        int err = WSAGetLastError();

        if (err == WSAEWOULDBLOCK) {
            fd_set read;
            FD_ZERO(&read);
            FD_SET(c->sock, &read);

            int status = select(0, &read, NULL, NULL, NULL);
            if (status < 0) {
                return NET_RECV_ERROR;
            }

            continue;
        }

        if (err == WSAEINTR) {
            continue; // Interrupted → retry
        }

        return NET_RECV_ERROR;
    }
    
#else
    
#endif

    return NET_RECV_COMPLETE;
}





int set_nonblocking(net_socket sock) {
#ifdef WIN32
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
}