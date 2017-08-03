#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>

#define IP   "127.0.0.1"
#define PORT        8787
#define MAXSIZE     1024
#define LISTENQ     5
#define FDSIZE      1000
#define EPOLLEVENTS 100

//IO��·����epoll
static void do_epoll(int listenfd);
//�¼�������
static void handle_events(int epollfd,struct epoll_event *events,int num,int listenfd,char *buf);
//������յ�������
static void handle_accpet(int epollfd,int listenfd);
//������
static void do_read(int epollfd,int fd,char *buf);
//д����
static void do_write(int epollfd,int fd,char *buf);
//����¼�
static void add_event(int epollfd,int fd,int state);
//�޸��¼�
static void modify_event(int epollfd,int fd,int state);
//ɾ���¼�
static void delete_event(int epollfd,int fd,int state);

int main(int argc,char *argv[])
{
    int  listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if (listenfd == -1)
    {
        perror("socket error:");
        exit(1);
    }

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,IP,&servaddr.sin_addr);
    servaddr.sin_port = htons(PORT);

    //�󶨽ӿ�
    if (bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        perror("bind error: ");
        exit(1);
    }

    //�����˿�
    if (listen(listenfd,LISTENQ) == -1)
    {
        perror("listen error: ");
        exit(1);
    }
    do_epoll(listenfd);    //ִ��epoll
    return 0;
}

static void do_epoll(int listenfd)
{
    int i,fd,epollfd;
    struct epoll_event events[EPOLLEVENTS];
    int ret;
    char buf[MAXSIZE];
    memset(buf,0,MAXSIZE);

  &nbsp; //����һ��������
    epollfd = epoll_create(FDSIZE);

   &nbsp;//��Ӽ����������¼�
    add_event(epollfd,listenfd,EPOLLIN);
    for ( ; ; )
    {
        //��ȡ�Ѿ�׼���õ��������¼�
        printf("waiting on epoll_wait...\n");
        ret = epoll_wait(epollfd,events,EPOLLEVENTS,1000);
        if (ret < 0)
        {
              perror("epoll_wait() error\n");
              break;
        }
        if (ret==0)
        {
              printf("epoll_wait time out.\n");
              break;
        }
        for (i = 0;i < ret;i++)
        {
            fd = events[i].data.fd;
            //���������������ͺ��¼����ͽ��д���
            if ((fd == listenfd) &&(events[i].events & EPOLLIN))   //��������
                  handle_accpet(epollfd,listenfd);
            else if (events[i].events & EPOLLIN)     //���¼�
                  do_read(epollfd,fd,buf);
            else if (events[i].events & EPOLLOUT)    //д�¼�
                  do_write(epollfd,fd,buf);
        }
    }
    close(epollfd);
}

static void handle_accpet(int epollfd,int listenfd)
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
        //���һ���ͻ����������¼�
        add_event(epollfd,new_fd,EPOLLIN);
    }
}

static void do_read(int epollfd,int fd,char *buf)
{
    int nread;
    nread = read(fd,buf,MAXSIZE);
    if (nread == -1)
    {
        perror("read error.\n");
        //�رղ�ɾ��ע����ļ�������
&nbsp;       close(fd);
        delete_event(epollfd,fd,EPOLLIN);
    }

    else if (nread == 0)
    {
        printf("client close.\n");
        close(fd);
        delete_event(epollfd,fd,EPOLLIN);
    }
    else
    {
        printf("receive message: %s\n",buf);
        //�޸���������Ӧ���¼����ɶ���Ϊд
        modify_event(epollfd,fd,EPOLLOUT|EPOLLET);
    }
}

static void do_write(int epollfd,int fd,char *buf)
{
    int nwrite;
    nwrite = write(fd,buf,strlen(buf));
    if (nwrite == -1)
    {
        perror("write error:");
        close(fd);
        delete_event(epollfd,fd,EPOLLOUT);
    }
    else
    {
&nbsp;       //�޸��¼�Ϊ��
        modify_event(epollfd,fd,EPOLLIN|EPOLLET);
    }
    memset(buf,0,MAXSIZE);
}


static void add_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
}

static void delete_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,&ev);
}

static void modify_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev);
}