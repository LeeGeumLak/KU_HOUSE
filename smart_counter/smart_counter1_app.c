#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include "smart_counter1_lib.c"

#define ADDRESS     "tcp://172.20.10.2:1883"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "/curtain"
//#define PAYLOAD     "1"
#define QOS         1
#define TIMEOUT     10000L

char peopleCnt[100] = {'\0', };
char *people;
int temp;

int *pub(void)
{
	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	int rc;

	MQTTClient_create(&client, ADDRESS, CLIENTID,
			MQTTCLIENT_PERSISTENCE_NONE, NULL);
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;

	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
	{
		printf("Failed to connect, return code %d\n", rc);
		exit(-1);
	}

	while(1)
	{
		pubmsg.payload = people;
		pubmsg.payloadlen = strlen(people);
		pubmsg.qos = QOS;
		pubmsg.retained = 0;
		MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
		rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
		printf("Message with delivery token %d delivered\n", token);

		sleep(1);

	}

	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);

	return 0;
}

int main()
{
	pthread_t p_thread;

	int thr_id;
	int status;

	temp = 0;
	people = (char *)malloc(sizeof(char *));
	strcpy(peopleCnt, "0");
	people = &peopleCnt;

	counter_start(people);
	thr_id = pthread_create(&p_thread, NULL, pub(), NULL);

	if(thr_id < 0)
	{
		perror("error\n");
		exit(0);
	}

	pthread_join(p_thread, (void **)&status);
}
