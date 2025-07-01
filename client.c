#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 2739
#define BUFFER_SIZE 256


int main() 
{
   
    int sockclient;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if ((sockclient = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[client] Eroare la creare socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("[client] Invalid server IP address");
        close(sockclient);
        return 1;
    }

    if (connect(sockclient, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[client] Eroare la conectare la server");
        close(sockclient);
        return 1;
    }

    printf("[client] Conectat la server la %s:%d\n", SERVER_IP, SERVER_PORT);

    while (1) {
        bzero(buffer, sizeof(buffer));

        int bytes = recv(sockclient, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            printf("[client] Server-ul a inchis conexiunea.\n");
            break;
        }

        buffer[bytes] = '\0';
        printf("[server] %s", buffer);

        if (strstr(buffer, "Quizz Incheiat!") != NULL || strstr(buffer, "castigatorul") != NULL) {
            continue;
        }

        if (strstr(buffer, "Intrebare:") != NULL) {
            printf("[client] Ai 10 secunde pentru a raspunde.\n");

            fd_set readfds;
            struct timeval timpr;

            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            timpr.tv_sec = 10;
            timpr.tv_usec = 0;

            int cselect = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timpr);
            if (cselect > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
                bzero(buffer, sizeof(buffer));
                fgets(buffer, sizeof(buffer), stdin);
                buffer[strcspn(buffer, "\n")] = 0;

                if (strlen(buffer) == 0) {
                strcpy(buffer, " "); 
                                   }


                send(sockclient, buffer, strlen(buffer), 0);
            } else {
                printf("[client] Nu ai raspuns la timp.\n");
                send(sockclient, "", 0, 0);
            }
        }
    }

    close(sockclient);
    printf("[client] Deconectat de la server.\n");
    return 0;
}








