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

/* ... */
#define max(A, B) ((A) >= (B) ? (A) : (B))

#define smallchar 5
#define bigchar 100
#define N 16 //número máximo de servidores num anel

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
    //struct sockaddr_in addr;
    //socklen_t addrlen;
    //char buffer[130];                 CONFIRMAR SE POSSO APAGAR
    //char host[NI_MAXHOST], service[NI_MAXSERV]; //consts in <netdb.h>

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
    //int flags;

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

    //Torna o server non-blocking
    //flags = fcntl(fd, F_GETFL, 0);
    //fcntl(fd, F_SETFL, flags | O_NONBLOCK);

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
            //printf("ERRO NO WRITE");
            exit(1);
        } /*error*/
        nleft -= nwritten;
        ptr += nwritten;
    }
    nleft = nbytes;
    ptr = buffer;

    /*
    while (nleft > 0)
    {
        nread = read(fd, ptr, nleft); //Recebe o eco
        if (nread == -1)
        {
            fprintf(stderr, "ERRO2 nread\n");
            exit(1);
        } //error

        else if (nread == 0)
            break; //closed by peer
        nleft -= nread;
        ptr += nread;
    }
    nread = nbytes - nleft;
    write(1, "echo: ", 6); //stdout
    write(1, buffer, nread);
    write(1, "\n", 2);
    */
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
    //char c = ' ';

    //VARIÁVEIS do servidor TCP
    struct addrinfo hints, *res;
    int errcode;
    ssize_t n;
    //ssize_t nw;
    struct sockaddr_in addr;
    socklen_t addrlen;
    char buffer[130], buffer_full[130], buffer_temp[130];
    //char *ptr;
    //char *send;
    //char message[130];

    //VARIÁVEIS do comandoos de utilização ou iterações
    char comando[128]; //guarda o comando inserido pelo utilizador
    //char comandofull[130]; //guarda o comando inserido pelo utilizador e NÃO O ALTERA
    //int i = 0;
    //int key = 0;
    //int count = 0; //conta o num. de espaços no menu (entry e sentry)

    char mensagem[130];
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
    //int key_original;      //key para o server que mandou o pedido de find

    //variáveis do entry
    char ipe_entry[128];
    char porto_entry[128];
    //int key_entry;
    int is_entry = 0;
    //struct sockaddr_in entry_addr;
    //socklen_t entry_addrlen;

    //VARIÁVEIS do servidor udp
    struct addrinfo udphints, *udpres;
    int udpfd;
    ssize_t nread;
    char host[NI_MAXHOST], service[NI_MAXSERV];

    //VARIÁVEIS de cliente TCP
    //ssize_t nbytes, nleft, nwritten;

    //Se na chamada do programa o numero de argumentos não fôr 3
    if (argc != 3)
    {
        printf("ERRO.\nInicialização inválida, volte a correr o programa!\n");
        exit(0);
    }

    //Guarda o ip e o porto na estrutura
    strcpy(servidor->ipe, argv[1]);
    printf("IP: %s\n", servidor->ipe);
    strcpy(servidor->porto, argv[2]);
    printf("Porto: %s\n", servidor->porto);

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
        counter = select(maxfd + 1, &rfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);

        printf("counter: %d\n", counter);

        if (counter <= 0)
        {
            printf("Erro no select");
            exit(1);
        } /*error*/

        // Se há input para ler
        //É dentro deste if que se lê o input do utilizador
        if (FD_ISSET(0, &rfds)) //o 0 está reservado para o teclado
        {
            //printf("Insira um comando:\n");
            //fflush(stdout);
            //scanf("%s", comando);
            if (fgets(comando, 128, stdin) == NULL)
            {
                //do something
            } //receber input do teclado

            //strcpy(comandofull, comando);

            //Dividir as mensagens
            token = strtok(comando, s); //procurar no input onde está o espaço
            //printf("comando: %s\n", comando); //fica sempre com o inicio
            //printf("token: %s\n", token);     //é o que avança

            if (strcmp(comando, "new") == 0)
            {
                printf("Escolheu: new\n");
                token = strtok(NULL, s); //é o token que vai lendo as coisas SEGUINTES
                //printf("token %d: %s\n", i, token);

                servidor->key = atoi(token);
                //printf("A chave escolhida é: %d\n", servidor->key);

                servidor->next->key = atoi(token);
                //printf("A chave do sucessor é: %d\n", servidor->next->key);
                strcpy(servidor->next->ipe, argv[1]);
                //printf("Sucessor ip: %s\n", servidor->next->ipe);
                strcpy(servidor->next->porto, argv[2]);
                //printf("Sucessor porto: %s\n", servidor->next->porto);

                servidor->next2->key = atoi(token);
                //printf("A chave do sucessor 2 é: %d\n", servidor->next2->key);
                strcpy(servidor->next2->ipe, argv[1]);
                //printf("Sucessor 2 IP: %s\n", servidor->next2->ipe);
                strcpy(servidor->next2->porto, argv[2]);
                //printf("Sucessor 2 porto: %s\n", servidor->next2->porto);
            }
            else if (strcmp(comando, "entry") == 0)
            {
                printf("Escolheu: entry\n");

                token = strtok(NULL, s); //não é preciso guardar, só é preciso passar à frente
                servidor->key = atoi(token);
                printf("A chave escolhida é: %d\n", servidor->key);

                token = strtok(NULL, s);
                //key_entry = atoi(token);

                token = strtok(NULL, s);
                strcpy(ipe_entry, token);

                token = strtok(NULL, s);
                token[strlen(token) - 1] = '\0';
                strcpy(porto_entry, token);

                if (snprintf(mensagem, 130, "EFND %d\n", servidor->key) == -1)
                    exit(1);

                sendmessageUDP(udpfd, ipe_entry, porto_entry, mensagem);
            }
            else if (strcmp(comando, "sentry") == 0)
            {
                printf("Escolheu: sentry\n");

                token = strtok(NULL, s); //não é preciso guardar, só é preciso passar à frente
                servidor->key = atoi(token);
                printf("A chave escolhida é: %d\n", servidor->key);

                token = strtok(NULL, s);
                servidor->next->key = atoi(token);
                printf("A chave do sucessor é: %d\n", servidor->next->key);

                token = strtok(NULL, s);
                strcpy(servidor->next->ipe, token);
                printf("Sucessor ip: %s\n", servidor->next->ipe);

                token = strtok(NULL, s);
                token[strlen(token) - 1] = '\0';
                strcpy(servidor->next->porto, token);
                printf("Sucessor porto: %s\n", servidor->next->porto);

                sfd = create_TCP(servidor->next->ipe, servidor->next->porto);

                if (snprintf(mensagem, 130, "NEW %d %s %s\n", servidor->key, servidor->ipe, servidor->porto) == -1)
                    exit(1);
                printf("A MSG É: %s\n", mensagem);
                sendmessageTCP(sfd, mensagem);
                fprintf(stderr, "I will be printed immediately\n");

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

                servidor->key = 0;

                servidor->next->key = 0;
                strcpy(servidor->next->ipe, " ");
                strcpy(servidor->next->porto, " ");

                servidor->next2->key = 0;
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
                printf("Sessão com predecessor -> pre_fd= %d\n", pre_fd);
            }
            else if (strcmp(comando, "find") == 0)
            {
                printf("Escolheu: find\n");

                token = strtok(NULL, s);
                key_k = atoi(token);

                if (servidor->key == servidor->next->key)
                {
                    if (write(1, "A chave que procura está no servidor em que se encontra\n", 58) == -1)
                        ;
                }
                else if (distance(key_k, servidor->next->key) > distance(key_k, servidor->key))
                {
                    if (snprintf(mensagem, 130, "FND %d %d %s %s", key_k, servidor->key, servidor->ipe, servidor->porto) == -1)
                        exit(1);
                    sendmessageTCP(suc_fd, mensagem);
                }
                else
                {
                    printf("A chave %d foi encontrada\n", key_k);
                    printf("Encontra-se no servidor %d\nIP: %s\nPorto: %s", servidor->next->key, servidor->next->ipe, servidor->next->porto);
                    printf("\n");
                }
                //pensar se é preciso algum else
            }
            else if (strcmp(comando, "exit\n") == 0)
            {
                printf("Escolheu: exit\n");
                close(fd);
                close(afd);
                //TEMOS QUE FAZER FREES?
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
            else
            {
                //close(newfd);
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
                    printf("TEMOS BARRA N");
                    printf("\n");
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

                //ptr = &buffer[0];
                /*
                while (n > 0)
                { 
                    if ((nw = write(suc_fd, ptr, n)) <= 0) //error
                        exit(1);
                    n -= nw;
                    ptr += nw;
                }
                */
                if (write(1, "Mensagem tcp recebida: ", 24) == -1)
                    ;
                if (write(1, buffer, strlen(buffer)) == -1)
                    ;

                if (ler_suc == 0)
                {
                    strcpy(buffer_full, buffer);
                    token = strtok(buffer, s); //procurar no input onde está o espaço
                    printf("buffer: %s\n", buffer);
                    printf("token: %s\n", token);

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

                        token = strtok(NULL, s); //não é preciso guardar, só é preciso passar à frente
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

            if (((n = read(pre_fd, buffer_temp, 130)) != 0))
            {

                if (n == -1)
                {
                    exit(1);
                }

                if (strtok(buffer_temp, c) != NULL)
                {
                    printf("TEMOS BARRA N");
                    printf("\n");
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

                if (ler_pre == 0)
                {
                    strcpy(buffer_full, buffer); // guarda a mensagem completa
                    token = strtok(buffer, s);   //procurar no input onde está o espaço

                    if (strcmp(buffer, "FND") == 0)
                    {
                        token = strtok(NULL, s);
                        key_k = atoi(token);

                        if (distance(key_k, servidor->next->key) > distance(key_k, servidor->key))
                        {
                            //suc_fd = create_TCP(servidor->next->ipe, servidor->next->porto); //É NECESSÁRIO? Nop, não é
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
            //n = sendto(udpfd, buffer, nread, 0, (struct sockaddr *)&addr, addrlen);
            //if (n == -1) /*error*/
            //   exit(1);

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
            printf("buffer: %s\n", buffer);
            printf("token: %s\n", token);

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
                    if (snprintf(mensagem, 130, "FND %d %d %s %s", key_k, servidor->key, servidor->ipe, servidor->porto) == -1)
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

                token = strtok(NULL, s); //não é preciso guardar, só é preciso passar à frente
                servidor->key = atoi(token);
                printf("A chave escolhida é: %d\n", servidor->key);

                token = strtok(NULL, s);
                servidor->next->key = atoi(token);
                printf("A chave do sucessor é: %d\n", servidor->next->key);

                token = strtok(NULL, s);
                strcpy(servidor->next->ipe, token);
                printf("Sucessor ip: %s\n", servidor->next->ipe);

                token = strtok(NULL, s);
                token[strlen(token) - 1] = '\0'; // REMOVE O \n DO FIM
                strcpy(servidor->next->porto, token);
                printf("Sucessor porto: %s\n", servidor->next->porto);

                sfd = create_TCP(servidor->next->ipe, servidor->next->porto);

                if (snprintf(mensagem, 130, "NEW %d %s %s\n", servidor->key, servidor->ipe, servidor->porto) == -1)
                    exit(1);
                printf("A MSG É: %s\n", mensagem);
                sendmessageTCP(sfd, mensagem);

                //Guarda informação de ligação com o sucessor
                suc_fd = sfd;
                ligacao->sucessor = 1;
            }
        }

        //Receber mensagens de um servidor novo
        if (FD_ISSET(afd, &rfds)) //MENSAGENS DE FORA
        {
            //Torna o server non-blocking
            //flags = fcntl(afd, F_GETFL, 0);
            //fcntl(afd, F_SETFL, flags | O_NONBLOCK);

            printf("afd: %d\n", afd);

            if ((n = read(afd, buffer_temp, 130)) != 0)
            {
                if (n == -1)
                {
                    fprintf(stderr, "ERRO: %s\n", gai_strerror(n));
                    exit(1);
                }

                if (strtok(buffer_temp, c) != NULL)
                {
                    printf("TEMOS BARRA N");
                    printf("\n");
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

                //ptr = &buffer[0];
                /*
                while (n > 0)
                {
                    if ((nw = write(afd, ptr, n)) <= 0) //error
                        exit(1);
                    n -= nw;
                    ptr += nw;
                }
                */

                if (write(1, "Mensagem tcp recebida: ", 24) == -1)
                    ;
                if (write(1, buffer, strlen(buffer)) == -1)
                    ;

                if (ler_nova == 0)
                {
                    //é a partir daqui que se INTERPRETA a mensagem

                    strcpy(buffer_full, buffer); //salvar a mensagem original
                    token = strtok(buffer, s);   //procurar no input onde está o espaço
                    printf("buffer: %s\n", buffer);
                    printf("token: %s\n", token);

                    //token = strtok(NULL, s); //é o token que vai lendo as coisas SEGUINTES

                    ligacao->nova = 0; // se já lemos tudo da ligacao nova, fechamos o descritor

                    if (strcmp(buffer, "NEW") == 0)
                    {

                        if (servidor->key == servidor->next->key) //Se houver só um servidor no anel
                        {

                            token = strtok(NULL, s); //não é preciso guardar, só é preciso passar à frente (duvida, confirmar)
                            servidor->next->key = atoi(token);

                            token = strtok(NULL, s);
                            strcpy(servidor->next->ipe, token);
                            printf("Sucessor ip: %s\n", servidor->next->ipe);

                            token = strtok(NULL, s);
                            strcpy(servidor->next->porto, token);
                            printf("Sucessor porto: %s\n", servidor->next->porto);

                            ligacao->predecessor = 1;
                            pre_fd = afd;
                            afd = -2;

                            //Diz a quem entrou quem é o seu duplo sucessor
                            //Que neste caso vai ser ele próprio (por isso tem o next)
                            if (snprintf(mensagem, 130, "SUCC %d %s %s\n", servidor->next->key, servidor->next->ipe, servidor->next->porto) == -1)
                                exit(1);
                            printf("mensagemm enviada: %s", mensagem);
                            sendmessageTCP(pre_fd, mensagem);

                            ligacao->sucessor = 1;
                            suc_fd = create_TCP(servidor->next->ipe, servidor->next->porto);

                            //snprintf(mensagem, 130, "SUCCCONF\n");
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
                            printf("mensagem enviada (NOVO ELSE): %s", mensagem);
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
    //while(1)

    close(afd);
    close(fd);
    exit(0);
}
