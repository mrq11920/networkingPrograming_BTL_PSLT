# networkingPrograming_BTL_PSLT

Bài tập lớn môn Lập trình mạng - nhóm Pháp sư lang thang

## Giao thức

Cổng giao thức: 9111

Phía Publisher: 
```
[+]Publisher Socket Created!
[+]Connected to Broker
Note: “QUIT” to quit
Input “CREATE” to create a new topic
Input "START" to start sending data

Publisher: HELO
Broker: 200 HELO client

Publisher: CREATE
Broker: 210 Create new topic
Publisher: locationA/sensorA
Broker: 220 Create topic successfully

Publisher: CREATE
Broker: 210 Create new topic
Publisher: locationAsensorA
Broker: 460 Invalid topic

Publisher: START
Broker: 230 Ok start sending data

Publisher: {"topic":"locationA/sensorA","datetime":"Sun Nov 28 17:50:51","temperature":"35","humidity":"53"}
Broker: 240 Topic is updated
Publisher:  {"topic":"locationF/sensorF","datetime":"Sun Nov 28 17:50:51","temperature":"35","humidity":"53"}
Broker: 450 Topic does not exist

Publisher: QUIT
Broker: 500 BYE
```

Sau đó tự động sinh dữ liệu cảm biến trong 1 khoảng thời gian nhất định trong topic này và publish lên Broker

Phía Subscriber:
```
[+]Subscriber Socket Created!
[+]Connected to Broker
Note: “QUIT” to quit
Input “SUB” to view all available topic then input a topic to subscribe
Input "START" to start receiving data

Publisher: abc
Broker: 404 Invalid command
Publisher: HELO
Broker: 200 HELO client

Subscriber: SUB
Broker: {"available_topics":"locationA/sensorA,locationA/sensorB"}
Subscriber: locationA/sensorA
Broker: 310 Subscribed to topic successfully

Subscriber: SUB
Broker: {"available_topics":"locationA/sensorA,locationA/sensorB"}
Subscriber: locationA/sensorD
Broker: 410 topic is not available

Subscriber: SUB
Broker: {"available_topics":"locationA/sensorA,locationA/sensorB"}
Subscriber: locationsensorAF
Broker: 460 Invalid topic

Subscriber: START
Broker: 320 Ok start receiving data
Broker: 
{“topic”:“locationA/sensorA”,“datetime”:”Sun Nov 28 17:50:51”,”temperature”:”35”,”humidity”:”56%”}
Broker: 
{“topic”:“locationA/sensorA”,“datetime”:”Sun Nov 28 17:50:52”,”temperature”:”35”,”humidity”:”57%”}

Subscriber: QUIT 
Broker: 500 BYE
```

Mỗi khi thông tin của topic bị thay đổi thì in ra màn hình thông tin đó
