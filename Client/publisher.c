// Nguyễn Minh Quang - 18021044
// Đây là Publisher, kết nối với broker tạo ra các topic rồi sinh dữ liệu và publish dữ liệu vào các topic 

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <regex.h>

int Write(int sockfd, const char *, size_t len, bool printMessage);
int Read(int sockfd, char *buf, size_t len, bool printMessage);
#define MAXLINE 4096
#define SERV_PORT 9111

#define C_CONNECTED 1
#define C_HANDSHAKING 2

#define C_CREATETOPIC 4
#define C_CREATETOPIC_DONE 8

#define C_START_SENDING_DATA 64

const char *BYE = "500 BYE";
const char *HELO_MESSAGE = "200 HELO client";
const char *CREATE_TOPIC_MESSAGE = "210 Create new topic";
const char *CREATE_TOPIC_SUCCESS_MESSAGE = "220 Create topic successfully";
const char *START_SENDING_DATA_MESSAGE = "230 Ok start sending data";
const char *UPDATE_TOPIC_SUCCESS_MESSAGE = "240 Topic is updated";

int main(int argc, char **argv)
{

    char receiveBuf[MAXLINE];
    char sendBuf[MAXLINE];
    char messageBuf[MAXLINE];
    int clientStatus;

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
    clientStatus = C_CONNECTED;

    // char topicId[128];
    char *createdTopics[10];
    int createdTopicsCount = 0;

    int epfd, stdinfd, nfds;
    struct epoll_event ev, events[5];
    epfd = epoll_create(10);

    //add stdinfd to epollfd
    stdinfd = fileno(stdin);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = stdinfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, stdinfd, &ev);

    //add sockfd to epollfd
    ev.data.fd = sockfd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    srand(time(NULL)); // Initialization, should only be called once.

    while (1)
    {
        nfds = epoll_wait(epfd, events, 5, 100); //1000milisecond = 1s
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
                {
                    close(sockfd);
                    exit(0);
                }
                else
                {
                    switch (clientStatus)
                    {
                    case C_CONNECTED:
                        /* code */
                        if (strncmp(receiveBuf, HELO_MESSAGE, strlen(HELO_MESSAGE)) == 0)
                        {
                            clientStatus = C_HANDSHAKING;
                        }
                        break;
                    case C_HANDSHAKING:
                        /* code */
                        if (strncmp(receiveBuf, CREATE_TOPIC_MESSAGE, strlen(CREATE_TOPIC_MESSAGE)) == 0)
                        {
                            clientStatus = C_CREATETOPIC;
                        }
                        break;
                    case C_CREATETOPIC:
                        /* code */
                        if (strncmp(receiveBuf, CREATE_TOPIC_SUCCESS_MESSAGE, strlen(CREATE_TOPIC_SUCCESS_MESSAGE)) == 0)
                        {
                            clientStatus = C_CREATETOPIC_DONE;
                            // allocate memory for topicId string
                            createdTopics[createdTopicsCount] = malloc(strlen(sendBuf));
                            // copy topicId from sendBuf to the top of createdTopics || createdTopics[createdTopicsCount]
                            strncpy(createdTopics[createdTopicsCount], sendBuf, strlen(sendBuf));
                            createdTopicsCount++;
                        }
                        break;
                    case C_CREATETOPIC_DONE:
                        // case 1: publisher send CREATE again to create a new topic and receive CREATE_TOPIC_MESSAGE(210 Create new topic)
                        if (strncmp(receiveBuf, CREATE_TOPIC_MESSAGE, strlen(CREATE_TOPIC_MESSAGE)) == 0)
                        {
                            // set it back to phase C_CREATETOPIC
                            clientStatus = C_CREATETOPIC;
                        }
                        // case 2: publisher send START to start sending data and receive START_SENDING_DATA_MESSAGE (230 Ok start sending data)
                        else if (strncmp(receiveBuf, START_SENDING_DATA_MESSAGE, strlen(START_SENDING_DATA_MESSAGE)) == 0)
                        {
                            clientStatus = C_START_SENDING_DATA;
                        }
                        break;
                    default:
                        break;
                    }
                }
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
        if (clientStatus == C_START_SENDING_DATA)
        {
            printf("Prepare data and send  it to Broker\n");
            for (int i = 0; i < createdTopicsCount; i++)
            {
                sleep(1);
                time_t now = time(NULL);
                struct tm tm_now;
                localtime_r(&now, &tm_now);
                char buff[100];
                strftime(buff, sizeof(buff), "%c", &tm_now);
                printf("Time is '%s'\n", buff);

                int r = rand();                // Returns a pseudo-random integer between 0 and RAND_MAX.
                int humidity = r % 50 + 30;    // range 30 -> 79
                int temperature = r % 15 + 25; //range 25 -> 39

                char dataBuf[128];
                sprintf(dataBuf, "{\"topic\":\"%s\",\"datetime\":\"%s\",\"temperature\":\"%d\",\"humidity\":\"%d\"}", createdTopics[i], buff, temperature, humidity);
                printf("data --> %s\n", dataBuf);

                strncpy(messageBuf, dataBuf, strlen(dataBuf));

                write(sockfd, messageBuf, strlen(messageBuf));
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