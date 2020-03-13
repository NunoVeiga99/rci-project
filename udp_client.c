#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<unistd.h>
#include<stdio.h>

int main(void)
{

struct addrinfo hints,*res;
int fd,errcode;
ssize_t n;
struct sockaddr_in addr;
socklen_t addrlen;
char buffer[128];
char host[NI_MAXHOST],service[NI_MAXSERV];//consts in <netdb.h>


fd=socket(AF_INET,SOCK_DGRAM,0);//UDP socket
if(fd==-1)/*error*/exit(1);

memset(&hints,0,sizeof hints);
hints.ai_family=AF_INET;//IPv4
hints.ai_socktype=SOCK_DGRAM;//UDP socket

errcode=getaddrinfo("tejo.tecnico.ulisboa.pt","58001",&hints,&res);
if(errcode!=0)/*error*/exit(1);
n=sendto(fd,"Hello!\n",7,0,res->ai_addr,res->ai_addrlen);
if(n==-1)/*error*/exit(1);

addrlen=sizeof(addr);
n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
if(n==-1)/*error*/exit(1);

write(1,"echo: ",6);//stdout
write(1,buffer,n);

if((errcode=getnameinfo((struct sockaddr *)&addr,addrlen,host,sizeof host,service,sizeof service,0))!=0)
fprintf(stderr,"error: getnameinfo: %s\n",gai_strerror(errcode));
else
printf("sent by [%s:%s]\n",host,service);


freeaddrinfo(res);
close(fd);
exit(0);


}
