
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

/* ... */
#define max(A, B) ((A) >= (B) ? (A) : (B))

#define smallchar 5
#define bigchar 100

int main(void)
{
    int fd, newfd, afd = 0;
    fd_set rfds;
    enum
    {
        idle,
        busy
    } state;

    int maxfd, counter;

    const char s[2] = " "; //para procurar por espaços, usado no menu
    char *token;           //para guardar numa string o resto da string que se está a usar (menu)

    // Variáveis do servidor TCP
    struct addrinfo hints, *res;
    int errcode;
    ssize_t n, nw;
    struct sockaddr_in addr;
    socklen_t addrlen;
    char *ptr, buffer[128];
    char pedro[128]; //para eu experimentar fazer cenas - TESTE
    int teste = 0;   //TESTE
    int i = 0;

    //Struct para guardar informações do servidor
    struct 
    {
        int key;
        char IP[bigchar];
        char next_server[bigchar];
        char next2_server[bigchar];
    } server;

    // Variáveis do servidor udp
    struct addrinfo udphints, *udpres;
    int udpfd;
    ssize_t nread;

    // Cria socket TCP
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        exit(1); //error
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       //IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP socket
    hints.ai_flags = AI_PASSIVE;

    //Obtém o o endereço e atribui um porto ao servidor TCP
    if ((errcode = getaddrinfo(NULL, "58001", &hints, &res)) != 0) /*error*/
        exit(1);

    //bind e listen TCP
    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) /*error*/
        exit(1);
    if (listen(fd, 15) == -1) /*error*/
        exit(1);

    // Cria um socket UDP
    if ((udpfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        exit(1); //error
    memset(&udphints, 0, sizeof udphints);
    udphints.ai_family = AF_INET;      //IPv4
    udphints.ai_socktype = SOCK_DGRAM; //UDP socket
    udphints.ai_flags = AI_PASSIVE;

    //Obtém endereço e atribui porto ao servidor UDP
    if ((errcode = getaddrinfo(NULL, "58001", &udphints, &udpres)) != 0) /*error*/
        exit(1);

    //bind UDP
    if (bind(udpfd, udpres->ai_addr, udpres->ai_addrlen) == -1) /*error*/
        exit(1);

    //Inicializa o "descritor"(?)
    FD_ZERO(&rfds);

    state = idle; // idle = não está ocupado

    while (1)
    {

        //input do utilizador
        FD_SET(0, &rfds);
        FD_SET(fd, &rfds);
        FD_SET(udpfd, &rfds);
        maxfd = max(fd, udpfd);

        // Se está ocupado
        if (state == busy)
        {
            FD_SET(afd, &rfds);
            maxfd = max(maxfd, afd);
        }

        // SELECT
        counter = select(maxfd + 1, &rfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);

        if (counter <= 0) /*error*/
            exit(1);

        // Se há input para ler
        //É dentro deste if que se lê o input do utilizador
        if (FD_ISSET(0, &rfds))
        {
            fflush(stdout);
            printf("Insira um comando:\n");
            //fprintf(stdout, "Insira um comando:\n");
            fflush(stdout);
            //scanf("%s", pedro);
            fgets(pedro, 128, stdin); //receber input do teclado

            token = strtok(pedro, s); //procurar no input onde está o espaço
            printf("pedro: %s\n", pedro);
            printf("token: %s\n", token);

            if (strcmp(pedro, "new") == 0)
            {
                printf("Escolheu: new\n");
                while (token != NULL)
                {
                    token = strtok(NULL, s);
                    printf("token %d: %s\n", i, token);
                    i++;
                }
            }
            else if (strcmp(pedro, "entry") == 0)
            {
                printf("Escolheu: entry\n");
            }
            else if (strcmp(pedro, "sentry") == 0)
            {
                printf("Escolheu: sentry\n");
            }
            else if (strcmp(pedro, "leave\n") == 0)
            {
                printf("Escolheu: leave\n");
            }
            else if (strcmp(pedro, "show\n") == 0)
            {
                printf("Escolheu: show\n");
            }
            else if (strcmp(pedro, "find") == 0)
            {
                printf("Escolheu: find\n");
            }
            else if (strcmp(pedro, "exit\n") == 0)
            {
                printf("Escolheu: exit\n");
            }
            else /* default: */
            {
                printf("Erro, insira um comando válido\n");
            }
        }

        //Conexão TCP
        if (FD_ISSET(fd, &rfds))
        {
            addrlen = sizeof(addr);
            //Aceita a conexão
            if ((newfd = accept(fd, (struct sockaddr *)&addr, &addrlen)) == -1) /*error*/
                exit(1);

            // Lê a mensagem
            while ((n = read(newfd, buffer, 128)) != 0)
            {
                if (n == -1) /*error*/
                    exit(1);
                ptr = &buffer[0];
                while (n > 0)
                {
                    if ((nw = write(newfd, ptr, n)) <= 0) /*error*/
                        exit(1);
                    n -= nw;
                    ptr += nw;
                }
            }
            write(1, "Mensagem tcp recebida: ", 24);
            write(1, buffer, 7);

            //Esta parte do codigo ainda não sei para que é que serve, nem sei para que é o afd
            switch (state)
            {
            case idle:
                afd = newfd;
                state = busy;
                break;
            case busy: /* ... */ //write “busy\n” in newfd
                close(newfd);
                break;
            }
        }

        //Conexão UDP
        if (FD_ISSET(udpfd, &rfds))
        {
            addrlen = sizeof(addr);
            nread = recvfrom(udpfd, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
            if (nread == -1) /*error*/
                exit(1);
            n = sendto(udpfd, buffer, nread, 0, (struct sockaddr *)&addr, addrlen);
            if (n == -1) /*error*/
                exit(1);

            write(1, "Mensagem udp recebida: ", 24);
            write(1, buffer, 7);
        }

        //Não sei para que é que serve o fucking afd mano (Pedro: EU NÃO PERCEBO NADA DISTO AHAHAHHAHA)
        if (FD_ISSET(afd, &rfds))
        {
            if ((n = read(afd, buffer, 128)) != 0)
            {
                if (n == -1) /*error*/
                    exit(1);
                /* ... */ //write buffer in afd
            }
            else
            {
                close(afd);
                state = idle;
            } //connection closed by peer
        }
    } //while(1)
    /*close(fd);exit(0);*/
}
