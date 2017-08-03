#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>

#define IP   "127.0.0.1"
#define PORT        8787
#define MAXSIZE     1024
#define LISTENQ     5

//IO????select
static void do_select(int listenfd);
//????????
static void handle_accpet(int fd);
//???
static void do_read(int fd,char *buf);
//???
static void do_write(int fd,char *buf);
//???????fd???? max_fd
static void clr_fd(int fd);

fd_set select_set; //??????????????????
int max_fd;  //????????

int main(int argc,char *argv[])
{
    int  listenfd;
    struct sockaddr_in servaddr;
    if ((listenfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        perror("socket error:");
        exit(1);
    }
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,IP,&servaddr.sin_addr);
    servaddr.sin_port = htons(PORT);
    if (bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        perror("bind error: ");
        exit(1);
    }
    if (listen(listenfd,LISTENQ) == -1)
    {
  	perror("listen");
  	exit(1);
    }
 
    do_select(listenfd);
    return 0;
}

static void do_select(int listenfd)
{
 	fd_set working_set;	//??select????????????
 	struct timeval timeout;
 
 	//init
 	FD_ZERO(&select_set);
 	max_fd=listenfd;
 	FD_SET(listenfd, &select_set);
 
 	timeout.tv_sec = 30;
 	timeout.tv_usec = 0;
 	int ready;
 	int fd;
 	char buf[MAXSIZE];
 	memset(buf,0,MAXSIZE);
 
 	while(1)
 	{
  	// Copy select_set into working_set
  	memcpy(&working_set, &select_set, sizeof(select_set));

  	printf("Waiting on select()...\n");
  	ready = select(max_fd + 1, &working_set, NULL, NULL, &timeout);
        if (ready < 0)
        {
            perror("  select() failed\n");
            break;
        }
        if (ready == 0)
        {
            printf("  select() timed out. End program.\n");
            break;
        }

        // Check each sd in working_set
        for (fd = 0; fd <= max_fd && ready > 0; ++fd)
        {
            // Check to see if this fd is ready
            if (FD_ISSET(fd, &working_set))
            {
                --ready;

                // Check to see if this is a listening fd
                if (fd == listenfd)
                {
                    	printf("  Listeing socket is readable\n");
     			handle_accpet(fd);
                }
                // This is not the listening sd
                else
                {
     			//read and write,close fd
     			do_read(fd, buf);
     			do_write(fd,buf);
     			clr_fd(fd);
                }
            }
        }
    }
}

static void handle_accpet(int listenfd)
{
    int new_fd;
    struct sockaddr_in cliaddr;
    socklen_t  cliaddrlen;
    new_fd = accept(listenfd,(struct sockaddr*)&cliaddr,&cliaddrlen);
    if (new_fd == -1)
        perror("accpet error:");
    else
    {
  	printf("accept a new client: %s:%d\n",inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port);
  	FD_SET(new_fd, &select_set);
  	if (new_fd > max_fd)
  	{
   		max_fd = new_fd;
  	}
    }
}

static void do_read(int fd,char *buf)
{
    int nread;
    nread = read(fd,buf,MAXSIZE);
    if (nread <= 0)
    {
  	printf("client close\n");
  	//close fd and remove it from select_set
  	clr_fd(fd);
    }
    else
    {
  	printf("receive message : %s\n",buf);
    }
}

static void do_write(int fd,char *buf)
{
    int nwrite;
    nwrite = write(fd,buf,strlen(buf));
    if (nwrite == -1)
    {
        perror("write error:");
  	clr_fd(fd);
    }
}

static void clr_fd(int fd)
{
   close(fd);
   FD_CLR(fd, &select_set);
  
   if (fd == max_fd)
   {
  	// find the new max fd
  	while (!FD_ISSET(max_fd, &select_set))
  	{
   		--max_fd;
    	}
   }
}