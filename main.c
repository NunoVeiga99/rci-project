
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
        fprintf(stderr, "ERRO1: %s\n", gai_strerror(n));
        exit(1);
    }

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*error*/
    {
        fprintf(stderr, "ERRO2 connect: %s\n", gai_strerror(n));
        exit(1);
    }

    return fd;
}

/*-----------------------------------------
Função sendmessageTCP
Envia uma mensagem TCP
-----------------------------------------*/
void sendmessageTCP(int fd, char message[128])
{

    ssize_t nbytes, nleft, nwritten, nread;
    char *ptr, buffer[128];

    ptr = strcpy(buffer, message);
    nbytes = strlen(buffer);

    nleft = nbytes;
    while (nleft > 0)
    {
        nwritten = write(fd, ptr, nleft); //manda a mensagem
        if (nwritten <= 0){
            fprintf(stderr, "ERRO1:\n");
           exit(1); 
        }                /*error*/
        nleft -= nwritten;
        ptr += nwritten;
    }
    nleft = nbytes;
    ptr = buffer;
    while (nleft > 0)
    {
        nread = read(fd, ptr, nleft); //Recebe o eco
        if (nread == -1){
            fprintf(stderr, "ERRO2 nread\n");
            exit(1);
        }          /*error*/
            
        else if (nread == 0)
            break; //closed by peer
        nleft -= nread;
        ptr += nread;
    }
    nread = nbytes - nleft;
    write(1, "echo: ", 6); //stdout
    write(1, buffer, nread);
    write(1, "\n", 2);
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

    int fd, newfd, sfd, afd;
    int suc_fd = 0;
    int pre_fd = 0;
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

    const char s[2] = " "; //para procurar por espaços, usado no menu
    char *token;           //para guardar numa string o resto da string que se está a usar (menu)
    char c = ' ';

    //VARIÁVEIS do servidor TCP
    struct addrinfo hints, *res;
    int errcode;
    ssize_t n, nw;
    struct sockaddr_in addr;
    socklen_t addrlen;
    char *ptr, buffer[128], buffer_full[128];
    char *send, message[128];

    //VARIÁVEIS do comandoos de utilização ou iterações
    char comando[128];     //guarda o comando inserido pelo utilizador
    char comandofull[128]; //guarda o comando inserido pelo utilizador e NÃO O ALTERA
    int i = 0;
    int key = 0;
    int count = 0; //conta o num. de espaços no menu (entry e sentry)

    char mensagem[128];

    //VARIÁVEIS do servidor udp
    struct addrinfo udphints, *udpres;
    int udpfd;
    ssize_t nread;

    //VARIÁVEIS de cliente TCP
    ssize_t nbytes, nleft, nwritten;

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
        memset(buffer,0,sizeof(buffer));

        n = 0;
        //"limpa" os descritores
        FD_ZERO(&rfds);

        //iniciaiza os descritores que estão sempre on
        FD_SET(0, &rfds);       //teclado
        FD_SET(fd, &rfds);      //espera TCP
        FD_SET(udpfd, &rfds);       //espera UDP
        maxfd = max(fd, udpfd);     //vê o maximo dos fd

        if (ligacao->nova == 1)
        {
            FD_SET(afd, &rfds);
            maxfd = max(maxfd, afd);
        }
        if(ligacao->sucessor == 1){
            FD_SET(suc_fd,&rfds);
            maxfd = max(maxfd, suc_fd);
        }
        if(ligacao->predecessor == 1){
            FD_SET(pre_fd,&rfds);
            maxfd = max(maxfd, pre_fd);
        }

        // SELECT
        counter = select(maxfd + 1, &rfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);

        printf("counter: %d\n", counter);

        if (counter <= 0) /*error*/
            exit(1);

        // Se há input para ler
        //É dentro deste if que se lê o input do utilizador
        if (FD_ISSET(0, &rfds)) //o 0 está reservado para o teclado
        {
            //printf("Insira um comando:\n");
            //fflush(stdout);
            //scanf("%s", comando);
            fgets(comando, 128, stdin); //receber input do teclado

            //strcpy(comandofull, comando);

            //Dividir as mensagens
            token = strtok(comando, s);       //procurar no input onde está o espaço
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
                token[strlen(token)-1] = '\0';
                strcpy(servidor->next->porto, token);
                printf("Sucessor porto: %s\n", servidor->next->porto);

                sfd = create_TCP(servidor->next->ipe, servidor->next->porto);

                snprintf(mensagem, 512, "NEW %d %s %s", servidor->key, servidor->ipe, servidor->porto);
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

        //Canal com sucessor
        if (FD_ISSET(suc_fd, &rfds))        //MENSAGEM DO SUCESSOR
        {
            if ((n = read(suc_fd, buffer, 128)) != 0)
            {
                if (n == -1) //error
                    exit(1);
                ptr = &buffer[0];
                while (n > 0)
                {
                    if ((nw = write(suc_fd, ptr, n)) <= 0) //error
                        exit(1);
                    n -= nw;
                    ptr += nw;
                }
                write(1, "Mensagem tcp recebida: ", 24);
                write(1, buffer, strlen(buffer));

                strcpy(buffer_full, buffer);
                token = strtok(buffer, s); //procurar no input onde está o espaço
                printf("buffer: %s\n", buffer);
                printf("token: %s\n", token);

                if (strcmp(buffer, "SUCC") == 0)
                {
                    token = strtok(NULL, s); //não é preciso guardar, só é preciso passar à frente (duvida, confirmar)
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

                    suc_fd = create_TCP(servidor->next->ipe,servidor->next->porto); 

                    sendmessageTCP(suc_fd, "SUCCONF\n"); 

                    snprintf(mensagem, 512, "SUCC %d %s %s", servidor->next->key, servidor->next->ipe, servidor->next->porto);
                    sendmessageTCP(pre_fd, mensagem);

                    

                }
            }
        }

        if(FD_ISSET(pre_fd,&rfds)){

        }

        //Conexão TCP
        if (FD_ISSET(fd, &rfds)) //receber TCP de alguem desconhecido
        {
            addrlen = sizeof(addr);

            //Aceita a conexão
            if ((newfd = accept(fd, (struct sockaddr *)&addr, &addrlen)) == -1) /*error*/
                exit(1);

            
            if(ligacao->nova == 0){
                 afd = newfd;
                 ligacao->nova = 1;
                 //break;
            }else{
                close(newfd);
                //break;
            }    
        }

        //Conexão UDP
        if (FD_ISSET(udpfd, &rfds)) //receber UDP de alguem desconhecido
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


        //Receber mensagens de um servidor novo
        if (FD_ISSET(afd, &rfds))       //MENSAGENS DE FORA
        {
            printf("afd: %d\n", afd);
            
            if ((n = read(afd, buffer, 128)) != 0)
            {
                if (n == -1)
                {
                    fprintf(stderr, "ERRO: %s\n", gai_strerror(n));
                    exit(1);
                }

                ptr = &buffer[0];
                while (n > 0)
                {
                    if ((nw = write(afd, ptr, n)) <= 0) /*error*/
                        exit(1);
                    n -= nw;
                    ptr += nw;
                }

                write(1, "Mensagem tcp recebida: ", 24);
                write(1, buffer, strlen(buffer));

                //é a partir daqui que se INTERPRETA a mensagem

                strcpy(buffer_full, buffer); //salvar a mensagem original
                token = strtok(buffer, s);   //procurar no input onde está o espaço
                printf("buffer: %s\n", buffer);
                printf("token: %s\n", token);

                //token = strtok(NULL, s); //é o token que vai lendo as coisas SEGUINTES

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

                        //Diz a quem entrou quem é o seu duplo sucessor
                        //Que neste caso vai ser ele próprio (por isso tem o next)
                        snprintf(mensagem, 512, "SUCC %d %s %s", servidor->next->key, servidor->next->ipe, servidor->next->porto);
                        printf("mensagemm enviada: %s", mensagem);
                        sendmessageTCP(pre_fd, mensagem);

                        suc_fd = create_TCP(servidor->next->ipe,servidor->next->porto); 

                        //snprintf(mensagem, 512, "SUCCCONF\n");
                        sendmessageTCP(suc_fd, "SUCCONF\n");
                    }
                    else
                    {                    
                        sendmessageTCP(pre_fd,buffer_full);

                        pre_fd = afd;

                        //Diz a quem entrou quem é o seu duplo sucessor
                        //Que neste caso vai ser ele próprio (por isso tem o next)
                        snprintf(mensagem, 512, "SUCC %d %s %s", servidor->next->key, servidor->next->ipe, servidor->next->porto);
                        printf("mensagem enviada (NOVO ELSE): %s", mensagem);
                        sendmessageTCP(pre_fd, mensagem);

                    }
                }
                else if (strcmp(buffer, "SUCCONF\n") == 0)
                {
                    ligacao->predecessor = 1;       //significa que já tenho um predecessor
                    pre_fd = afd;
                }
            }
            else
            {
                close(afd);
                //ligacao->sucessor = 0;
            } //connection closed by peer
        }

    } //while(1)

    
    
    close(afd);
    close(fd);
    exit(0);
}
