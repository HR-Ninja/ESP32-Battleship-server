#ifndef NETCODE_H
#define NETCODE_H

#define WIN32

#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #define _WIN32_WINNT 0x0600

    #pragma comment (lib, "Ws2_32.lib")

    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>

    #define net_close(s) closesocket(s)
    #define NET_INVALID_SOCKET INVALID_SOCKET

    typedef SOCKET net_socket;
#else
    #include "lwip/sockets.h"   // ESP32 (lwIP)
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>

    #define net_close(s) close(s)
    #define NET_INVALID_SOCKET -1

    typedef int net_socket;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* -- Config -- */
#define DEFAULT_PORT "8080"
#define MAX_PAYLOAD_SIZE 512
#define MAX_SEND_QUEUE 32
#define MAX_RECV_QUEUE 32

/* -- Codes -- */
#define NET_SUCCESS 1
#define NET_FAIL    0

#define NET_RECV_ERROR      -2
#define NET_RECV_DISCONNECT -1
#define NET_RECV_COMPLETE    0
#define NET_RECV_WOULDBLOCK  1

#define NET_SEND_ERROR      -2
#define NET_SEND_DISCONNECT -1
#define NET_SEND_COMPLETE    0
#define NET_SEND_WOULDBLOCK  1

typedef struct {
    uint8_t buffer[MAX_PAYLOAD_SIZE];
} Payload;

typedef struct {
    uint16_t type;
    uint16_t length;
} PacketHeader;

typedef struct {
    Payload payload;
    uint16_t type;
    uint16_t length;
} Packet;

typedef struct {
    Packet items[MAX_SEND_QUEUE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} SendQueue;

typedef struct {
    Packet items[MAX_RECV_QUEUE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} RecvQueue;

typedef struct {
    uint16_t header_bytes_received;
    uint16_t packet_bytes_received;
    uint16_t expected_length;
    uint8_t has_header;
} RecvState;

typedef struct {
    Packet packet;
    net_socket  sock;
} Client;

/* -- Core -- */

static int init_socket();
static int set_nonblocking(net_socket sock);

// Blocking
static int send_packet(const Client* c, const uint16_t type, const uint16_t length, const void* data);
static int recv_packet(Client* c);


#endif
