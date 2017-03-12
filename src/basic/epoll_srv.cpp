#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/epoll.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <vector>
#include <algorithm>
#include <iostream>

#define SERVPORT 6000 //设定服务器服务端口为6000

typedef std::vector<struct epoll_event> EventList;

#define ERR_EXIT(m) \
        do \
        { \
                perror(m); \
                exit(EXIT_FAILURE); \
        } while(0)

int main(void)
{
    // SIGPIPE的产生，当另一方调用close关闭后，我们在socket里写，对端会返回RST
    // 此时我们如果再次往里写，则会发生SIGPIPE，默认的方式是退出程序
    // 因此我们需要忽略该信号
    signal(SIGPIPE, SIG_IGN);
    // 防止出现僵尸进程
    signal(SIGCHLD, SIG_IGN);

    int idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
    int listenfd;
    //if ((listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    // int domain, int type, int protocol
    if ((listenfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)) < 0)
        ERR_EXIT("socket");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVPORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int on = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        ERR_EXIT("setsockopt");

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        ERR_EXIT("bind");
    if (listen(listenfd, SOMAXCONN) < 0)
        ERR_EXIT("listen");

    std::vector<int> clients;
    int epollfd;
    // EPOLL_CLOEXEC 根 O_CLOEXEC 一样的作用
    epollfd = epoll_create1(EPOLL_CLOEXEC);

    struct epoll_event event;
    // data是一个 union 结构，此处我们存放了 fd
    event.data.fd = listenfd;
    event.events = EPOLLIN/* | EPOLLET*/;
    // int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event);

    EventList events(16); // 初始16个
    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    int connfd;

    int nready;
    while (1)
    {
        // since c++11 it events.data() ==  &*events.begin()
        // int epoll_wait(int epfd, struct epoll_event *events,int maxevents, int timeout);
        nready = epoll_wait(epollfd, events.data(), static_cast<int>(events.size()), -1);
        if (nready == -1)
        {
            if (errno == EINTR)
                continue;

            ERR_EXIT("epoll_wait");
        }
        if (nready == 0)	// nothing happended
            continue;

        if ((size_t)nready == events.size()) // 如果都活跃，则增加一倍的大小
            events.resize(events.size()*2);

        for (int i = 0; i < nready; ++i)
        {
            if (events[i].data.fd == listenfd)
            {
                peerlen = sizeof(peeraddr);
                // accept4 相比较于 accept 可以能够 设置 flags
                connfd = ::accept4(listenfd, (struct sockaddr*)&peeraddr,
                                   &peerlen, SOCK_NONBLOCK | SOCK_CLOEXEC);

                if (connfd == -1)
                {
                    // 文件描述符超限制
                    if (errno == EMFILE)
                    {
                        close(idlefd);
                        idlefd = accept(listenfd, NULL, NULL);
                        close(idlefd);
                        idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
                        continue;
                    }
                    else
                        ERR_EXIT("accept4");
                }


                std::cout<<"ip="<<inet_ntoa(peeraddr.sin_addr)<<
                         " port="<<ntohs(peeraddr.sin_port)<<std::endl;

                clients.push_back(connfd);

                event.data.fd = connfd;
                event.events = EPOLLIN/* | EPOLLET*/;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &event);
            }
            else if (events[i].events & EPOLLIN)
            {
                connfd = events[i].data.fd;
                if (connfd < 0)
                    continue;

                char buf[1024] = {0};
                int ret = read(connfd, buf, 1024);
                if (ret == -1)
                    ERR_EXIT("read");
                if (ret == 0)
                {
                    // 关闭文件描述符
                    std::cout<<"client close"<<std::endl;
                    close(connfd);
                    event = events[i];
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, &event);
                    clients.erase(std::remove(clients.begin(), clients.end(), connfd), clients.end());
                    continue;
                }

                std::cout<<buf;
                write(connfd, buf, strlen(buf));
            }

        }
    }

    return 0;
}