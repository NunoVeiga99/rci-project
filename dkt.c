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
#include <fcntl.h>
#include <sys/select.h>
#include <time.h>
#include <signal.h>

#define max(A, B) ((A) >= (B) ? (A) : (B))

#define smallchar 5
#define bigchar 100
#define N 16                //número máximo de servidores num anel
#define udp_limite 2        //2 segundos de limite para o tempo udp
#define udp_maxtentativas 5 //3 tentativas máximas para repetir enviar a mensagem tcp

//Struct para guardar informações do servidor
struct server
{
    int key;
    char ipe[bigchar]; //read as IPê
    char porto[bigchar];
    struct server *next;
    struct server *next2;
} server;

//Struct connection
//Os valores estão = 1 quando a conexão está ativa
//e = 0 caso contrário
struct connection
{
    int sucessor;
    int predecessor;
    int nova;

} connection;

/*-----------------------------------------
Função create_UDP
Cria um socket UDP, ao qual atribui um descritor
-----------------------------------------*/
void sendmessageUDP(int fd, char ip[128], char porto[128], char message[128])
{

    struct addrinfo hints, *res;
    int errcode;
    ssize_t n;

    if (fd == -1) /*error*/
        exit(1);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      //IPv4
    hints.ai_socktype = SOCK_DGRAM; //UDP socket

    errcode = getaddrinfo(ip, porto, &hints, &res);
    if (errcode != 0) /*error*/
        exit(1);

    //Envia mensagem
    n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*error*/
        exit(1);
}

/*-----------------------------------------
Função create_TCP
Cria um socket TCP, ao qual atribui um descritor
-----------------------------------------*/
int create_TCP(char ip[128], char porto[128])
{

    struct addrinfo hints, *res;
    int n, fd;

    fd = socket(AF_INET, SOCK_STREAM, 0); //TCP socket
    if (fd == -1)
        exit(1); //error

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       //IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP socket

    n = getaddrinfo(ip, porto, &hints, &res);
    if (n != 0) /*error*/
    {
        fprintf(stderr, "ERRO CREATE: %s\n", gai_strerror(n));
        exit(1);
    }

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*error*/
    {
        fprintf(stderr, "ERRO2 connect: %s\n", gai_strerror(n));
        exit(1);
    }

    freeaddrinfo(res);

    return fd;
}

/*-----------------------------------------
Função sendmessageTCP
Envia uma mensagem TCP
-----------------------------------------*/
void sendmessageTCP(int fd, char message[130])
{
    ssize_t nbytes, nleft, nwritten;
    //ssize_t nread;
    //ssize_t nread;
    char *ptr, buffer[130];

    ptr = strcpy(buffer, message);
    nbytes = strlen(buffer);

    nleft = nbytes;
    while (nleft > 0)
    {
        nwritten = write(fd, ptr, nleft); //manda a mensagem
        if (nwritten <= 0)
        {
            fprintf(stderr, "ERRO WRITE:\n");
            exit(1);
        } /*error*/
        nleft -= nwritten;
        ptr += nwritten;
    }
    nleft = nbytes;
    ptr = buffer;
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

int distance(int k, int l)
{
    int result;
    result = (l - k) % N;
    if (result < 0)
    {
        result = N + result;
    }

    return result;
}

int main(int argc, char *argv[])
{

    int fd = -2, newfd = -2, sfd = -2, afd = -2;
    //int flags;
    int suc_fd = -2;
    int pre_fd = -2;
    fd_set rfds;

    //Inicializa a estrutura ligacao
    struct connection *ligacao = (struct connection *)malloc(sizeof(connection));
    ligacao->nova = 0;
    ligacao->predecessor = 0;
    ligacao->sucessor = 0;

    int maxfd, counter;

    //Inicializa o servidor
    //Aloca memória
    struct server *servidor = (struct server *)malloc(sizeof(server));
    servidor->next = (struct server *)malloc(sizeof(server));
    servidor->next2 = (struct server *)malloc(sizeof(server));
    servidor->key = -1;

    const char c[2] = "\n";
    const char s[2] = " "; //para procurar por espaços, usado no menu
    char *token;           //para guardar numa string o resto da string que se está a usar (menu)

    //VARIÁVEIS do servidor TCP
    struct addrinfo hints, *res;
    int errcode;
    ssize_t n;
    //ssize_t nw;
    struct sockaddr_in addr;
    socklen_t addrlen;
    char buffer[130], buffer_full[130], buffer_temp[130];

    //VARIÁVEIS do comandoos de utilização ou iterações
    char comando[128]; //guarda o comando inserido pelo utilizador
    char mensagem[130];
    char key_error[10];

    char mensagem1[65], mensagem2[65];
    int ler_nova = 0;
    int ler_suc = 0;
    int ler_pre = 0;

    //Variáveis do find
    char ipe_find[128];    //ip do servidor que pede o find
    char porto_find[128];  //porto do servidor que pede o find
    char ipe_found[128];   //ipe do servidor que tem a chave
    char porto_found[128]; //porto o servidor que tem a chave
    int key_found = -1;    // chave do servidor que tem a chave
    int find_fd = -2;      //ligacao
    int key_k = -1;        //key para procurar no find

    //variáveis do entry
    char ipe_entry[128];
    char porto_entry[128];
    int is_entry = 0;

    //VARIÁVEIS do servidor udp
    struct addrinfo udphints, *udpres;
    int udpfd;
    ssize_t nread;
    char host[NI_MAXHOST], service[NI_MAXSERV];

    struct timeval tempo_lim;
    tempo_lim.tv_sec = udp_limite; //segundos
    tempo_lim.tv_usec = 0;         //milisegundos

    int udp_tentativas = 0;

    time_t udp_ultima_tentativa = time(NULL);

    //Se na chamada do programa o numero de argumentos não fôr 3
    if (argc != 3)
    {
        printf("ERRO.\nInicialização inválida, volte a correr o programa!\n");
        exit(0);
    }

    //Guarda o ip e o porto na estrutura
    strcpy(servidor->ipe, argv[1]);
    strcpy(servidor->porto, argv[2]);

    //Lidar com o SIGIPE
    struct sigaction act; // estrutura necessário para lidar com o sigpipe
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1) /*error*/
        exit(1);

    // Cria socket TCP
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        exit(1); //error
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       //IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP socket
    //hints.ai_flags = AI_PASSIVE;

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

    while (1)
    {
        if ((ler_suc == 0) && (ler_pre == 0) && (ler_nova == 0))
        {
            memset(buffer, 0, sizeof(buffer));
        }
        memset(buffer_full, 0, sizeof(buffer));
        memset(buffer_temp, 0, sizeof(buffer_temp));
        memset(key_error,0,sizeof(key_error));

        n = 0;
        //"limpa" os descritores
        FD_ZERO(&rfds);

        //iniciaiza os descritores que estão sempre on
        FD_SET(0, &rfds);       //teclado
        FD_SET(fd, &rfds);      //espera TCP
        FD_SET(udpfd, &rfds);   //espera UDP
        maxfd = max(fd, udpfd); //vê o maximo dos fd

        if (ligacao->nova == 1)
        {
            FD_SET(afd, &rfds);
            maxfd = max(maxfd, afd);
        }
        if (ligacao->sucessor == 1)
        {
            FD_SET(suc_fd, &rfds);
            maxfd = max(maxfd, suc_fd);
        }
        if (ligacao->predecessor == 1)
        {
            FD_SET(pre_fd, &rfds);
            maxfd = max(maxfd, pre_fd);
        }

        // SELECT
        if (udp_tentativas > 0)
        {
            counter = select(maxfd + 1, &rfds, (fd_set *)NULL, (fd_set *)NULL, &tempo_lim);
        }
        else
        {
            counter = select(maxfd + 1, &rfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
        }

        if (counter <= 0)
        {
            printf("Erro no select");
            exit(1);
        } /*error*/

        //Verificar se há timeout de udp
        if (udp_tentativas > 0 && (time(NULL) - udp_ultima_tentativa) > udp_limite)
        {
            printf("À espera da resposta EFND");
            if (udp_tentativas < udp_maxtentativas)
            {
                printf("A reenviar mensagem udp");
                if (snprintf(mensagem, 130, "EFND %d\n", servidor->key) == -1)
                    exit(1);
                sendmessageUDP(udpfd, ipe_entry, porto_entry, mensagem);

                udp_ultima_tentativa = time(NULL);
                udp_tentativas = udp_tentativas + 1;
            }
            else
            {
                printf("Ocorreu erro, foram realizadas demasiadas tentativas de envio de mensagem sem sucesso");
                udp_tentativas = 0;
            }
        }




        // Se há input para ler
        //É dentro deste if que se lê o input do utilizador
        if (FD_ISSET(0, &rfds)) //o 0 está reservado para o teclado
        {
            if (fgets(comando, 128, stdin) == NULL)
            {
                //printf("ERRO na leitura de input");
                //exit(1);//do something
            } //receber input do teclado

            //Dividir as mensagens
            token = strtok(comando, s); //procurar no input onde está o espaço

            if (strcmp(comando, "new") == 0)
            {
                printf("Escolheu: new\n");
                token = strtok(NULL, s); //é o token que vai lendo as coisas SEGUINTES
                servidor->key = atoi(token);


                /* Verifica se o valor da chave inserida  é menor que o numero máximo N*/
                while (servidor->key > N)
                {
                    printf("A chave que inseriu é inválida.\nInsira um número menor que %d\n", N);
                    printf("Chave: ");
                    if (fgets(key_error, 10, stdin) == NULL);
                    servidor->key = atoi(key_error);
                }

                servidor->next->key = servidor->key;
                strcpy(servidor->next->ipe, argv[1]);
                strcpy(servidor->next->porto, argv[2]);
                servidor->next2->key = servidor->key;
                strcpy(servidor->next2->ipe, argv[1]);
                strcpy(servidor->next2->porto, argv[2]);
            }
            else if (strcmp(comando, "entry") == 0)
            {
                printf("Escolheu: entry\n");

                token = strtok(NULL, s); //não é preciso guardar, só é preciso passar à frente
                servidor->key = atoi(token);

                 /* Verifica se o valor da chave inserida  é menor que o numero máximo N*/
                while (servidor->key > N)
                {
                    printf("A chave que inseriu é inválida.\nInsira um número menor que %d para a chave do seu servidor\n", N);
                    printf("Chave: ");
                    if (fgets(key_error, 10, stdin) == NULL);
                    servidor->key = atoi(key_error);
                }

                token = strtok(NULL, s);

                token = strtok(NULL, s);
                strcpy(ipe_entry, token);

                token = strtok(NULL, s);
                token[strlen(token) - 1] = '\0';
                strcpy(porto_entry, token);

                if (snprintf(mensagem, 130, "EFND %d\n", servidor->key) == -1)
                    exit(1);
                udp_tentativas = 1;
                udp_ultima_tentativa = time(NULL);
                sendmessageUDP(udpfd, ipe_entry, porto_entry, mensagem);
            }
            else if (strcmp(comando, "sentry") == 0)
            {
                printf("Escolheu: sentry\n");

                token = strtok(NULL, s); //não é preciso guardar, só é preciso passar à frente
                servidor->key = atoi(token);

                 /* Verifica se o valor da chave inserida  é menor que o numero máximo N*/
                while (servidor->key > N)
                {
                    printf("A chave que inseriu é inválida.\nInsira um número menor que %d para a chave do seu servidor\n", N);
                    printf("Chave: ");
                    if (fgets(key_error, 10, stdin) == NULL);
                    servidor->key = atoi(key_error);
                }

                token = strtok(NULL, s);
                servidor->next->key = atoi(token);

                token = strtok(NULL, s);
                strcpy(servidor->next->ipe, token);

                token = strtok(NULL, s);
                token[strlen(token) - 1] = '\0';
                strcpy(servidor->next->porto, token);

                sfd = create_TCP(servidor->next->ipe, servidor->next->porto);

                /*
                if (snprintf(mensagem, 130, "NEW %d %s %s", servidor->key, servidor->ipe, servidor->porto) == -1)
                    exit(1);
                sendmessageTCP(sfd, mensagem);
                */

                if (snprintf(mensagem1, 130, "NEW %d ", servidor->key) == -1)
                    exit(1);
                printf("A MSG1 É: %s\n", mensagem1);
                if (snprintf(mensagem2, 130, "%s %s\n", servidor->ipe, servidor->porto) == -1)
                    ;
                printf("A MSG2 É: %s\n", mensagem2);
                sendmessageTCP(sfd, mensagem1);
                sendmessageTCP(sfd, mensagem2);

                //Guarda informação de ligação com o sucessor
                suc_fd = sfd;
                ligacao->sucessor = 1;
            }
            else if (strcmp(comando, "leave\n") == 0)
            {
                printf("Escolheu: leave\n");

                close(suc_fd);
                close(pre_fd);
                suc_fd = -2;
                pre_fd = -2;
                ligacao->sucessor = 0;
                ligacao->predecessor = 0;

                servidor->key = -1;

                servidor->next->key = -1;
                strcpy(servidor->next->ipe, " ");
                strcpy(servidor->next->porto, " ");

                servidor->next2->key = -1;
                strcpy(servidor->next2->ipe, " ");
                strcpy(servidor->next2->porto, " ");
            }
            else if (strcmp(comando, "show\n") == 0)
            {
                printf("Escolheu: show\n");

                printf("Chave do servidor:%d \n", servidor->key);
                printf("Endereço IP:%s \n", servidor->ipe);
                printf("Porto:%s \n", servidor->porto);

                printf("\nINFORMAÇÕES DO SUCESSOR \n");
                printf("Chave do succ:%d \n", servidor->next->key);
                printf("Endereço IP succ:%s \n", servidor->next->ipe);
                printf("Porto succ:%s \n", servidor->next->porto);

                printf("\nINFORMAÇÕES DO SEGUNDO SUCESSOR \n");
                printf("Chave do succ2:%d \n", servidor->next2->key);
                printf("Endereço IP succ2:%s \n", servidor->next2->ipe);
                printf("Porto succ2:%s \n\n", servidor->next2->porto);

                printf("Sessão com sucessor -> suc_fd = %d\n", suc_fd);
                printf("Sessão com predecessor -> pre_fd= %d\n\n\n", pre_fd);
            }
            else if (strcmp(comando, "find") == 0)
            {
                printf("Escolheu: find\n");

                token = strtok(NULL, s);
                key_k = atoi(token);

                 /* Verifica se o valor da chave inserida  é menor que o numero máximo N*/
                while (key_k > N)
                {
                    printf("A chave que inseriu é inválida.\nInsira um número menor que %d para a chave que procura\n", N);
                    printf("Chave: ");
                    if (fgets(key_error, 10, stdin) == NULL);
                    key_k = atoi(key_error);
                }

                if (servidor->key == servidor->next->key)
                {
                    if (write(1, "A chave que procura está no servidor em que se encontra\n", 58) == -1)
                        ;
                }
                else if (distance(key_k, servidor->next->key) > distance(key_k, servidor->key))
                {
                    if (snprintf(mensagem, 130, "FND %d %d %s %s\n", key_k, servidor->key, servidor->ipe, servidor->porto) == -1)
                        exit(1);
                    sendmessageTCP(suc_fd, mensagem);
                }
                else
                {
                    printf("A chave %d foi encontrada\n", key_k);
                    printf("Encontra-se no servidor %d\nIP: %s\nPorto: %s", servidor->next->key, servidor->next->ipe, servidor->next->porto);
                    printf("\n");
                }
            }
            else if (strcmp(comando, "exit\n") == 0)
            {
                printf("Escolheu: exit\n");
                close(fd);
                close(afd);
                exit(0);
            }
            else /* default: */
            {
                printf("Erro, insira um comando válido\n");
            }
        }

        //Conexão TCP
        if (FD_ISSET(fd, &rfds)) //receber TCP de alguem desconhecido
        {
            addrlen = sizeof(addr);

            //Aceita a conexão
            if ((newfd = accept(fd, (struct sockaddr *)&addr, &addrlen)) == -1) /*error*/
                exit(1);

            afd = newfd;

            if (ligacao->nova == 0)
            {
                ligacao->nova = 1;
                //break;
            }
        }

        //Canal com sucessor
        if (FD_ISSET(suc_fd, &rfds) && ligacao->sucessor == 1) //MENSAGEM DO SUCESSOR
        {
            if ((n = read(suc_fd, buffer_temp, 130)) != 0)
            {
                if (n == -1) //error
                    exit(1);

                if (strtok(buffer_temp, c) != NULL)
                {
                    if (ler_suc == 1)
                    {
                        strcat(buffer, buffer_temp);
                        ler_suc = 0;
                    }
                    else
                    {
                        strcpy(buffer, buffer_temp);
                    }
                }
                else
                {
                    if (ler_suc == 1)
                    {
                        strcat(buffer, buffer_temp);
                    }
                    else if (ler_suc == 0)
                    {
                        strcpy(buffer, buffer_temp);
                        ler_suc = 1;
                    }
                }

                if (write(1, "Mensagem tcp recebida: ", 24) == -1)
                    ;
                if (write(1, buffer, strlen(buffer)) == -1)
                    ;
                printf("\n");

                if (ler_suc == 0)
                {

                    strcpy(buffer_full, buffer);
                    token = strtok(buffer, s); //procurar no input onde está o espaço

                    if (strcmp(buffer, "SUCC") == 0)
                    {
                        token = strtok(NULL, s); //não é preciso guardar, só é preciso passar à frente
                        servidor->next2->key = atoi(token);

                        token = strtok(NULL, s);
                        strcpy(servidor->next2->ipe, token);

                        token = strtok(NULL, s);
                        strcpy(servidor->next2->porto, token);
                    }
                    else if (strcmp(buffer, "NEW") == 0)
                    {
                        servidor->next2->key = servidor->next->key;
                        strcpy(servidor->next2->ipe, servidor->next->ipe);
                        strcpy(servidor->next2->porto, servidor->next->porto);

                        token = strtok(NULL, s);
                        servidor->next->key = atoi(token);

                        token = strtok(NULL, s);
                        strcpy(servidor->next->ipe, token);

                        token = strtok(NULL, s);
                        strcpy(servidor->next->porto, token);

                        suc_fd = create_TCP(servidor->next->ipe, servidor->next->porto);

                        sendmessageTCP(suc_fd, "SUCCCONF\n");

                        if (snprintf(mensagem, 130, "SUCC %d %s %s\n", servidor->next->key, servidor->next->ipe, servidor->next->porto) == -1)
                            exit(1);
                        sendmessageTCP(pre_fd, mensagem);
                    }
                }
            }
            else if (n == 0) //Caso não haja sucessor (o sucessor saiu)
            {
                close(suc_fd);
                suc_fd = -2;

                if (servidor->key == servidor->next2->key)
                {
                    close(pre_fd);
                    pre_fd = -2;
                    ligacao->sucessor = 0;
                    ligacao->predecessor = 0;
                    servidor->next->key = servidor->key;
                    strcpy(servidor->next->ipe, servidor->ipe);
                    strcpy(servidor->next->porto, servidor->porto);
                }
                else
                {
                    servidor->next->key = servidor->next2->key;
                    strcpy(servidor->next->ipe, servidor->next2->ipe);
                    strcpy(servidor->next->porto, servidor->next2->porto);

                    suc_fd = create_TCP(servidor->next->ipe, servidor->next->porto);

                    sendmessageTCP(suc_fd, "SUCCCONF\n");

                    if (snprintf(mensagem, 130, "SUCC %d %s %s\n", servidor->next->key, servidor->next->ipe, servidor->next->porto) == -1)
                        exit(1);
                    sendmessageTCP(pre_fd, mensagem);
                }
            }
        }

        if (FD_ISSET(pre_fd, &rfds) && ligacao->predecessor == 1)
        {
            if ((n = read(pre_fd, buffer_temp, 130)) != 0)
            {

                if (n == -1)
                {
                    exit(1);
                }

                if (strtok(buffer_temp, c) != NULL)
                {
                    if (ler_pre == 1)
                    {
                        strcat(buffer, buffer_temp);
                        ler_pre = 0;
                    }
                    else
                    {
                        strcpy(buffer, buffer_temp);
                    }
                }
                else
                {
                    if (ler_pre == 1)
                    {
                        strcat(buffer, buffer_temp);
                    }
                    else if (ler_pre == 0)
                    {
                        strcpy(buffer, buffer_temp);
                        ler_pre = 1;
                    }
                }

                if (write(1, "Mensagem tcp recebida: ", 24) == -1)
                    ;
                if (write(1, buffer, strlen(buffer)) == -1)
                    ;
                printf("\n");

                if (ler_pre == 0)
                {

                    strcpy(buffer_full, buffer);
                    token = strtok(buffer, s); //procurar no input onde está o espaço

                    if (strcmp(buffer, "FND") == 0)
                    {
                        token = strtok(NULL, s);
                        key_k = atoi(token);

                        if (distance(key_k, servidor->next->key) > distance(key_k, servidor->key))
                        {
                            sendmessageTCP(suc_fd, buffer_full);
                        }
                        else
                        {
                            token = strtok(NULL, s); // só para saltar á frente a key, que não interessa para este caso

                            token = strtok(NULL, s);
                            strcpy(ipe_find, token);

                            token = strtok(NULL, s);
                            strcpy(porto_find, token);

                            find_fd = create_TCP(ipe_find, porto_find);
                            if (snprintf(mensagem, 130, "KEY %d %d %s %s\n", key_k, servidor->next->key, servidor->next->ipe, servidor->next->porto) == -1)
                                exit(1);
                            sendmessageTCP(find_fd, mensagem);
                        }
                    }
                }
            }
            if (n == 0)
            {
                close(pre_fd);
                pre_fd = -2;
                ligacao->predecessor = 0;
            }
        }

        //Conexão UDP
        if (FD_ISSET(udpfd, &rfds)) //receber UDP de alguem desconhecido
        {
            addrlen = sizeof(addr);
            nread = recvfrom(udpfd, buffer, 130, 0, (struct sockaddr *)&addr, &addrlen);
            if (nread == -1) /*error*/

                exit(1);

            if (write(1, "Mensagem udp recebida: ", 24) == -1)
                ;
            if (write(1, buffer, strlen(buffer)) == -1)
                ;

            if ((errcode = getnameinfo((struct sockaddr *)&addr, addrlen, host, sizeof host, service, sizeof service, 0)) != 0)
                fprintf(stderr, "error: getnameinfo: %s\n", gai_strerror(errcode));
            else
                printf("sent by [%s:%s]\n", host, service);

            strcpy(ipe_entry, host);
            strcpy(porto_entry, service);

            strcpy(buffer_full, buffer); //salvar a mensagem original
            token = strtok(buffer, s);   //procurar no input onde está o espaço

            if (strcmp(buffer, "EFND") == 0)
            {
                is_entry = 1;

                token = strtok(NULL, s);
                key_k = atoi(token);

                if (servidor->key == servidor->next->key)
                {

                    if (write(1, "A chave que procura está no servidor em que se encontra\n", 58) == -1)
                        ;
                    is_entry = 0;
                    if (snprintf(mensagem, 130, "EKEY %d %d %s %s\n", key_k, servidor->key, servidor->ipe, servidor->porto) == -1)
                        exit(1);
                    sendmessageUDP(udpfd, ipe_entry, porto_entry, mensagem);
                }
                else if (distance(key_k, servidor->next->key) > distance(key_k, servidor->key))
                {
                    if (snprintf(mensagem, 130, "FND %d %d %s %s\n", key_k, servidor->key, servidor->ipe, servidor->porto) == -1)
                        exit(1);
                    sendmessageTCP(suc_fd, mensagem);
                }
                else
                {
                    printf("A chave %d foi encontrada\n", key_k);
                    printf("Encontra-se no servidor %d\nIP: %s\nPorto: %s", servidor->next->key, servidor->next->ipe, servidor->next->porto);
                    printf("\n");
                    is_entry = 0;
                    if (snprintf(mensagem, 130, "EKEY %d %d %s %s\n", key_k, servidor->next->key, servidor->next->ipe, servidor->next->porto) == -1)
                        exit(1);
                    sendmessageUDP(udpfd, ipe_entry, porto_entry, mensagem);
                }
            }
            else if (strcmp(buffer, "EKEY") == 0)
            {
                udp_tentativas = 0; //significa que a mensagem foi recebida

                token = strtok(NULL, s); //não é preciso guardar, só é preciso passar à frente
                servidor->key = atoi(token);

                token = strtok(NULL, s);
                servidor->next->key = atoi(token);

                token = strtok(NULL, s);
                strcpy(servidor->next->ipe, token);

                token = strtok(NULL, s);
                token[strlen(token) - 1] = '\0'; // REMOVE O \n DO FIM
                strcpy(servidor->next->porto, token);

                sfd = create_TCP(servidor->next->ipe, servidor->next->porto);

                if (snprintf(mensagem, 130, "NEW %d %s %s\n", servidor->key, servidor->ipe, servidor->porto) == -1)
                    exit(1);
                sendmessageTCP(sfd, mensagem);

                //Guarda informação de ligação com o sucessor
                suc_fd = sfd;
                ligacao->sucessor = 1;
            }
        }

        //Receber mensagens de um servidor novo
        if (FD_ISSET(afd, &rfds)) //MENSAGENS DE FORA
        {
            if ((n = read(afd, buffer_temp, 130)) != 0)
            {
                if (n == -1)
                {
                    fprintf(stderr, "ERRO: %s\n", gai_strerror(n));
                    exit(1);
                }

                if (strtok(buffer_temp, c) != NULL)
                {
                    if (ler_nova == 1)
                    {
                        strcat(buffer, buffer_temp);
                        ler_nova = 0;
                    }
                    else
                    {
                        strcpy(buffer, buffer_temp);
                    }
                }
                else
                {
                    if (ler_nova == 1)
                    {
                        strcat(buffer, buffer_temp);
                    }
                    else if (ler_nova == 0)
                    {
                        strcpy(buffer, buffer_temp);
                        ler_nova = 1;
                    }
                }

                if (write(1, "Mensagem tcp recebida: ", 24) == -1)
                    ;
                if (write(1, buffer, strlen(buffer)) == -1)
                    ;
                printf("\n");

                //é a partir daqui que se INTERPRETA a mensagem

                if (ler_nova == 0)
                {

                    strcpy(buffer_full, buffer); //salvar a mensagem original
                    token = strtok(buffer, s);   //procurar no input onde está o espaço

                    ligacao->nova = 0; // se já lemos tudo da ligacao nova, fechamos o descritor

                    if (strcmp(buffer, "NEW") == 0)
                    {
                        if (servidor->key == servidor->next->key) //Se houver só um servidor no anel
                        {

                            token = strtok(NULL, s); //não é preciso guardar, só é preciso passar à frente (duvida, confirmar)
                            servidor->next->key = atoi(token);

                            token = strtok(NULL, s);
                            strcpy(servidor->next->ipe, token);

                            token = strtok(NULL, s);
                            strcpy(servidor->next->porto, token);

                            ligacao->predecessor = 1;
                            pre_fd = afd;
                            afd = -2;

                            //Diz a quem entrou quem é o seu duplo sucessor
                            //Que neste caso vai ser ele próprio (por isso tem o next)
                            if (snprintf(mensagem, 130, "SUCC %d %s %s\n", servidor->next->key, servidor->next->ipe, servidor->next->porto) == -1)
                                exit(1);

                            sendmessageTCP(pre_fd, mensagem);

                            ligacao->sucessor = 1;
                            suc_fd = create_TCP(servidor->next->ipe, servidor->next->porto);

                            sendmessageTCP(suc_fd, "SUCCCONF\n");
                        }
                        else
                        {
                            sendmessageTCP(pre_fd, buffer_full);

                            pre_fd = afd;
                            afd = -2;
                            //Diz a quem entrou quem é o seu duplo sucessor
                            //Que neste caso vai ser ele próprio (por isso tem o next)
                            if (snprintf(mensagem, 130, "SUCC %d %s %s\n", servidor->next->key, servidor->next->ipe, servidor->next->porto) == -1)
                                exit(1);
                            sendmessageTCP(pre_fd, mensagem);
                        }
                    }
                    else if (strcmp(buffer, "SUCCCONF") == 0)
                    {

                        ligacao->predecessor = 1;
                        pre_fd = afd;
                        afd = -2;

                        if (snprintf(mensagem, 130, "SUCC %d %s %s\n", servidor->next->key, servidor->next->ipe, servidor->next->porto) == -1)
                            exit(1);
                        sendmessageTCP(pre_fd, mensagem);
                    }
                    else if (strcmp(buffer, "KEY") == 0)
                    {
                        //chave que se estava á procura
                        token = strtok(NULL, s);
                        key_k = atoi(token);

                        //chave do servidor onde está key_k
                        token = strtok(NULL, s);
                        key_found = atoi(token);

                        //ip do servidor onde está key_k
                        token = strtok(NULL, s);
                        strcpy(ipe_found, token);

                        //porto do servidor onde está key_k
                        token = strtok(NULL, s);
                        strcpy(porto_found, token);

                        printf("A chave %d foi encontrada\n", key_k);
                        printf("Encontra-se no servidor %d\nIP: %s\nPorto: %s", key_found, ipe_found, porto_found);
                        printf("\n");

                        if (is_entry == 1)
                        {
                            is_entry = 0;

                            if (snprintf(mensagem, 130, "EKEY %d %d %s %s\n", key_k, key_found, ipe_found, porto_found) == -1)
                                exit(1);
                            sendmessageUDP(udpfd, ipe_entry, porto_entry, mensagem);
                        }
                    }
                }
            }
            else
            {
                close(afd);
                ligacao->nova = 0;
            } //connection closed by peer
        }
    }

    close(afd);
    close(fd);
    exit(0);
}