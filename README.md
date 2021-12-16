# networkingPrograming_BTL_PSLT

Cổng giao thức: 9111

## Giao thức

Phía Publisher: 
```
[+]Publisher Socket Created!
[+]Connected to Broker
Note: "QUIT" to quit
Input "CREATE" to create a new topic
Input "DEL"

Publisher: HELO
Broker: 200 HELO client
Publisher: CREATE
Broker: 210 Create new topic
Publisher: locationA/sensorA
Broker: 220 Create topic "locationA/sensorA" successfully
Publisher: {"topic":"locationA/sensorA","datetime":"Sun Nov 28 17:50:51","temperature":"35","humidity":"53%"}
Broker: 230 Topic is updated
Publisher: QUIT
Broker: 500 BYE
```

Sau đó tự động sinh dữ liệu cảm biến trong 1 khoảng thời gian nhất định trong topic này và publish lên Broker

Phía Subscriber:
```
[+]Subscriber Socket Created!
[+]Connected to Broker
Note: "QUIT" to quit
Input "TOPIC" to subscribe an available topic
Input "SUB" to view all available topic then input a topic to subscribe
Input "UNSUB" to view all subscribed topics then input a topic to unsubscribe

Publisher: abc
Broker: 404 Invalid command
Publisher: HELO
Broker: 200 HELO client
Subscriber: SUB
Broker: {"available_topics":"locationA/sensorA,locationA/sensorB"}
Subscriber: locationA/sensorA
Broker: 310 Subscribed to topic: "locationA/sensorA"
Subscriber: locationA/sensorD
Broker: 410 topic is not available
Broker: 
{"topic":"locationA/sensorA","datetime":"Sun Nov 28 17:50:51","temperature":"35","humidity":"56%"}
Broker: 
{"topic":"locationA/sensorA","datetime":"Sun Nov 28 17:50:52","temperature":"35","humidity":"57%"}
Subscriber: QUIT 
Broker: 500 BYE
```

Mỗi khi thông tin của topic bị thay đổi thì in ra màn hình thông tin đó
