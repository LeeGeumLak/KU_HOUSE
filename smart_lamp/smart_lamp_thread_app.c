#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "MQTTClient.h"
#include "smart_lamp_thread_lib.c"

#define ADDRESS     "tcp://127.0.0.1:1883"
#define CLIENTID    "ExampleClientSub"
#define TOPIC       "/curtain"
#define PAYLOAD     "1"
#define QOS         1
#define TIMEOUT     10000L

int check;
char* count;
int temp = 0;

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    int i;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    char *payloadptr = message->payload;

    count = message->payload;
    if(!(strcmp(count, "0") == 0) && (temp == 0)) {
	temp = 1;
	lamp_start();
    }
    if((strcmp(count, "0") == 0) && (temp == 1)) {
	temp = 0;
	lamp_stop();
    }

    for(i=0; i<message->payloadlen; i++) {
        putchar(*payloadptr++);
    }
    putchar('\n');

    printf("agine : %s\n", (char*)message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    check++;

    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

void sub(void) {

    while(1) {
    	MQTTClient client;
    	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    	int rc;
    	int ch;
    	check = 0;

    	MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    	conn_opts.keepAliveInterval = 20;
    	conn_opts.cleansession = 1;
    	MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
       	        printf("Failed to connect, return code %d\n", rc);
       	 	exit(EXIT_FAILURE);
    	}

    	printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
               "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);

    	MQTTClient_subscribe(client, TOPIC, QOS);

    	do {
		printf("one time\n");
		sleep(1);
    	} while(1);

    	MQTTClient_disconnect(client, 10000);
    	MQTTClient_destroy(&client);

    	sleep(1);

    	//return count;
    }
}

int main(void) {
	pthread_t my_thread;
	void *s;

	pthread_create(&my_thread, NULL, sub, NULL);
	pthread_join(my_thread, &s);

/*	while(1) {
		count = 1;
		temp = 0;
		if(count != 0 && temp == 0) {
			temp = 1;
			lamp_start();
		}
		if(count == 0 && temp == 1) {
			temp = 0;
			lamp_stop();
		}
		sleep(1);
	}*/
}
