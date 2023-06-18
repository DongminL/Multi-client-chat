#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

void* receive_sock(void* sock);
void* send_sock(void* sock);
char* setNickname();

struct sending_packet {
    char type[1024];
    char msg[2048];
};

pthread_t t_send, t_receive;   
char *nickname;

void main() {
    int sock_fd;
    struct sockaddr_in s_addr;
    int check;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) { 
        perror("socket failed: ");
        exit(1);
    }

    memset(&s_addr, '0', sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(8080);
    check = inet_pton(AF_INET, "127.0.0.1", &s_addr.sin_addr);
    if (check <= 0) {
        perror("inet_pton failed: ");
        exit(1);
    }

    check = connect(sock_fd, (struct sockaddr *) &s_addr, sizeof(s_addr));
    if (check < 0) {
        perror("connect failed: ");
        exit(1);
    }

    nickname = setNickname();    // Client Nickname

    printf("<<<<<<<<< Chat Client >>>>>>>>>>\n");
    printf("---------------menu-------------\n");
    printf("--------------------------------\n");
    printf("1. Change nickname\n");
    printf("2. Guess the number game (1 ~ 100)\n");
    printf("command : \"!(Number)\"\n");
    printf("Enter \"quit\" to end the chat.\n");
    printf("--------------------------------\n");

    pthread_create(&t_send, NULL, send_sock, (void *) &sock_fd);
    pthread_create(&t_receive, NULL, receive_sock, (void *) &sock_fd);
    pthread_join(t_send, NULL);
   

    close(sock_fd);
}

void* send_sock(void *sock) {
    int sock_fd = *((int *) sock);
    char *buffer = NULL;
    size_t len = 0;
    struct sending_packet pck;

    /* Entrance Info */
    printf(" >> Join the chat !!\n");
    printf("[%s]'s join.\n", nickname);
    sprintf(pck.msg, "[%s]'s join.\n", nickname);
    sprintf(pck.type, "INFO");
    send(sock_fd, (struct sending_packet *) &pck, sizeof(pck), 0);

    int flag = 0;
    while (1) {
        getline(&buffer, &len, stdin);
        sprintf(pck.msg, "%s", buffer);
        sprintf(pck.type, "%s", nickname);

        if (strcmp(pck.msg, "quit\n") == 0) {
            flag = -1;
        }
        /* Change Nickname */
        else if (strcmp(pck.msg, "!(1)\n") == 0) {
            char before[1024];
            strcpy(before, nickname);

            printf("You will change nickname.\n");
            nickname = setNickname();
            printf("Your nickname is changed for %s.\n", nickname);

            sprintf(pck.msg, "Changed nickname : [%s] -> [%s].\n", before, nickname);
            sprintf(pck.type, "INFO");
            send(sock_fd, (struct sending_packet *) &pck, sizeof(pck), 0);

            continue;
        }

        send(sock_fd, (struct sending_packet *) &pck, sizeof(pck), 0);

        if (flag == -1) {
            break;
        }
        else {
            printf("[%s] %s", pck.type, pck.msg);
        }
    }

    free(buffer);
    pthread_kill(t_receive, 2);    // Kill t_receive Thread
    shutdown(sock_fd, SHUT_RDWR);

    return NULL;
}

void* receive_sock(void *sock) {
    int sock_fd = *((int *) sock);
    struct sending_packet pck;

    /* Receive message */
    while(1) {
        recv(sock_fd, (struct sending_packet *) &pck, sizeof(pck), 0);
        
        if (strcmp(pck.type, "INFO") == 0) {
            printf("%s", pck.msg);
        }
        else {
            printf("[%s] %s", pck.type, pck.msg);
        }
    }

    return NULL;
}

char* setNickname() {
    char *nickname = malloc(sizeof(char) * 1024);    // Client Nickname

    printf("Please enter your nickname : ");
    scanf("%s", nickname);
    getchar();  // remove "\n"
    printf("--------------------------------\n");

    return nickname;
}