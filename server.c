#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>      
#include <libxml/parser.h>
#include <libxml/tree.h>



typedef struct 
{
    int socket;
    int scor;
    int activ;
} Client;

int PORT;
int num_intrebari; 
Client* clienti = NULL;          
pthread_t* threads = NULL;
int nr_clienti = 0;
int participanti;
int joc_inceput = 0;
int frv[100];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
xmlNode *root;

int NrIntrebari(xmlNode *node) 
{
    int count = 0;
    for (xmlNode *cur_node = node; cur_node; cur_node = cur_node->next) 
    {
        if (cur_node->type == XML_ELEMENT_NODE && strcmp((const char *)cur_node->name, "question") == 0) 
        {
            count++;
        }
        count += NrIntrebari(cur_node->children); 
    }
    return count;
}

void IntrebaresiRaspuns(xmlNode *node, int intrebarei, char *intrebare, char *raspuns) 
{
    int count = 0;
    for (xmlNode *cur_node = node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE && strcmp((const char *)cur_node->name, "question") == 0) 
        {
            if (count == intrebarei) 
            {
                for (xmlNode *child = cur_node->children; child; child = child->next) 
                {
                    if (child->type == XML_ELEMENT_NODE) 
                    {
                        if (strcmp((const char *)child->name, "text") == 0) 
                        {
                            strcpy(intrebare, (const char *)xmlNodeGetContent(child));
                        } else if (strcmp((const char *)child->name, "answer") == 0) 
                        {
                            strcpy(raspuns, (const char *)xmlNodeGetContent(child));
                        }
                    }
                }
                return;
            }
            count++;
        }
        IntrebaresiRaspuns(cur_node->children, intrebarei, intrebare, raspuns);
    }
}


void* clientf(void* arg) 
{
    int client_index = *(int*)arg;
    free(arg);
    char buffer[256];
    
    printf("[server] Client %d conectat.\n", client_index);

    while (!joc_inceput) 
    {
        usleep(100000);
    }

    for (int q = 0; q < num_intrebari; q++) 
    {
    char intrebare[256];
    char raspuns[256];
        
        if (!clienti[client_index].activ) 
        {
            break;
        }

        memset(buffer,0, sizeof(buffer));
        
        IntrebaresiRaspuns(root,q,intrebare,raspuns);
        
        snprintf(buffer, sizeof(buffer), "Intrebare: %s\n", intrebare); 
        send(clienti[client_index].socket, buffer, strlen(buffer), 0);

        fd_set readfds;
        struct timeval timpr;
        FD_ZERO(&readfds);
        FD_SET(clienti[client_index].socket, &readfds);
        timpr.tv_sec = 10;
        timpr.tv_usec = 0;

        int cselect = select(clienti[client_index].socket + 1, &readfds, NULL, NULL, &timpr);

        if (cselect > 0 && FD_ISSET(clienti[client_index].socket, &readfds)) 
        {
            memset(buffer, 0,sizeof(buffer));
            int bytes = recv(clienti[client_index].socket, buffer, sizeof(buffer) - 1, 0);

            if (bytes <= 0) 
            {
                printf("[server] Client %d deconectat.\n", client_index);
                pthread_mutex_lock(&lock);
                clienti[client_index].activ = 0;
                pthread_mutex_unlock(&lock);
                break;
            }

            buffer[bytes] = '\0';
            pthread_mutex_lock(&lock);
            if (strcmp(buffer,raspuns) == 0) 
            {
                clienti[client_index].scor++;
                printf("[server] Client %d a raspuns corect.\n", client_index);
            } 
            else 
            {
                printf("[server] Client %d a raspuns incorect.\n", client_index);
            }
            pthread_mutex_unlock(&lock);
        } 
        else 
        {
            printf("[server] Client %d nu a raspuns la timp.\n", client_index);
        }
    }

    return NULL;
}

int main() 
{
    printf("Introduceti portul: ");
    scanf("%d",&PORT);
    
    printf("Cati jucatori participa? (Puteti introduce orice numar) ");
    scanf("%d", &participanti);
 
    clienti = (Client*)malloc(sizeof(Client));
    threads = (pthread_t*)malloc(sizeof(pthread_t));
 
    xmlInitParser();
    xmlDoc *date = xmlReadFile("intrebarir.xml", NULL, 0);
    if (date == NULL) 
    {
        printf("Eroare la deschiderea fisierului XML.\n");
        return 1;
    }

    root = xmlDocGetRootElement(date);
    num_intrebari = NrIntrebari(root);
    if (num_intrebari == 0) {
    printf("Nu exista intrebari in fisierul XML.\n");
    xmlFreeDoc(date);
    xmlCleanupParser();
    return 1;
}
    
    struct sockaddr_in server_addr, client_addr;
    int server_sock, client_sock;
    socklen_t client_len = sizeof(client_addr);
    int optval = 1;

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        perror("[server] Eroare la crearea socket.\n");
        return errno;
    }

    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    memset(&server_addr, 0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("[server] Eroare la bind.\n");
        return errno;
    }

    if (listen(server_sock, SOMAXCONN) == -1) 
    {
        perror("[server] Eroare la listen.\n");
        return errno;
    }

    printf("[server] Asteptam conexiuni noi la port %d...\n", PORT);

    while (nr_clienti < participanti) 
    {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("[server] Eroare la accept.\n");
            continue;
        }
        
        pthread_mutex_lock(&lock);
        
        clienti = realloc(clienti, (nr_clienti + 1) * sizeof(Client));
        threads = realloc(threads, (nr_clienti + 1) * sizeof(pthread_t));
        
        clienti[nr_clienti].socket = client_sock;
        clienti[nr_clienti].scor = 0;
        clienti[nr_clienti].activ = 1;
        int* client_index = malloc(sizeof(int));
        *client_index = nr_clienti;
        pthread_create(&threads[nr_clienti], NULL, clientf, client_index);
        nr_clienti++;
        pthread_mutex_unlock(&lock);

        printf("[server] Client nou conectat de la %s\n", inet_ntoa(client_addr.sin_addr));
    }

    printf("[server] Toti clientii s-au conectat. Scrie 'start' sa incepi quizz-ul.\n");
    char command[10];
    scanf("%s", command);
    if (strcmp(command, "start") == 0) 
    {
        joc_inceput = 1;
           
        pthread_mutex_lock(&lock);
    for (int i = 0; i < nr_clienti; i++) 
    {
        if (clienti[i].activ) {
            send(clienti[i].socket, "Incepe jocul!\n", 15, 0);
        }
    }
    pthread_mutex_unlock(&lock);
        
        
    }

    for (int i = 0; i < nr_clienti; i++) {
        pthread_join(threads[i], NULL);
    }

    
    int scor_max = 0;
    int cindex = -1;
    char buffer[256];
    int egal=0;

    pthread_mutex_lock(&lock);
    for (int i = 0; i < nr_clienti; i++) 
    {
        if (clienti[i].activ && clienti[i].scor > scor_max) 
        {
            scor_max = clienti[i].scor;
            cindex = i;
        }
    }

    for (int i = 0; i < nr_clienti; i++) 
    {
        if (clienti[i].activ && clienti[i].scor == scor_max && cindex!=i) 
        {
            frv[i]++;
            egal=1;
        }
    }

    for (int i = 0; i < nr_clienti; i++) 
    {
        if (clienti[i].activ) 
        {
            memset(buffer, 0,sizeof(buffer));
            snprintf(buffer, sizeof(buffer), "Quizz Incheiat! Scorul tau final: %d\n", clienti[i].scor);
            send(clienti[i].socket, buffer, strlen(buffer), 0);

            if (i == cindex) 
            {   if(egal==0)
                snprintf(buffer, sizeof(buffer), "Felicitari! Tu esti castigatorul cu %d puncte!\n", scor_max);
                else 
                snprintf(buffer, sizeof(buffer), "Ai castigat la Egalitate! Ai terminat cu %d puncte!\n", scor_max);
            }
            else if(i!=cindex && frv[i]>0)
            {
                snprintf(buffer,sizeof(buffer),"Ai castigat la Egalitate! Ai acelasi scor cu jucatorul %d ,%d puncte!\n",cindex , scor_max);
            
            } 
            else 
            {
                snprintf(buffer, sizeof(buffer), "castigatorul este client %d cu %d puncte.\n", cindex , scor_max);
            }
            send(clienti[i].socket, buffer, strlen(buffer), 0);

            close(clienti[i].socket);
        }
    }
    pthread_mutex_unlock(&lock);
    
    xmlFreeDoc(date);
    xmlCleanupParser();
    free(clienti);
    free(threads);
    close(server_sock);
    printf("[server] Quizz Incheiat. Server-ul se inchide\n");
    return 0;
}

