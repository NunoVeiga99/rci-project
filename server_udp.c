#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<unistd.h>

int main(void)
{
struct addrinfo udphints,*udpres;
int udpfd,errcode;
struct sockaddr_in addr;
socklen_t addrlen;
ssize_t n,nread;
char buffer[128];

if((udpfd=socket(AF_INET,SOCK_DGRAM,0))==-1)exit(1);//error
memset(&udphints,0,sizeof udphints);
udphints.ai_family=AF_INET;//IPv4
udphints.ai_socktype=SOCK_DGRAM;//UDP socket
udphints.ai_flags=AI_PASSIVE;
if((errcode=getaddrinfo(NULL,"58001",&udphints,&udpres))!=0)/*error*/exit(1);

if(bind(udpfd,udpres->ai_addr,udpres->ai_addrlen)==-1)/*error*/exit(1);
while(1){addrlen=sizeof(addr);
nread=recvfrom(udpfd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
if(nread==-1)/*error*/exit(1);
n=sendto(udpfd,buffer,nread,0,(struct sockaddr*)&addr,addrlen);
if(n==-1)/*error*/exit(1);}

freeaddrinfo(udpres);
close(udpfd);
exit(0);

}