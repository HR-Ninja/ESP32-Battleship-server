#include "includes/netcode.h"
#include <stdbool.h>
#include <signal.h>
#include <process.h> 

static int g_running = 1;
static const g_tick_rate = 1000 / 60;

void handle_sigint(int sig) {
    (void)sig;  // unused
    g_running = 0;
}

// Receiver thread function
unsigned __stdcall recv_thread(void* arg) {
    Client* c = (Client*)arg;

    TimeVal tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while (g_running) {
        int rc = net_recv(c, &tv);

        if (rc == NET_RECV_COMPLETE) {
            printf("[RECV] type=%u length=%u\n",
                   c->packet.header.type, c->packet.header.length);

            if (c->packet.header.length > 0) {
                printf("[RECV] payload: '%.*s'\n",
                       c->packet.header.length,
                       (char*)c->packet.payload.buffer);
            }
        }
        else if (rc == NET_RECV_DISCONNECT) {
            printf("[RECV] client disconnected\n");
            g_running = 0;
            break;
        }
        else if (rc == NET_RECV_ERROR) {
            printf("[RECV] error\n");
            g_running = 0;
            break;
        }
        // else timeout → loop continues
    }

    return 0;
}

int main(void) {

    signal(SIGINT, handle_sigint);


    Client client;
    client.sock = NET_INVALID_SOCKET;

    printf("Starting server on port %s...\n", DEFAULT_PORT);

    if (net_init(NULL, DEFAULT_PORT, &client) != NET_SUCCESS) {
        printf("Net Init failed.\n");
        return -1;
    }

    printf("Client connected! Socket = %d\n", (int)client.sock);

    if (net_set_nonblocking(client.sock) != 0) {
        printf("Warning: could not set non-blocking mode\n");
    }

    uintptr_t thread = _beginthreadex(NULL, 0, recv_thread, &client, 0, NULL);

    while (g_running) {
        DWORD start = GetTickCount();


        printf("[TICK] doing server work...\n");

        const char* msg = "tick";
        net_send(&client, 1, (uint16_t)strlen(msg), msg);


        DWORD elapsed = GetTickCount() - start;
        if (elapsed < (DWORD)g_tick_rate) {
            Sleep(g_tick_rate - elapsed);
        }
    }

    WaitForSingleObject((HANDLE)thread, INFINITE);
    CloseHandle((HANDLE)thread);

    net_close(&client);
    printf("Server shut down.\n");
    return 0;
}