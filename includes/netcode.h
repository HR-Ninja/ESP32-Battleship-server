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

    #define NET_INVALID_SOCKET INVALID_SOCKET

    typedef struct addrinfo address_info;
    typedef SOCKET net_socket;
#else
    #include "lwip/sockets.h"   // ESP32 (lwIP)
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>

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
#define MAX_QUEUE_SIZE 64

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

typedef struct timeval TimeVal;

typedef struct {
    uint8_t buffer[MAX_PAYLOAD_SIZE];
} Payload;

typedef struct {
    uint16_t type;
    uint16_t length;
} PacketHeader;

typedef struct {
    Payload payload;
    PacketHeader header;
} Packet;

typedef struct {
    Packet items[MAX_QUEUE_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} Queue;

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

typedef Queue SendQueue;
typedef Queue RecvQueue;

/* -- Core -- */
extern inline int net_init(const char* host, const char* port, Client* c);
extern inline int net_close(Client* c);
extern int net_set_nonblocking(net_socket sock);

// Blocking
extern int net_send(const Client* c, const uint16_t type, const uint16_t length, const void* data);
extern int net_recv(Client* c, const TimeVal* tv);

// Queue helpers
extern inline void net_init_queue(Queue* q);
extern inline int net_is_queue_empty(const Queue* q);
extern inline int net_is_queue_full(const Queue* q);
extern const Packet* net_queue_peek(const Queue* q);
extern int net_enqueue_packet(Queue* q, const Packet* data);
extern Packet* net_dequeue_packet(Queue* q);

// Recv


// Send
extern void net_flush_queue(Queue* q);

#endif
