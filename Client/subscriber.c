// Nguyễn Minh Quang - 18021044
// 10.2 Viết chương trình chat cho phép nhiều client có thể chat với nhau sử dụng máy chủ đa luồng
// Đây là phía Client, Nhập input từ bàn phím và giao tiếp với server theo giao thức trong tài liệu kèm theo

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <regex.h>

int Write(int sockfd, const char *, size_t len, bool printMessage);
int Read(int sockfd, char *buf, size_t len, bool printMessage);
#define MAXLINE 4096
#define SERV_PORT 9111

const char *BYE = "500 BYE";

int main(int argc, char **argv)
{

    char receiveBuf[MAXLINE];
    char sendBuf[MAXLINE];

    char host[32];

    int sockfd, nbytes;
    struct sockaddr_in serv_addr;
    if (argc != 2)
    {
        printf("usage: a.out <IPaddress>");
        exit(1);
    }

    strcpy(host, argv[1]);
    printf("host --> %s\n", host);
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket creation error \n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    // serv_addr.sin_addr.s_addr = inet_addr("192.168.0.1");
    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0)
    {
        printf("Invalid address/ Address not supported \n");
        exit(1);
    }
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        exit(1);
    }
    printf("connected\n");

    int epfd, stdinfd, nfds;
    struct epoll_event ev, events[5];
    epfd = epoll_create(10);

    stdinfd = fileno(stdin);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = stdinfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, stdinfd, &ev);

    //add sockfd to epollfd
    ev.data.fd = sockfd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
    while (1)
    {
        nfds = epoll_wait(epfd, events, 5, 1000); //1000milisecond = 1s
        // printf("nfds --> %d", nfds);
        for (int i = 0; i < nfds; i++)
        {
            // this when user enter some string
            if (events[i].data.fd == stdinfd)
            {
                fgets(sendBuf, sizeof(sendBuf), stdin);
                sendBuf[strlen(sendBuf) - 1] = '\0';

                ev.data.fd = sockfd;
                ev.events = EPOLLOUT | EPOLLET;
                //Ready to write after reading string from console
                epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev); // EPOLL_CTL_MOD: Change the settings associated with fd in the interest list to the new settings specified in event.

            }
            else if (events[i].events & EPOLLIN)
            //If it is a connected user and data is received, then read in.
            {
                if ((sockfd = events[i].data.fd) < 0)
                    continue;
                if ((nbytes = Read(sockfd, receiveBuf, MAXLINE, true)) < 0)
                {
                    if (errno == ECONNRESET)
                    {
                        close(sockfd);
                        exit(1);
                    }
                    else
                        printf("readline error\n");
                }
                else if (nbytes == 0)
                {
                    close(sockfd);
                    exit(1);
                }
                receiveBuf[nbytes] = '\0';
                // printf("AFTER EPOLLIN\n");

                //exit program
                if (strncmp(receiveBuf, BYE, strlen(BYE)) == 0)
                    exit(0);
            }
            else if (events[i].events & EPOLLOUT)
            {
                sockfd = events[i].data.fd;
                Write(sockfd, sendBuf, strlen(sendBuf), true);

                ev.data.fd = sockfd;
                ev.events = EPOLLIN | EPOLLET;
                //After writing, this sockfd is ready to read
                epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
            }
        }
    }
}
int Write(int sockfd, const char *buf, size_t len, bool printMessage)
{
    int nbytes;
    if ((nbytes = write(sockfd, buf, len)) < 0)
    {
        perror("Write!");
        exit(1);
    }
    if (printMessage == true)
        printf("WRITE --> %s| size --> %d\n", buf, nbytes);
    else
        printf("WRITE|size --> %d\n", nbytes);

    return nbytes;
}
int Read(int sockfd, char *buf, size_t len, bool printMessage)
{
    int nbytes;
    if ((nbytes = read(sockfd, buf, len)) < 0)
    {
        perror("Read!");
        exit(1);
    }
    if (nbytes < len)
        buf[nbytes] = '\0';
    if (printMessage == true)
        printf("READ ---> %s| size --> %d\n", buf, nbytes);
    else
        printf("READ| size --> %d\n", nbytes);

    return nbytes;
}