#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <poll.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <vector>
#include <iostream>

#define ERR_EXIT(m)\
    do \
    { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while(0)

/* 定义了一个struct pollfd类型的向量 */
typedef std::vector<struct pollfd> PollFdList;

int main(void)
{

    printf("eg: nc 127.0.0.1 5188\n");

    signal(SIGPIPE, SIG_IGN);   //屏蔽SIGPIPE
    signal(SIGCHLD, SIG_IGN);   //避免僵死进程

    //int idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
    int listenfd;

    //if ((listenfd = socket(PE_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    if ((listenfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)) < 0)
        ERR_EXIT("socket");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(5188);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    int on = 1;
    /* 设置地址重复利用 */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        ERR_EXIT("setsockopt");

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        ERR_EXIT("bind");
    if (listen(listenfd, SOMAXCONN) < 0)
        ERR_EXIT("listen");

    struct pollfd pfd;
    pfd.fd = listenfd;  /* 监听的套接字 */
    pfd.events = POLLIN;    /* 监听读事件 */

    PollFdList pollfds;     /* 定义了一个struct pollfd 套接字*/
    pollfds.push_back(pfd); /* 把pfd放到pollfds向量中去 */

    int nready = 0;
    
    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    int connfd;

    while (1) {
        /* &*pollfds.begin数组的首地址  pollfds.size()数组的大小*/
        nready = poll(&*pollfds.begin(), pollfds.size(), -1);
        if (nready == -1) {
            if (errno == EINTR)
                continue;
            ERR_EXIT("poll");
        }
        if (nready == 0)    // nothing happended
            continue;

        if (pollfds[0].revents & POLLIN) {  /* 如果监听套接字触发的是读事件 */
            peerlen = sizeof(peeraddr);
                    /* accpet4这个函数可以设置返回文件描述符的属性 */
            connfd = accept4(listenfd, (struct sockaddr*)&peeraddr, 
                        &peerlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (connfd == -1)
                ERR_EXIT("accpet4");
/*
            if (connfd == -1) {
                if (errno == EMFILE) {
                    close(idlefd);
                    idlefd = accpet(listedfd, NULL, NULL);
                    close(idlefd);
                    idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
                    continue;
                } else 
                    ERR_EXIT("accpet4");
            } 
*/
            pfd.fd = connfd;	/* 把监听到的文件描述符放到poll结构体中 */
            pfd.events = POLLIN;	/* 设置要监听的事件 */
            pfd.revents = 0;	/* 把之前的清空 */
            pollfds.push_back(pfd);	/* 把这个poll结构体加入到要监听的结构体中 */
            --nready;
            //连接成功
            std::cout << "ip=" << inet_ntoa(peeraddr.sin_addr) << 
                " port=" << ntohs(peeraddr.sin_port) << std::endl;
                	
             if (nready == 0) 
             	continue;
        }

        std::cout << pollfds.size() << std::endl;
        std::cout << nready << std::endl;
        /* 通过迭代器遍历剩余的动态数组,
        	终止条件,遍历结束或者等待读取的文件描述符没有了 */
        for (PollFdList::iterator it = pollfds.begin() + 1;
                it != pollfds.end()&&nready > 0; it++) {
            
            if (it->revents & POLLIN){
                --nready;
                connfd = it->fd;
                char buf[1024] = {0};
                int ret = read(connfd, buf, 1024);
                if (ret == -1)
                    ERR_EXIT("read");
                if (ret == 0) {
                    std::cout << "client close" << std::endl;
                    it = pollfds.erase(it);	//把这个文件描述符内容读完,然后把这个fd删除
                    --it;

                    close(connfd);
                    continue;
                }
                std::cout << buf;
                /* 反射回去 */
                write(connfd, buf, strlen(buf));
            }
        }
    }

    return 0;
}

