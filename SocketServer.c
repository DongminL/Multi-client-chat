#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <time.h>


#define MAX_CLIENT 100

void* server_handler(void* sock);
struct sending_packet receive_sock(int sock);
void send_sock(struct sending_packet pck, int client);
int mini_game(int target, int num, struct sending_packet pck);

struct sending_packet {
    char type[1024];
    char msg[2048];
};

int client_cnt = 0; // Current Clients count
int client_socks[MAX_CLIENT];   // Clients List
pthread_mutex_t mutex;  // Mutual Exclusion
int game = -1;   // -1 : Not in game

void main() {
    int s_sock_fd, new_sock_fd;
    struct sockaddr_in s_address, c_address;
    int addrlen = sizeof(s_address);
    int check;
    pthread_t tid; // Thread Object

    pthread_mutex_init(&mutex, NULL);   // Mutex Initialize
    s_sock_fd = socket(AF_INET, SOCK_STREAM, 0);   
    if (s_sock_fd == -1) {
        perror("socket failed: ");
        exit(1);
    }

    memset(&s_address, '0', addrlen);
    s_address.sin_family = AF_INET;
    s_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    s_address.sin_port = htons(8080);
    check = bind(s_sock_fd, (struct sockaddr *)&s_address, sizeof(s_address));
    if (check == -1) {
        perror("bind error: ");
        exit(1);
    }

    printf("<<<<< Chat Server >>>>>\n");

    while (client_cnt <= MAX_CLIENT) {
        check = listen(s_sock_fd, MAX_CLIENT);
        if (check == -1) {
            perror("listen failed: ");
            exit(1);
        }

        /* Add the clients list */
        if ((client_cnt + 1) <= MAX_CLIENT) {
            new_sock_fd = accept(s_sock_fd, (struct sockaddr *) &c_address, (socklen_t *) &addrlen);
            if (new_sock_fd < 0) {
                perror("accept failed: ");
                exit(1);
            }
    
            pthread_mutex_lock(&mutex);
            client_socks[client_cnt++] = new_sock_fd;
            pthread_mutex_unlock(&mutex);

            pthread_create(&tid, NULL, server_handler, (void *) &new_sock_fd);   
            pthread_detach(tid);    // exit individual threads
            printf("Chatter (%d / %d)\n", client_cnt, MAX_CLIENT);   // Display current number of clients
        }
    }
    

    // Before the server closes, close clients
    for (int i = 0; i < client_cnt; i++) {
        close(client_socks[i]);
    }

    pthread_exit(&tid);
    close(s_sock_fd);   // close server socket
}

void* server_handler(void* sock) {
    struct sending_packet pck;
    int client = *((int *) sock);
    int flag = 0;

    while(1) {
        pck = receive_sock(client);

        /* Quit Client */
        if (strcmp(pck.msg, "quit\n") == 0) {
            flag = -1;

            sprintf(pck.msg, "[%s] left the chat room.\n", pck.type);
            sprintf(pck.type, "INFO");
        }
        /* Mini Game */
        else if (strcmp(pck.msg, "!(2)\n") == 0) {
            pthread_mutex_lock(&mutex);
            srand((int) time(NULL));
            game = (rand() % 100) + 1;;
            pthread_mutex_unlock(&mutex);

            sprintf(pck.msg, "Guess one number from 1 to 100!\n");
            sprintf(pck.type, "INFO");

            send_sock(pck, -1); // Send to all client

            continue;
        }
        if (game != -1 && atoi(pck.msg) != 0) {
            send_sock(pck, client);

            if (mini_game(game, atoi(pck.msg), pck) == 1) {
                pthread_mutex_lock(&mutex);
                game = -1;
                pthread_mutex_unlock(&mutex);
            }

            continue;
        }

        send_sock(pck, client);
        
        if (flag == -1) {
            break;
        }
    }

    /* Delete disconnected client from the list */
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_cnt; i++) {
        if (client == client_socks[i]) {
            client_socks[i] = client_socks[--client_cnt];
            printf("Chatter (%d / %d)\n", client_cnt, MAX_CLIENT);   // Display current number of clients
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    shutdown(client, SHUT_RDWR);

    return NULL;
}

struct sending_packet receive_sock (int sock) {
    struct sending_packet pck;

    recv(sock, (struct sending_packet *) &pck, sizeof(pck), 0);
    return pck;
}

void send_sock(struct sending_packet pck, int client) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_cnt; i++) {
        if (client != client_socks[i])
            send(client_socks[i], (struct sending_packet *) &pck, sizeof(pck), 0);
    }
    pthread_mutex_unlock(&mutex);
}

int mini_game(int target, int num, struct sending_packet pck) {
    if (target == num) {
        sprintf(pck.msg, "[GAME] That's right answer!\n");
        sprintf(pck.type, "INFO");
        
        send_sock(pck, -1); // Send to all client

        return 1;
    }
    else if (target < num) {
        sprintf(pck.msg, "[GAME] Down\n");
        sprintf(pck.type, "INFO");
        
        send_sock(pck, -1); // Send to all client

        return 0;
    }
    else {
        sprintf(pck.msg, "[GAME] Up\n");
        sprintf(pck.type, "INFO");
        
        send_sock(pck, -1); // Send to all client

        return 0;
    }
}