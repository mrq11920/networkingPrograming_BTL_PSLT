//Nguyễn Minh Quang - 18021044
// Đây chương trình broker, nhận yêu cầu từ subscriber, publisher và thực hiện theo yêu cầu như trong giao thức 

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>

#define MAXLINE 4096
#define LISTENQ 20
#define SERV_PORT 9111

#define PUBLISHER 256
#define SUBSCRIBER 257

#define C_CONNECTED 1
#define C_HANDSHAKING 2

#define C_CREATETOPIC 4
#define C_CREATETOPIC_DONE 8

#define C_SUB2TOPIC 16
#define C_SUB2TOPIC_DONE 32

#define C_START_SENDING_DATA 64
#define C_START_RECEIVING_DATA 128

// #define C_AUTHENTICATING 8
// #define C_AUTHENTICATING_DONE 16
// #define C_CHATTING 32
// #define C_DONE 64

#define MAX_CONNECTIONS 256
#define MAX_TOPICS 128

#define SUB_ALL_TOPIC 512
#define SUB_ALL_SENSOR_IN_LOCATION 513
#define SUB_A_TOPIC 514

void setnonblocking(int sock);

const char *HELO = "HELO";
const char *QUIT = "QUIT";
const char *BYE_MESSAGE = "500 BYE";
const char *CREATE = "CREATE";
const char *SUB2TOPIC = "SUB";
const char *LIST = "LIST";
const char *START = "START";

// struct subscriber
// {
//     int subscriberId;
//     char *messageBuffer[10];
//     int messageCount;
// };
// struct publisher
// {
//     int publisherId;
//     char *messageBuffer[32];
//     int messageCount;
// };
struct topic
{
    char topicId[128];
    // so if subscriberAndPublisherfdList[5] = PUBLISHER
    // meaning that in this topic client has file descriptor equal 5 is a PUBLISHER
    // the same for subscriberAndPublisherfdList[6] = SUBSCRIBER
    // meaning that in this topic client has file descriptor equal 6 is a SUBSCRIBER
    int subscriberAndPublisherfdList[MAX_CONNECTIONS];
    char *messageQueue[256];
    int messageCount;
    // int dispose;
    // int publisherList[MAX_CONNECTIONS];
    // int subscriberList[MAX_CONNECTIONS];
};

struct topic allTopics[MAX_TOPICS];

int topicsHaveMessageQueue[MAX_TOPICS];
int topicsHaveMessageCount = 0;

int allClientsType[MAX_CONNECTIONS];
short clientStatus[MAX_CONNECTIONS];

// struct user allOnlineUsers[MAX_USERS];

// int getAllOnlineUser(char *buf);
// int addUserToOnlineList(int sockfd, struct user usr);

int getAllAvailableTopics(char *buf)
{
    strcpy(buf, "{\"available_topics\":\"");
    int count = 0;
    for (int i = 0; i < MAX_TOPICS; i++)
    {
        if (strlen(allTopics[i].topicId) > 0)
        {
            char topicId[128];
            strcpy(topicId, allTopics[i].topicId);

            //append string to buffer
            if (count == 0)
                strcat(buf, topicId);
            else
            {
                strcat(buf, ",");
                strcat(buf, topicId);
            }
            count++;
        }
    }
    strcat(buf, "\"}");
    return 1;
}
int getSubscribedTopics(int sockfd)
{
    // strcpy()
}
int handleQuitClient(int sockfd)
{
    printf("call handleQuitClient, sockfd --> %d\n", sockfd);
    clientStatus[sockfd] = 0;
    // int clientType = allClients[sockfd]; // clientType: PUBLISHER or SUBSCRIBER
    allClientsType[sockfd] = 0;
    for (int i = 0; i < MAX_TOPICS; i++)
    {
        // set every client that has file descriptor equal sockfd in every topic to ZERO
        if (strlen(allTopics[i].topicId) > 0)
        {
            allTopics[i].subscriberAndPublisherfdList[sockfd] = 0;
        }
    }
}

int subscribeToTopic(char *topicId, int sockfd, int subscribeType)
{
    int success = 0;
    int clientType = allClientsType[sockfd];
    // printf("clientType --> %d\n", (clientType == SUBSCRIBER) ? "SUBSCRIBER" : "PUBLISHER");
    for (int i = 0; i < MAX_TOPICS; i++)
    {
        // check if a topic is valid and client is a SUBSCRIBER
        if (strlen(allTopics[i].topicId) > 0 && allClientsType[sockfd] == SUBSCRIBER)
        {
            switch (subscribeType)
            {
            case SUB_ALL_TOPIC:
                printf("case SUB_ALL_TOPIC\n");
                allTopics[i].subscriberAndPublisherfdList[sockfd] = SUBSCRIBER;
                success = 1;
                break;
            case SUB_ALL_SENSOR_IN_LOCATION:
                printf("case SUB_ALL_SENSOR_IN_LOCATION\n");
                //point to slash '/'
                char *slash;
                // index of slash '/'
                int index;
                slash = strchr(topicId, '/');
                index = (int)(slash - topicId);

                //compare location of topic
                if (strncmp(allTopics[i].topicId, topicId, index) == 0)
                {
                    allTopics[i].subscriberAndPublisherfdList[sockfd] = SUBSCRIBER;
                    success = 1;
                }
                break;
            case SUB_A_TOPIC:
                printf("case SUB_A_TOPIC\n");
                if (strncmp(allTopics[i].topicId, topicId, strlen(topicId)) == 0)
                {
                    allTopics[i].subscriberAndPublisherfdList[sockfd] = SUBSCRIBER;
                    success = 1;
                }
                break;
            default:
                break;
            }
        }
    }
    return success;
}

int addMessageToTopic(char *topicId, char *message, int sockfd)
{
    // to do test to check
    printf("addMessageToTopic");
    int topicIndex = -1;
    for (int i = 0; i < MAX_TOPICS; i++)
    {
        if (strlen(allTopics[i].topicId) > 0 && allTopics[i].subscriberAndPublisherfdList[sockfd] == PUBLISHER && strncmp(allTopics[i].topicId, topicId, strlen(topicId)) == 0)
        {
            topicIndex = i;
            struct topic *tp;
            tp = &allTopics[i];
            printf("found topic --> %s\n", topicId);
            //allocate space for message
            tp->messageQueue[tp->messageCount] = malloc(strlen(message));
            strncpy(tp->messageQueue[tp->messageCount], message, strlen(message));
            tp->messageCount++;
            break;
        }
    }
    return topicIndex;
}
int addTopicToAllTopics(struct topic tp, int sockfd)
{
    //check topic already exists
    // char tpId[tp.topicId]
    int tpExists = 0;
    for (int i = 0; i < MAX_TOPICS; i++)
    {
        if (strlen(allTopics[i].topicId) > 0 && strncmp(tp.topicId, allTopics[i].topicId, strlen(tp.topicId)) == 0)
        {
            tpExists = 1;
            allTopics[i].subscriberAndPublisherfdList[sockfd] = PUBLISHER;
            break;
        }
    }
    if (!tpExists)
    {
        for (int i = 0; i < MAX_TOPICS; i++)
        {
            if (strlen(allTopics[i].topicId) == 0)
            {
                struct topic *t;
                t = &allTopics[i];
                strcpy(t->topicId, tp.topicId);
                t->subscriberAndPublisherfdList[sockfd] = PUBLISHER;
                break;
            }
        }
    }
}
//pass message to receiver buffer
// int passMessageToReceiver(char *sender, char *receiver, char *message);

int main(int argc, char *argv[])
{
    int i, maxi, listenfd, connfd, sockfd, epfd, nfds;
    ssize_t n;
    char receiveBuf[MAXLINE];
    char sendBuf[MAXLINE];
    char messageBuf[MAXLINE];

    socklen_t clilen;

    //ev is used to register events, and the array is used to return the events to be processed
    struct epoll_event ev, events[20];

    //Generate epoll-specific file descriptors used to process accept
    epfd = epoll_create(MAX_CONNECTIONS);

    struct sockaddr_in cliaddr, servaddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // setnonblocking(listenfd);

    //Set the file descriptor related to the event to be processed
    ev.data.fd = listenfd;
    ev.events = EPOLLIN | EPOLLET;
    //Register epoll event

    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    listen(listenfd, LISTENQ);

    printf("Waiting for a client at port --> %d \n", SERV_PORT);
    maxi = 0; //useless

    for (;;)
    {
        //every 100ms loop return max 20 socket I/O events
        nfds = epoll_wait(epfd, events, 20, 100);

        for (i = 0; i < nfds; ++i)
        {
            if (events[i].data.fd == listenfd) //If it is newly detected that a SOCKET user is connected to the bound SOCKET port, a new connection is established.
            {
                connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
                if (connfd < 0)
                {
                    perror("connfd<0");
                    exit(1);
                }
                // setnonblocking(connfd);

                char *str = inet_ntoa(cliaddr.sin_addr);
                printf("accept a connection from %s\n", str);

                // we identify each client or user by it's sockfd
                clientStatus[connfd] = C_CONNECTED;

                //add new connfd to watchlist (epfd)
                ev.data.fd = connfd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
            }
            else
            {
                if (events[i].events & EPOLLIN)
                //If it is a connected user and data is received, then read in.
                {
                    // printf("EPOLLIN\n");
                    if ((sockfd = events[i].data.fd) < 0)
                        continue;
                    if ((n = read(sockfd, receiveBuf, MAXLINE)) < 0)
                    {
                        if (errno == ECONNRESET)
                        {
                            close(sockfd);
                            handleQuitClient(sockfd);
                            events[i].data.fd = -1;
                        }
                        else
                            printf("readline error\n");
                    }
                    else if (n == 0)
                    {
                        close(sockfd);
                        handleQuitClient(sockfd);
                        events[i].data.fd = -1;
                    }
                    receiveBuf[n] = '\0';
                    // printf("AFTER EPOLLIN\n");
                    // user send QUIT to end session
                    //check if receiveBuf contains QUIT
                    if (strstr(receiveBuf, QUIT) != NULL)
                    {
                        strcpy(sendBuf, "500 BYE");
                    }
                    // we identify each client or user by it's sockfd
                    else
                    {
                        switch (clientStatus[sockfd])
                        {
                        case C_CONNECTED:
                            // now we read message and do handshaking
                            // Client: HELO
                            // Server: 200 HELO client
                            printf("check for HELO from client!, sockfd --> %d\n", sockfd);
                            if (strncmp(receiveBuf, HELO, strlen(HELO)) == 0)
                            {
                                strcpy(sendBuf, "200 HELO client");
                                clientStatus[sockfd] = C_HANDSHAKING;
                            }
                            else
                                strcpy(sendBuf, "404 Invalid command");
                            break;
                        case C_HANDSHAKING:
                            // now we identify client
                            // printf("check for SUB or CREATE from client!\n");
                            // case 1:
                            // Publisher: CREATE
                            // Broker: 210 Create new topic
                            if (strncmp(receiveBuf, CREATE, strlen(CREATE)) == 0)
                            {
                                strcpy(sendBuf, "210 Create new topic");
                                clientStatus[sockfd] = C_CREATETOPIC;
                                allClientsType[sockfd] = PUBLISHER;
                            }
                            // case 2
                            // Subscriber: SUB
                            // Broker: {“available topics”:”locationA/sensorA,locationA/sensorB”}
                            else if (strncmp(receiveBuf, SUB2TOPIC, strlen(SUB2TOPIC)) == 0)
                            {
                                // return  {“available topics”:”locationA/sensorA,locationA/sensorB”}
                                // strcpy(sendBuf, "{“available topics”:”locationA/sensorA,locationA/sensorB”}");
                                printf("client send SUB \n");
                                char availableTopics[MAXLINE];
                                getAllAvailableTopics(availableTopics);
                                strcpy(sendBuf, availableTopics);

                                clientStatus[sockfd] = C_SUB2TOPIC;
                                allClientsType[sockfd] = SUBSCRIBER;
                            }
                            else
                                strcpy(sendBuf, "404 Invalid command");
                            break;
                        case C_CREATETOPIC:
                            // Publisher: locationA/sensorA
                            // Broker: 220 Create topic successfully
                            // validate input by checking if it contains '/'
                            if (strchr(receiveBuf, '/') != NULL)
                            {
                                // createTopic
                                struct topic tp;
                                strcpy(tp.topicId, receiveBuf);
                                tp.messageCount = 0;
                                tp.subscriberAndPublisherfdList[sockfd] = PUBLISHER;

                                addTopicToAllTopics(tp, sockfd);
                                strcpy(sendBuf, "220 Create topic successfully");
                                clientStatus[sockfd] = C_CREATETOPIC_DONE;
                            }
                            else
                            {
                                strcpy(sendBuf, "460 Invalid topic");
                                //set it back to phase C_HANDSHAKING
                                clientStatus[sockfd] = C_HANDSHAKING;
                            }

                            break;
                        case C_CREATETOPIC_DONE:
                            // case 1: publisher sends START to start sending message
                            if (strncmp(receiveBuf, START, strlen(START)) == 0)
                            {
                                strcpy(sendBuf, "230 Ok start sending data");
                                clientStatus[sockfd] = C_START_SENDING_DATA;
                            }
                            // case 2: publisher sends CREATE again to create a new topic
                            else if (strncmp(receiveBuf, CREATE, strlen(CREATE)) == 0)
                            {
                                strcpy(sendBuf, "210 Create new topic");
                                clientStatus[sockfd] = C_CREATETOPIC;
                            }
                            else
                                strcpy(sendBuf, "404 Invalid command");
                            break;
                        case C_START_SENDING_DATA:
                            // Publisher: {"topic":"locationA/sensorA","datetime":"Sun Nov 28 17:50:51","temperature":"35","humidity":"56%""}
                            // Broker: 240 Topic is updated
                            printf("check for message from publisher | case C_CREATETOPIC_DONE\n");
                            printf("receiveBuff --> %s\n", receiveBuf);
                            if (strncmp(receiveBuf, "{\"topic\"", 8) == 0)
                            {
                                char *regexString = "\\{\"topic\":\"(.*)\",\"datetime\":\"(.*)\"";
                                size_t maxGroups = 3;
                                regex_t regexCompiled;
                                regmatch_t groupArray[maxGroups];
                                if (regcomp(&regexCompiled, regexString, REG_EXTENDED))
                                {
                                    printf("Could not compile regular expression.\n");
                                    return 1;
                                }
                                printf("compile regex successfully\n");
                                if (regexec(&regexCompiled, receiveBuf, maxGroups, groupArray, 0) == 0)
                                {
                                    // reference: https://www.gnu.org/software/libc/manual/html_node/Regexp-Subexpressions.html
                                    // groupArray[i].rm_so  vị trí bắt dầu của match string tương ứng với group (0 , 1, 2 hoặc 3)
                                    // groupArray[i].rm_eo  index cuối cùng của match string tương ứng với group (0 1 2 3)
                                    // nên độ dài matching string = groupArray[i].rm_eo - groupArray[i].rm_so
                                    // printf("found topicId\n");
                                    char topicId[128];
                                    int topicIdLength = groupArray[1].rm_eo - groupArray[1].rm_so;
                                    // printf("topicId length --> %d \n", topicIdLength);
                                    strncpy(topicId, receiveBuf + (int)groupArray[1].rm_so, topicIdLength);
                                    topicId[topicIdLength] = '\0';

                                    printf("topicId ---> %s, Message --> %s\n", topicId, receiveBuf);

                                    int topicIndex = addMessageToTopic(topicId, receiveBuf, sockfd);
                                    printf("message just added to topic index --> %d\n", topicIndex);

                                    // add topic that has new message to a queue, later we sent this message to all of its subscribers
                                    if (topicIndex >= 0)
                                    {
                                        topicsHaveMessageQueue[topicsHaveMessageCount] = topicIndex;
                                        topicsHaveMessageCount++;
                                        strcpy(sendBuf, "240 Topic is updated");
                                    }
                                    else
                                    {
                                        printf("does not found specific topic\n");
                                        strcpy(sendBuf, "450 Topic does not exist");
                                    }
                                }
                                else
                                    strcpy(sendBuf, "404 Invalid command");
                            }
                            else
                                strcpy(sendBuf, "404 Invalid command");
                            break;
                        case C_SUB2TOPIC:
                            // Subscriber: locationA/sensorA
                            // Broker: 310 Subscribed to topic: “locationA/sensorA”
                            printf("Check for topic input from client\n");
                            char topicId[128];

                            int success = 0;
                            strncpy(topicId, receiveBuf, n + 1);
                            printf("client want to sub to topicId --> %s\n", topicId);
                            // validate topic by checking if it contains '/'
                            if (strchr(receiveBuf, '/') != NULL)
                            {
                                char location[128], sensor[128];
                                // split topicId by '/' and copy to location and sensor
                                int count = 0;
                                char *end, *r, *tok;
                                r = end = strdup(topicId);
                                while ((tok = strsep(&end, "/")) != NULL)
                                {
                                    if (count == 0)
                                    {
                                        printf("location --> %s\n", tok);
                                        strcpy(location, tok);
                                    }
                                    else
                                    {
                                        printf("sensor --> %s\n", tok);
                                        strcpy(sensor, tok);
                                    }
                                    count++;
                                }
                                // free duplicate string (a copy of topicId)
                                free(r);

                                // case 1: */* means sub to every topic
                                if (strncmp(location, "*", 1) == 0 && strncmp(sensor, "*", 1) == 0)
                                    success = subscribeToTopic(topicId, sockfd, SUB_ALL_TOPIC);
                                // case 2: locationA/* means sub to every sensors in locationA
                                else if (strncmp(sensor, "*", 1) == 0)
                                    success = subscribeToTopic(topicId, sockfd, SUB_ALL_SENSOR_IN_LOCATION);
                                // case 3: locationA/SensorA just sub to a topic
                                else
                                    success = subscribeToTopic(topicId, sockfd, SUB_A_TOPIC);

                                if (success)
                                {
                                    strcpy(sendBuf, "310 Subscribe to topic successfully");
                                    clientStatus[sockfd] = C_SUB2TOPIC_DONE;
                                }
                                else
                                {
                                    strcpy(sendBuf, "410 Topic is not available");
                                    // return to phase C_HANDSHAKING.
                                    clientStatus[sockfd] = C_HANDSHAKING;
                                }
                            }
                            else
                            {
                                strcpy(sendBuf, "460 Invalid topic");
                                //set it back to phase C_HANDSHAKING
                                clientStatus[sockfd] = C_HANDSHAKING;
                            }

                            break;
                        case C_SUB2TOPIC_DONE:

                            // case 1: subscriber sends START to start receiving data
                            if (strncmp(receiveBuf, START, strlen(START)) == 0)
                            {
                                strcpy(sendBuf, "320 Ok start receiving data");
                                clientStatus[sockfd] = C_START_RECEIVING_DATA;
                            }
                            // case 2: publisher sends SUB again to subscribe to a topic
                            else if (strncmp(receiveBuf, SUB2TOPIC, strlen(SUB2TOPIC)) == 0)
                            {
                                printf("client send SUB \n");
                                char availableTopics[MAXLINE];
                                getAllAvailableTopics(availableTopics);
                                strcpy(sendBuf, availableTopics);
                                //set it back to phase C_SUB2TOPIC
                                clientStatus[sockfd] = C_SUB2TOPIC;
                            }
                            else
                                strcpy(sendBuf, "404 Invalid command");

                            break;
                        case C_START_RECEIVING_DATA:
                            break;
                        default:
                            break;
                        }
                    }

                    ev.data.fd = sockfd;
                    ev.events = EPOLLOUT | EPOLLET;
                    //Ready to write after reading
                    epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev); // EPOLL_CTL_MOD: Change the settings associated with fd in the interest list to the new settings specified in event.
                }
                else if (events[i].events & EPOLLOUT) //  If there is data to send
                {
                    sockfd = events[i].data.fd;
                    write(sockfd, sendBuf, strlen(sendBuf));

                    if (strncmp(sendBuf, BYE_MESSAGE, strlen(BYE_MESSAGE)) == 0)
                    {
                        close(sockfd);
                        handleQuitClient(sockfd);
                        events[i].data.fd = -1;
                    }
                    else
                    {
                        ev.data.fd = sockfd;
                        ev.events = EPOLLIN | EPOLLET;
                        //After writing, this sockfd is ready to read
                        epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
                    }
                }
            }
        }
        if (topicsHaveMessageCount > 0)
        {
            printf("topicsHaveMessageCount --> %d\n", topicsHaveMessageCount);
            // pop from top of stack
            struct topic *tp = &allTopics[topicsHaveMessageQueue[topicsHaveMessageCount - 1]];
            // minus 1 because we pop data
            topicsHaveMessageCount--;
            if (tp->messageCount > 0)
            {
                strcpy(messageBuf, tp->messageQueue[tp->messageCount - 1]);
                printf("copy message from topic to messageBuf\n");
                for (int i = 0; i < MAX_CONNECTIONS; i++)
                {
                    // check in sockfd list of a topic if it's a SUBSCRIBER and ready for receiving message
                    if (tp->subscriberAndPublisherfdList[i] == SUBSCRIBER && clientStatus[i] == C_START_RECEIVING_DATA)
                    {
                        write(i, messageBuf, strlen(messageBuf));
                    }
                }

                printf("free one message from top of a topic\n");
                free(tp->messageQueue[tp->messageCount - 1]);
                tp->messageCount--;
            }
        }
    }
    return 0;
}

void setnonblocking(int sock)
{
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0)
    {
        perror("fcntl(sock,GETFL)");
        exit(1);
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0)
    {
        perror("fcntl(sock,SETFL,opts)");
        exit(1);
    }
}
