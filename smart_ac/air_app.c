#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "air_lib.c"

#define ADDRESS     "tcp://172.20.10.2:1883"
#define CLIENTID    "ExampleClientSub"
#define TOPIC       "/curtain"
#define PAYLOAD     "Hello"
#define QOS         1
#define TIMEOUT     10000L

int check;
char *count;
int temp;

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
	printf("   message: ");
	payloadptr = message->payload;
	
	count = message->payload;

	if(!(strcmp(count, "0") == 0) && temp == 0)
	{
		temp = 1;
		air_start();
	}

	if((strcmp(count, "0") == 0) && temp == 1)
	{
		temp = 0;
		air_finish();
	}


	for(i=0; i<message->payloadlen; i++)
	{
		putchar(*payloadptr++);
	}
	putchar('\n');

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
int sub()
{
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

	MQTTClient_subscribe(client, TOPIC, QOS);
	do
	{
		printf("one time\n");
		sleep(1);
	}
	while(1);
	
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
	
	return count;
}

void *th_sub(void *data)
{
	printf("thr start\n");
	sub();
	printf("thr finish\n");
}

int main()
{
	pthread_t p_thread;
	
	int thr_id;
	int status;

	temp = 0;

	thr_id = pthread_create(&p_thread, NULL, th_sub, NULL);

	if(thr_id < 0)
	{
		perror("error\n");
		exit(0);
	}

	pthread_join(p_thread, (void **)&status);
}
