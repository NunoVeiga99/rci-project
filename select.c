#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>




/* ... */
#define max(A, B) ((A) >= (B) ? (A) : (B))
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
    /*...*/
    /*fd=socket(…);bind(fd,…);listen(fd,…);*/
    state = idle;
    while (1)
    {
        FD_ZERO(&rfds);
        //input do utilizador 
        FD_SET(0,&rfds);
        FD_SET(fd, &rfds);
        maxfd = fd;
        if (state == busy)
        {
            FD_SET(afd, &rfds);
            maxfd = max(maxfd, afd);
        }
        counter = select(maxfd + 1, &rfds,
                         (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
        if (counter <= 0) /*error*/
            exit(1);
            
        if(FD_ISSET(0, &rfds)){


        }
        
        
        if (FD_ISSET(fd, &rfds))
        {
            addrlen = sizeof(addr);
            if ((newfd = accept(fd, (struct sockaddr *)&addr, &addrlen)) == -1) /*error*/
                exit(1);
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
