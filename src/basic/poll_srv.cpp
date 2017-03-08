//
// Created by zhuanxu on 17/3/7.
//


#include <sys/socket.h>
#include <netinet/in.h> //IPPROTO_TCP
#include <cstdlib>
#include <cstdio>
#include <strings.h>
#include <poll.h>
#include <vector>
#include <sys/errno.h>
//#include <zconf.h>
#include <fcntl.h>
#include <iostream>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

using namespace std;

using PollFdList = vector<struct pollfd>;


#define SERVPORT 6000 //设定服务器服务端口为6000

#define ERR_EXIT(m) \
    do {\
        perror(m); \
        exit(EXIT_FAILURE); \
    }while(0)

int main() {
    // SIGPIPE的产生，当另一方调用close关闭后，我们在socket里写，对端会返回RST
    // 此时我们如果再次往里写，则会发生SIGPIPE，默认的方式是退出程序
    // 因此我们需要忽略该信号
    signal(SIGPIPE, SIG_IGN);
    // 防止出现僵尸进程
    signal(SIGCHLD, SIG_IGN);

    int idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);

    // int domain, int type, int protocol
    // 设置 SOCK_NONBLOCK 非阻塞, SOCK_CLOEXEC fock 出的子进程做替换的时候，关闭打开的描述文件符，而不是继承
    int listen_fd = socket(PF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (listen_fd < 0){
        ERR_EXIT("套接字创建失败!");
    }
    // set socket address reused
    // int setsockopt(int socket, int level, int option_name,
    // const void *option_value, socklen_t option_len);
    int err,sock_reuse=1;
    // SO_REUSEADDR : 设置地址复用
    // [Linux下getsockopt/setsockopt 函数说明](http://blog.csdn.net/xioahw/article/details/4056514)
    err = setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&sock_reuse, sizeof(sock_reuse));
    if (err < 0){
        ERR_EXIT("套接字可重用设置失败!");
    }
    //绑定套接字
    struct sockaddr_in srv_addr;
//    memset(&srv_addr,0, sizeof(srv_addr));
    bzero(&srv_addr,sizeof(srv_addr));
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERVPORT);

    err = ::bind(listen_fd,(struct sockaddr *)&srv_addr, sizeof(srv_addr));
    if (err < 0){
        ERR_EXIT("绑定失败!");
    }
    // 监听
    // SOMAXCONN 最大的accept队列
    if (listen(listen_fd,SOMAXCONN)<0){
        ERR_EXIT("设置监听失败!");
    }
    // 设置poll的read事件
    struct pollfd pfd;
    pfd.fd = listen_fd;
    pfd.events = POLL_IN;

    PollFdList pollfds;
    pollfds.push_back(pfd);
    // 无限大循环
    int nready = 0;

    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    int connfd;

    while (1){
        // -1 no time out
        nready = poll(pollfds.data(),pollfds.size(),-1);
        if (nready == -1){
            if (errno == EINTR){
                continue;
            }
            ERR_EXIT("poll失败");
        }
        if (nready == 0){// nothing happended
            continue;
        }

        if (pollfds[0].revents & POLLIN)
        {
            peerlen = sizeof(peeraddr);
            // 相比于accpet能够支持 SOCK_NONBLOCK | SOCK_CLOEXEC 两个参数
            connfd = ::accept4(listen_fd, (struct sockaddr*)&peeraddr,
                             &peerlen, SOCK_NONBLOCK | SOCK_CLOEXEC);

            /*
             if (connfd == -1){
                ERR_EXIT("accept4");
             }
             */
            // 正确的处理方式，优雅的关闭
            if (connfd == -1)
            {
                if (errno == EMFILE)
                {
                    close(idlefd);
                    idlefd = accept(listen_fd, NULL, NULL);
                    close(idlefd);
                    idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
                    continue;
                }
                else
                    ERR_EXIT("accept4失败");
            }

            pfd.fd = connfd;
            pfd.events = POLLIN;
            pfd.revents = 0;
            pollfds.push_back(pfd);
            --nready;


            std::cout<<"ip="<<inet_ntoa(peeraddr.sin_addr)<<
                     " port="<<ntohs(peeraddr.sin_port)<<std::endl;
            if (nready == 0)
                continue;
        }

        //std::cout<<pollfds.size()<<std::endl;
        //std::cout<<nready<<std::endl;
        for (PollFdList::iterator it=pollfds.begin()+1;
             it != pollfds.end() && nready >0; ++it)
        {
            if (it->revents & POLLIN)
            {
                --nready;
                connfd = it->fd;
                char buf[1024] = {0};
                int ret = read(connfd, buf, 1024);
                if (ret == -1)
                    ERR_EXIT("read");
                if (ret == 0) // 对方已关闭
                {
                    std::cout<<"client close"<<std::endl;
                    it = pollfds.erase(it);
                    --it;

                    close(connfd);
                    continue;
                }

                std::cout<<buf;
                write(connfd, buf, strlen(buf));

            }
        }
    }
}