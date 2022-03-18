#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int errno;

// portul de conectare la server
int port;
char password[55];

int main (int argc, char *argv[]){

    char msg[400];
    int sd;


    struct sockaddr_in server;

    if (argc != 3)
    {
        printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    port = atoi (argv[2]);

    if((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if(connect(sd, (struct sockaddr*)&server, sizeof(struct sockaddr)) == -1){
        perror("[client]eroare la connect");
        return errno;
    }


    int length_answer;
    int length_comand;
    while(1) {
        bzero(msg, 400);

        if (read(sd, &length_answer, sizeof(int)) < 0) {
            perror("Eroare la read:");
            close(sd);
            return errno;
        }
        if (read(sd, msg, length_answer) < 0) {
            perror("[client] Eroare la read");
            close(sd);
            return errno;
        }

        printf("%s", msg);
        fflush(stdout);
        bzero(msg,400);

        read(0, msg, 400);

        length_comand = strlen(msg);
        if(write(sd, &length_comand, sizeof(int)) <= 0){
            {
                perror("[client] Eroare la write");
                return errno;
            }
        }

        if (write(sd, msg, length_comand) <= 0) {
            perror("[client] Eroare la write");
            return errno;
        }



    }

    close(sd);
    return 0;
}