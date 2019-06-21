#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include <errno.h>
#include <unistd.h>
#include "fridge_lib.c"
#include <pthread.h>

#define ADDRESS     "tcp://172.20.10.2:1883"
#define CLIENTID    "ExampleClientSub"
#define TOPIC       "/curtain"
#define PAYLOAD     "Hello"
#define QOS         1
#define TIMEOUT     10000L

int check;
char* count;
int temp=0;

volatile MQTTClient_deliveryToken deliveredtoken;
void delivered(void *context, MQTTClient_deliveryToken dt)
{
	printf("Message with token value %d delivery confirmed\n", dt);
	deliveredtoken = dt;
}
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
	int i;
	char* payloadptr;
	printf("Message arrived\n");
	printf("     topic: %s\n", topicName);
	printf("   message: ");
	payloadptr = message->payload;
	
	count = message->payload;
	if(!(strcmp(count,"0")==0) && temp ==0 ){
		temp = 1;
		fridge_start();
	}
	else if((strcmp(count,"0")==0) && temp ==1){
		temp = 0;	
		fridge_finish();
	}

	for(i=0; i<message->payloadlen; i++)
	{
		putchar(*payloadptr++);
	}
	putchar('\n');

	printf("agine : %s\n", (char*)message->payload);
	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	check++;
	return 1;
}
void connlost(void *context, char *cause)
{
	printf("\nConnection lost\n");
	printf("     cause: %s\n", cause);
}
void sub()
{
    while(1){
	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	int rc;
	int ch;
	check = 0;
	MQTTClient_create(&client, ADDRESS, CLIENTID,
			MQTTCLIENT_PERSISTENCE_NONE, NULL);
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
	{
		printf("Failed to connect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
			"Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);

	MQTTClient_subscribe(client, TOPIC, QOS);
	do
	{
		printf("one time\n");
		sleep(1);
	}
	while(check != 1);
	
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
	
	sleep(1);
    }
}

int main()
{

	temp = 0;

	pthread_t my_thread;
	void *s;

	pthread_create(&my_thread, NULL, sub, NULL);
	pthread_join(my_thread, &s);

//	printf("ddddddddddddddddddd\n");
//	while(1) {
//		if(count != 0)
//		{//	temp = 1;
//			fridge_start();
//		}
//		if(count ==0)
//		{
		//	temp = 0;
//			fridge_finish();
//		}
//		sleep(1);
//	}
}
