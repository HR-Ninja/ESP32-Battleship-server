#include "includes/netcode.h"

inline int net_init(const char* host, const char* port, Client* c) {
#ifdef WIN32
    WSADATA wsa_data;

    int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (result != 0) {
        return NET_FAIL;
    }

    address_info* ai = NULL;
    if (!host) { host = "localhost"; }
    if (!port) { port = DEFAULT_PORT; }

    address_info hints = {0};
    
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    result = getaddrinfo(host, port, &hints, ai);
    if (result != 0) {
        return NET_FAIL;
    }

    net_socket listen_sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (listen_sock == NET_INVALID_SOCKET) {
        freeaddrinfo(ai);
        return NET_FAIL;
    }


    result = bind(listen_sock, ai->ai_addr, (int)ai->ai_addrlen);
    if (result == NET_INVALID_SOCKET) {
        freeaddrinfo(ai);
        closesocket(listen_sock);
        return NET_FAIL;
    }

    freeaddrinfo(ai);

    result = listen(listen_sock, SOMAXCONN);
    if (result == NET_INVALID_SOCKET) {
        closesocket(listen_sock);
        return NET_FAIL;
    }

    c->sock = accept(listen_sock, NULL, NULL);
    if (c->sock == NET_INVALID_SOCKET) {
        closesocket(c->sock);
        return NET_FAIL;
    }

    closesocket(listen_sock);

#else

#endif

    return NET_SUCCESS;
}


inline int net_close(Client* c) {
#ifdef WIN32
    return closesocket(c->sock);
#else

#endif
    return NET_SUCCESS;
}

int net_send(const Client* c, const uint16_t type, const uint16_t length, const void* data) {
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
    // Add later
#endif
}

int net_recv(Client* c, TimeVal* tv) {
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

            int status = 0;
            if (tv == NULL) {
                status = select(0, &read, NULL, NULL, NULL);
            }
            else {
                status = select(0, &read, NULL, NULL, tv);
            }

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

            int status = 0;
            if (tv == NULL) {
                status = select(0, &read, NULL, NULL, NULL);
            }
            else {
                status = select(0, &read, NULL, NULL, tv);
            }

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
    // Add later
#endif

    return NET_RECV_COMPLETE;
}

int net_set_nonblocking(net_socket sock) {
#ifdef WIN32
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
}