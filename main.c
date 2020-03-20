
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

//Struct para guardar informações do servidor
struct server
{
    int key;
    char ipe[bigchar]; //read as IPê
    char porto[bigchar];
    struct server *next;
    struct server *next2;
} server;

int sendmessage(int fd, char *message, char *ip, char *porto)
{

    struct addrinfo hints, *res;
    int n;
    ssize_t nbytes, nleft, nwritten, nread;
    char *ptr, buffer[128];

    fd = socket(AF_INET, SOCK_STREAM, 0); //TCP socket
    if (fd == -1)
        exit(1); //error
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       //IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP socket

    n = getaddrinfo(ip, porto, &hints, &res);
    if (n != 0) /*error*/
        exit(1);
    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*error*/
        exit(1);

    ptr = strcpy(buffer, message);
    nbytes = 7;

    nleft = nbytes;
    while (nleft > 0)
    {
        nwritten = write(fd, ptr, nleft);
        if (nwritten <= 0) /*error*/
            exit(1);
        nleft -= nwritten;
        ptr += nwritten;
    }
    nleft = nbytes;
    ptr = buffer;
    while (nleft > 0)
    {
        nread = read(fd, ptr, nleft);
        if (nread == -1) /*error*/
            exit(1);
        else if (nread == 0)
            break; //closed by peer
        nleft -= nread;
        ptr += nread;
    }
    nread = nbytes - nleft;
    close(fd);
    write(1, "echo: ", 6); //stdout
    write(1, buffer, nread);
    return fd;
}

int countspace(char *s, char c)
{
    int i, count = 0;

    for (i = 0; s[i]; i++)
    {
        if (s[i] == c)
        {
            count++;
        }
    }
    return count;
}

int main(int argc, char *argv[])
{
    int fd, newfd, sfd, afd = 0;
    fd_set rfds;
    enum
    {
        idle,
        busy
    } state;

    int maxfd, counter;

    //Inicializa o servidor
    //Aloca memória
    struct server *servidor = (struct server *)malloc(sizeof(server));
    servidor->next = (struct server *)malloc(sizeof(server));
    servidor->next2 = (struct server *)malloc(sizeof(server));

    const char s[2] = " "; //para procurar por espaços, usado no menu
    char *token;           //para guardar numa string o resto da string que se está a usar (menu)
    char c = ' ';

    //VARIÁVEIS do servidor TCP
    struct addrinfo hints, *res;
    int errcode;
    ssize_t n, nw;
    struct sockaddr_in addr;
    socklen_t addrlen;
    char *ptr, buffer[128];
    char *send, message[128];

    //VARIÁVEIS do comandoos de utilização ou iterações
    char comando[128];     //guarda o comando inserido pelo utilizador
    char comandofull[128]; //guarda o comando inserido pelo utilizador e NÃO O ALTERA
    int i = 0;
    int key = 0;
    int count = 0; //conta o num. de espaços no menu (entry e sentry)

    //VARIÁVEIS do servidor udp
    struct addrinfo udphints, *udpres;
    int udpfd;
    ssize_t nread;

    //VARIÁVEIS de cliente TCP
    ssize_t nbytes, nleft, nwritten;

    //Se na chamada do programa
    if (argc != 3)
    {
        printf("ERRO.\nInicialização inválida, volte a correr o programa!\n");
        exit(0);
    }

    //Guarda o ip e o porto na estrutura
    strcpy(servidor->ipe, argv[1]);
    printf("IP: %s\n", servidor->ipe);
    //servidor->porto = atoi(argv[2]);
    strcpy(servidor->porto, argv[2]);
    printf("Porto: %s\n", servidor->porto);

    // Cria socket TCP
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        exit(1); //error
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       //IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP socket
    hints.ai_flags = AI_PASSIVE;

    //Obtém o o endereço e atribui um porto ao servidor TCP
    if ((errcode = getaddrinfo(servidor->ipe, servidor->porto, &hints, &res)) != 0) /*error*/
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
    if ((errcode = getaddrinfo(NULL, servidor->porto, &udphints, &udpres)) != 0) /*error*/
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

            //printf("Insira um comando:\n");
            //fflush(stdout);
            //scanf("%s", comando);
            fgets(comando, 128, stdin); //receber input do teclado

            //strcpy(comandofull, comando);

            token = strtok(comando, s); //procurar no input onde está o espaço
            printf("comando: %s\n", comando);
            printf("token: %s\n", token);

            if (strcmp(comando, "new") == 0)
            {
                printf("Escolheu: new\n");
                token = strtok(NULL, s); //é o token que vai lendo as coisas SEGUINTES
                printf("token %d: %s\n", i, token);

                servidor->key = atoi(token);
                printf("A chave escolhida é: %d\n", servidor->key);

                servidor->next->key = atoi(token);
                printf("A chave do sucessor é: %d\n", servidor->next->key);
                strcpy(servidor->next->ipe, argv[1]);
                printf("Sucessor ip: %s\n", servidor->next->ipe);
                strcpy(servidor->next->porto, argv[2]);
                printf("Sucessor porto: %s\n", servidor->next->porto);

                servidor->next2->key = atoi(token);
                printf("A chave do sucessor 2 é: %d\n", servidor->next2->key);
                strcpy(servidor->next2->ipe, argv[1]);
                printf("Sucessor 2 IP: %s\n", servidor->next2->ipe);
                strcpy(servidor->next2->porto, argv[2]);
                printf("Sucessor 2 porto: %s\n", servidor->next2->porto);
            }
            else if (strcmp(comando, "entry") == 0)
            {
                printf("Escolheu: entry\n");
            }
            else if (strcmp(comando, "sentry") == 0)
            {
                printf("Escolheu: sentry\n");

                FD_SET(sfd, &rfds);

                //count = countspace(comandofull, c);
                //printf("token: %s \n", comandofull);
                //printf("character '%c' occurs %d times \n ", c, count);

                token = strtok(NULL, s); //não é preciso guardar, só é preciso passar à frente (duvida, confirmar)
                servidor->key = atoi(token);
                printf("A chave escolhida é: %d\n", servidor->key);

                token = strtok(NULL, s);
                servidor->next->key = atoi(token);
                printf("A chave do sucessor é: %d\n", servidor->next->key);

                token = strtok(NULL, s);
                strcpy(servidor->next->ipe, token);
                printf("Sucessor ip: %s\n", servidor->next->ipe);

                token = strtok(NULL, s);
                strcpy(servidor->next->porto, token);
                printf("Sucessor porto: %s\n", servidor->next->porto);

                //Connect TCP
                n = getaddrinfo(servidor->next->ipe, servidor->next->porto, &hints, &res); //Obtem os endereços do sucessor
                if (n != 0)                                                                /*error*/
                {
                    perror("ERRO1:");
                }
                n = connect(fd, res->ai_addr, res->ai_addrlen); //Conecta ao sucessor
                if (n == -1)                                    /*error*/
                {
                    perror("ERRO2:");
                }

                send = strcpy(buffer, "SUCCCONF\n"); //Informa ao sucessor que a configuração foi bem feita

                nleft = nbytes;

                //Envia a mensagem (write)
                while (nleft > 0)
                {
                    nwritten = write(fd, send, nleft);
                    if (nwritten <= 0) /*error*/
                        exit(1);
                    nleft -= nwritten;
                    send += nwritten;
                }
                nleft = nbytes;
                send = message;

                //read (receive echo)
                while (nleft > 0)
                {
                    nread = read(fd, send, nleft);
                    if (nread == -1) /*error*/
                        exit(1);
                    else if (nread == 0)
                        break; //closed by peer
                    nleft -= nread;
                    send += nread;
                }
                nread = nbytes - nleft;

                write(1, "echo: ", 6); //stdout
                write(1, message, nread);

                /*
                while(token!=NULL)
                {
                    
                    
                }*/

                //strcpy(servidor->next->ipe,argv[2]);
                //printf("Succ ip: %s", servidor->next->ipe);
            }
            else if (strcmp(comando, "leave\n") == 0)
            {
                printf("Escolheu: leave\n");
            }
            else if (strcmp(comando, "show\n") == 0)
            {
                printf("Escolheu: show\n");

                printf("Chave do servidor:%d \n", servidor->key);
                printf("Endereço IP:%s \n", servidor->ipe);
                printf("Porto:%s \n", servidor->porto);

                printf("\n INFORMAÇÕES DO SUCESSOR \n");
                printf("Chave do succ:%d \n", servidor->next->key);
                printf("Endereço IP succ:%s \n", servidor->next->ipe);
                printf("Porto succ:%s \n", servidor->next->porto);

                printf("\n INFORMAÇÕES DO SEGUNDO SUCESSOR \n");
                printf("Chave do succ2:%d \n", servidor->next2->key);
                printf("Endereço IP succ2:%s \n", servidor->next2->ipe);
                printf("Porto succ2:%s \n", servidor->next2->porto);
            }
            else if (strcmp(comando, "find") == 0)
            {
                printf("Escolheu: find\n");
            }
            else if (strcmp(comando, "exit\n") == 0)
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
    free(comando);
    free(comandofull);
    /*close(fd);exit(0);*/
}
