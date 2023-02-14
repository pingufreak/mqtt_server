#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <strings.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include "mqtt_structs.h"
#include <stdbool.h>
#include "mqtt_struct_queue.c"
#include <semaphore.h>

#define DEBUG

const char    *SERVER_IP = "127.0.0.1";
const char    *SERVER_IF = "lo";
const uint16_t SERVER_PORT = 1883;
const uint8_t  SEM_WAIT = 10;

// Aus bind(2) Beispiel 
#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

void *clientThread(void*);

typedef struct clientThreadStructTpl {
  int clientSocketFD;
  Queue *queue;
} clientThreadStructTpl;

sem_t semQueueFull, semQueueEmpty;
pthread_mutex_t mutex;

int main(int argc, char *argv) {
 int serverSocketFD, clientSocketFD;
 struct sockaddr_in serverSocketSettings, clientSocketSettings;

 pthread_mutex_init(&mutex, NULL);
 sem_init(&semQueueEmpty, 0, SEM_WAIT);
 sem_init(&semQueueFull, 0, 0);

  
 // serverSocketFD erstellen
 errno = 0;
 serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
 if ( errno != 0 ) { handle_error("serverSocketFD socket()"); } 
 #ifdef DEBUG 
 printf("debug: serverSocketFD socket()\n"); 
 #endif

 // Addresse darf wiederverwendet werden
 errno = 0;
 setsockopt(serverSocketFD, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
 if ( errno != 0 ) { handle_error("setsockopt() SO_REUSEADDR"); }
 #ifdef DEBUG 
 printf("debug: setsockopt() SO_REUSEADDR\n"); 
 #endif

 // Port darf wiederverwendet werden
 errno = 0;
 setsockopt(serverSocketFD, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));
 if ( errno != 0 ) { handle_error("setsockopt() SO_REUSEPORT"); }
 #ifdef DEBUG 
 printf("debug: setsockopt() SO_REUSEPORT\n"); 
 #endif

 // Der Server soll nur ein Interface lauschen
 errno = 0;
 setsockopt(serverSocketFD, SOL_SOCKET, SO_BINDTODEVICE, SERVER_IF, sizeof(SERVER_IF));
 if ( errno != 0 ) { handle_error("setsockopt() SO_BINDTODEVICE"); }
 #ifdef DEBUG 
 printf("debug: setsockopt() SO_BINDTODERVICE\n"); 
 #endif

 // Speicher für Struktur sockaddr_in setzen
 errno = 0;
 bzero(&serverSocketSettings, sizeof(serverSocketSettings));
 if( errno != 0 ) { handle_error("bzero() serverSocketSettings"); };
 #ifdef DEBUG 
 printf("debug: bzero() serverSocketSettings\n");
 #endif
 
 // Speicher für Struktur in_addr setzen
 errno = 0;
 struct in_addr serverSocketIP;
 bzero(&serverSocketIP, sizeof(serverSocketIP));
 if( errno != 0 ) { handle_error("bzero() serverSocketIP"); };
 #ifdef DEBUG 
 printf("debug: bzero() serverSocketIP\n"); 
 #endif

 // Zuweisen der Protokollfamilie
 serverSocketSettings.sin_family = AF_INET;
 
 // Zuweisen der IP-Adresse zur in_addr Struktur serverSocketIP
 errno = 0;
 inet_aton(SERVER_IP, &serverSocketIP);
 if( errno != 0 ) { handle_error("inet_aton() SERVER_IP"); };
 serverSocketSettings.sin_addr = serverSocketIP;
 #ifdef DEBUG 
 printf("debug: inet_aton() SERVER_IP\n");
 #endif

 // TCP-Port definieren
 errno = 0;
 serverSocketSettings.sin_port = htons(SERVER_PORT);
 if( errno != 0 ) { handle_error("htons() SERVER_PORT"); };
 #ifdef DEBUG 
 printf("debug: htons() SERVER_PORT\n"); 
 #endif

 // Socket binden
 errno = 0;
 bind(serverSocketFD, (struct sockaddr*)&serverSocketSettings, sizeof(serverSocketSettings));
 if( errno != 0 ) { handle_error("bind() serverSocketFD"); };
 #ifdef DEBUG 
 printf("debug: bind() serverSocketFD\n"); 
 #endif

 // Listen aktivieren
 errno = 0;
 listen(serverSocketFD, 5);
 if( errno != 0 ) { handle_error("listen() serverSocketFD"); };
 #ifdef DEBUG 
 printf("debug: listen() serverSocketFD\n"); 
 #endif

 // Client Verbindungen akzeptieren
 int clientSocketSettingsLength = sizeof(struct sockaddr_in);
 Queue * queue = initQueue();
 clientThreadStructTpl clientThreadStruct;
 clientThreadStruct.queue = queue;

 int i = 0;

 while(1) {
  errno = 0;
  clientSocketFD = accept(serverSocketFD, (struct sockaddr*)&clientSocketSettings, &clientSocketSettingsLength);
  clientThreadStruct.clientSocketFD = clientSocketFD;
  
  if( errno != 0 ) { handle_error("accept() serverSocketFD"); };
  #ifdef DEBUG 
  printf("debug: accept() serverSocketFD\n"); 
  #endif

  pthread_t threadId = (pthread_t) malloc(sizeof(pthread_t));
  errno = 0;
  pthread_create( &threadId, NULL, clientThread, (void *) &clientThreadStruct);
  if( errno != 0 ) { handle_error("close() clientSocketFD"); };
  #ifdef DEBUG 
  printf("debug: pthread_create clientSocketFD\n"); 
  #endif
 }

 errno = 0;
 close(serverSocketFD);
 if( errno != 0 ) { handle_error("close() serverSocketFD"); };
 #ifdef DEBUG 
 printf("debug: close() serverSocketFD\n");
 #endif

 #ifdef DEBUG 
 printf("debug: pthread_mutex_destroy(&mutex)\n");
 #endif
 errno = 0;
 pthread_mutex_destroy(&mutex);
 if( errno != 0 ) { handle_error("pthread_mutex_destroy(&mutex)"); };

 #ifdef DEBUG 
 printf("debug: sem_destroy(&semQueueEmpty)\n");
 #endif
 errno = 0;
 sem_destroy(&semQueueEmpty);
 if( errno != 0 ) { handle_error("sem_destroy(&semQueueEmpty)"); };

 #ifdef DEBUG 
 printf("debug: sem_destroy(&semQueueFull)\n");
 #endif
 errno = 0;
 sem_destroy(&semQueueFull);
 if( errno != 0 ) { handle_error("sem_destroy(&semQueueFull)"); };

 return EXIT_SUCCESS;
}

void *clientThread(void *arg) {  
 clientThreadStructTpl *clientThreadStruct = (clientThreadStructTpl *) arg;
 int clientSocketFDTmp = clientThreadStruct->clientSocketFD;
 Queue *queue = clientThreadStruct->queue;

 int index = 0;

 uint8_t mqttFixedHeaderBuffer[1], mqttControlPacketConnectBuffer[17], mqttControlPacketConnectackBuffer[4], mqttControlPacketPublishBuffer[9], mqttControlPacketPublishSendBuffer[10], mqttControlPacketSubscribeBuffer[9], mqttControlPacketSubackBuffer[5], mqttControlPacketDisconnectBuffer[2];

 /*
   recv() CONNECT
 */

 errno = 0;
 recv(clientSocketFDTmp, &mqttControlPacketConnectBuffer, sizeof(mqttControlPacketConnectBuffer), 0);
 if( errno != 0 ) { handle_error("send() mqttControlPacketConnectBuffer"); };

 mqttControlPacketConnectTpl *mqttControlPacketConnect = (mqttControlPacketConnectTpl*) malloc(sizeof(mqttControlPacketConnectTpl));
 
 mqttControlPacketConnect->mqttFixedHeaderByte1 = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->mqttFixedHeaderRemainingLength = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->mqttVariableHeaderProtocolNameMSB = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->mqttVariableHeaderProtocolNameLSB = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar0 = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar1 = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar2 = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar3 = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->mqttVariableHeaderProtocolLevel = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->mqttVariableHeaderConnectFlags = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->mqttVariableHeaderKeepAliveLSB = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->mqttVariableHeaderKeepAliveMSB = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->clientIdMSB = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->clientIdLSB = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->clientIdChar0 = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->clientIdChar1 = mqttControlPacketConnectBuffer[index++];
 mqttControlPacketConnect->clientIdChar2 = mqttControlPacketConnectBuffer[index];

 #ifdef DEBUG 
 printf("recv(): mqttControlPacketConnectBuffer\n");
 #endif

 // FIXME: CONNECT AUSWERTEN, FEWHLERHANDLING!!!

/*
   send() CONNACK
 */
 
 mqttControlPacketConnectackTpl *mqttControlPacketConnectack = (mqttControlPacketConnectackTpl*) malloc(sizeof(mqttControlPacketConnectackTpl));

 index = 0;

 mqttControlPacketConnectack->mqttFixedHeaderByte1Bits.mqttControlPacketFlags = 0;
 mqttControlPacketConnectack->mqttFixedHeaderByte1Bits.mqttControlPacketType = CONNACK;
 mqttControlPacketConnectackBuffer[index++] = mqttControlPacketConnectack->mqttFixedHeaderByte1;

 mqttControlPacketConnectack->mqttFixedHeaderRemainingLength = 15;
 mqttControlPacketConnectackBuffer[index++] = mqttControlPacketConnectack->mqttFixedHeaderRemainingLength;

 mqttControlPacketConnectack->mqttVariableHeaderConnectackFlags = 0;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnectack->mqttVariableHeaderConnectackFlags;

 mqttControlPacketConnectack->mqttVariableHeaderConnectackReturncode = 0;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnectack->mqttVariableHeaderConnectackReturncode;

 errno = 0;
 send(clientSocketFDTmp, &mqttControlPacketConnectackBuffer, sizeof(mqttControlPacketConnectackBuffer), 0);
 if( errno != 0 ) { handle_error("send() mqttControlPacketConnectackBuffer"); };

 #ifdef DEBUG 
 printf("send(): mqttControlPacketConnectackBuffer\n");
 #endif

 errno = 0;
 recv(clientSocketFDTmp, &mqttFixedHeaderBuffer, sizeof(mqttFixedHeaderBuffer), 0);
 if( errno != 0 ) { handle_error("receive() mqttFixedHeaderBuffer"); };

 #ifdef DEBUG 
 printf("recv(): mqttFixedHeaderBuffer\n");
 #endif

 mqttFixedHeaderTpl mqttFixedHeader;
 mqttFixedHeader.mqttFixedHeaderByte1 = mqttFixedHeaderBuffer[0];

 if(mqttFixedHeader.mqttFixedHeaderByte1Bits.mqttControlPacketType == PUBLISH) {
  /*
   recv() PUBLISH
  */

  #ifdef DEBUG 
  printf("recv(): mqttFixedHeaderBuffer -> PUBLISH received\n");
  #endif
 
  errno = 0;
  recv(clientSocketFDTmp, &mqttControlPacketPublishBuffer, sizeof(mqttControlPacketPublishBuffer), 0);
  if( errno != 0 ) { handle_error("receive() mqttControlPacketPublishBuffer"); };
  
  #ifdef DEBUG 
  printf("recv(): mqttControlPacketPublishBuffer\n");
  #endif

  // mqttControlPacketPublish vorbereiten und in Puffer zum Versand ablegen 
  mqttControlPacketPublishTpl *mqttControlPacketPublish = (mqttControlPacketPublishTpl*) malloc(sizeof(mqttControlPacketPublishTpl));
 
  index = 0;

  mqttControlPacketPublish->mqttFixedHeaderRemainingLength = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderTopicNameLSB = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderTopicNameMSB = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderTopicNameChar0 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderTopicNameChar1 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderTopicNameChar2 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderPayloadChar0 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderPayloadChar1 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderPayloadChar2 = mqttControlPacketPublishBuffer[index];

  sem_wait(&semQueueEmpty);
  pthread_mutex_lock(&mutex);

  char *topic = malloc(4);
  char *value = malloc(4);

  topic[0] = mqttControlPacketPublish->mqttVariableHeaderTopicNameChar0; 
  topic[1] = mqttControlPacketPublish->mqttVariableHeaderTopicNameChar1; 
  topic[2] = mqttControlPacketPublish->mqttVariableHeaderTopicNameChar2;
  topic[3] = '\0';

  value[0] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar0; 
  value[1] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar1; 
  value[2] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar2;
  value[3] = '\0';

  enqueue(queue, topic, value);

  pthread_mutex_unlock(&mutex);
  sem_post(&semQueueFull);

  // FIXME: CONNECT AUSWERTEN!!!
  /*
   for(int i = 0; i <sizeof(mqttControlPacketPublishBuffer); i++) {
    printf("%d %d\n", i, mqttControlPacketPublishBuffer[i]);
   }
  */
  /*
    recv() DISCONNECT
  */

  errno = 0;
  recv(clientSocketFDTmp, &mqttControlPacketDisconnectBuffer, sizeof(mqttControlPacketDisconnectBuffer), 0);
  if( errno != 0 ) { handle_error("receive() mqttControlPacketDisconnectBuffer"); };

  #ifdef DEBUG 
  printf("recv(): mqttControlPacketDisconnectBuffer\n");
  #endif

  // mqttControlPacketDisconnect vorbereiten und in Puffer zum Versand ablegen 
  mqttControlPacketDisconnectTpl *mqttControlPacketDisconnect = (mqttControlPacketDisconnectTpl*) malloc(sizeof(mqttControlPacketDisconnectTpl));
 
  index = 0;

  mqttControlPacketDisconnect->mqttFixedHeaderByte1 = mqttControlPacketDisconnectBuffer[index++];
  mqttControlPacketDisconnect->mqttFixedHeaderRemainingLength = mqttControlPacketDisconnectBuffer[index++];

  /*
  // FIXME: DISCONNECT AUSWERTEN!!!
  for(int i = 0; i <sizeof(mqttControlPacketDisconnectBuffer); i++) {
   printf("%d %d\n", i, mqttControlPacketDisconnectBuffer[i]);
  }
 
  */

  #ifdef DEBUG 
  printf("close(): clientThreadStruct->clientSocketFD\n");
  #endif

  errno = 0;
  close(clientThreadStruct->clientSocketFD);
  if( errno != 0 ) { handle_error("close(clientThreadStruct->clientSocketFD)"); };  
 }

 if(mqttFixedHeader.mqttFixedHeaderByte1Bits.mqttControlPacketType == SUBSCRIBE) {
  /*
   recv() SUBSCRIBE
  */

  #ifdef DEBUG 
  printf("recv(): mqttFixedHeaderBuffer -> SUBSCRIBE received\n");
  #endif 
  
  #ifdef DEBUG 
  printf("recv(): mqttControlPacketSubscribeBuffer\n");
  #endif

  errno = 0;
  recv(clientSocketFDTmp, &mqttControlPacketSubscribeBuffer, sizeof(mqttControlPacketSubscribeBuffer), 0);
  if( errno != 0 ) { handle_error("receive() mqttControlPacketSubscribeBuffer"); };

  // mqttControlPacketSubscribe vorbereiten und in Puffer zum Versand ablegen 
  mqttControlPacketSubscribeTpl *mqttControlPacketSubscribe = (mqttControlPacketSubscribeTpl*) malloc(sizeof(mqttControlPacketSubscribeTpl));
  
  index = 0;
 
  mqttControlPacketSubscribe->mqttFixedHeaderRemainingLength = mqttControlPacketSubscribeBuffer[index++];
  mqttControlPacketSubscribe->mqttVariableHeaderPacketIdentifierMSB = mqttControlPacketSubscribeBuffer[index++];
  mqttControlPacketSubscribe->mqttVariableHeaderPacketIdentifierLSB = mqttControlPacketSubscribeBuffer[index++];
  mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterMSB = mqttControlPacketSubscribeBuffer[index++];
  mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterLSB = mqttControlPacketSubscribeBuffer[index++];
  mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterChar0 = mqttControlPacketSubscribeBuffer[index++];
  mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterChar1 = mqttControlPacketSubscribeBuffer[index++];
  mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterChar2 = mqttControlPacketSubscribeBuffer[index++];
  mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterQos = mqttControlPacketSubscribeBuffer[index];
 
  printf("%c%c%c\n",  
   mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterChar0, 
   mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterChar1, 
   mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterChar2
  );
 
  // QUEUE DEQUEUE
  // SPÄTER MIT SEMAPHOREN ARBEITEN

  /*
   send() SUBACK
  */
  
  // mqttControlPacketSuback vorbereiten für den Versand
  mqttControlPacketSubackTpl *mqttControlPacketSuback = (mqttControlPacketSubackTpl*) malloc(sizeof(mqttControlPacketSubackTpl));
 
  index = 0;

  mqttControlPacketSuback->mqttFixedHeaderByte1Bits.mqttControlPacketFlags = 0;
  mqttControlPacketSuback->mqttFixedHeaderByte1Bits.mqttControlPacketType = SUBACK;
  mqttControlPacketSubackBuffer[index++] = mqttControlPacketSuback->mqttFixedHeaderByte1;
  mqttControlPacketSuback->mqttFixedHeaderRemainingLength = 3;
  mqttControlPacketSubackBuffer[index++] = mqttControlPacketSuback->mqttFixedHeaderRemainingLength;
  mqttControlPacketSuback->mqttVariableHeaderPacketIdentifierMSB = 0;
  mqttControlPacketSubackBuffer[index++] = mqttControlPacketSuback->mqttVariableHeaderPacketIdentifierMSB;
  mqttControlPacketSuback->mqttVariableHeaderPacketIdentifierLSB = 1;
  mqttControlPacketSubackBuffer[index++] = mqttControlPacketSuback->mqttVariableHeaderPacketIdentifierLSB;
  mqttControlPacketSuback->mqttVariableHeaderSubackReturncode = 0;
  mqttControlPacketSubackBuffer[index++] = mqttControlPacketSuback->mqttVariableHeaderSubackReturncode;

  errno = 0;
  send(clientSocketFDTmp, &mqttControlPacketSubackBuffer, sizeof(mqttControlPacketSubackBuffer), 0);
  if( errno != 0 ) { handle_error("send() mqttControlPacketSubackBuffer"); };
  
  /*
   send() PUBLISH von dequeue
  */

  // FIXMXE: So lange bis ein Disconnect Paket ankommt, bis die Semaphore freigegeben worden ist auf Queue
  while(1) {
   sem_wait(&semQueueFull);
   pthread_mutex_lock(&mutex);

   // mqttControlPacketPublish vorbereiten und in Puffer zum Versand ablegen (+1 weil wir hier das erste Byte vom Header wieder braucehn)
   mqttFixedHeaderTpl *mqttFixedHeader = (mqttFixedHeaderTpl*) malloc(sizeof(mqttFixedHeaderTpl));

   // mqttControlPacketPublish vorbereiten und in Puffer zum Versand ablegen (+1 weil wir hier das erste Byte vom Header wieder braucehn)
   mqttControlPacketPublishTpl *mqttControlPacketPublish = (mqttControlPacketPublishTpl*) malloc(sizeof(mqttControlPacketPublishTpl));

   index = 0;
 
   mqttFixedHeader->mqttFixedHeaderByte1Bits.mqttControlPacketFlags = 0;
   mqttFixedHeader->mqttFixedHeaderByte1Bits.mqttControlPacketType = PUBLISH;
   mqttControlPacketPublishSendBuffer[index++] = mqttFixedHeader->mqttFixedHeaderByte1;
   
   mqttControlPacketPublish->mqttFixedHeaderRemainingLength = 8;
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttFixedHeaderRemainingLength;
 
   mqttControlPacketPublish->mqttVariableHeaderTopicNameLSB = 0;
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderTopicNameLSB;
 
   mqttControlPacketPublish->mqttVariableHeaderTopicNameMSB = 3;
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderTopicNameMSB;
 
   mqttControlPacketPublish->mqttVariableHeaderTopicNameChar0 = 't';
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderTopicNameChar0;
 
   mqttControlPacketPublish->mqttVariableHeaderTopicNameChar1 = '0';
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderTopicNameChar1;
 
   mqttControlPacketPublish->mqttVariableHeaderTopicNameChar2 = '1';
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderTopicNameChar2;
 
   mqttControlPacketPublish->mqttVariableHeaderPayloadChar0 = '+';
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar0;
 
   mqttControlPacketPublish->mqttVariableHeaderPayloadChar1 = '1';
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar1;
 
   mqttControlPacketPublish->mqttVariableHeaderPayloadChar2 = '5';
   mqttControlPacketPublishSendBuffer[index] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar2;
 
   // FIXME: DEQUEUE
 
   errno = 0;
   send(clientSocketFDTmp, &mqttControlPacketPublishSendBuffer, sizeof(mqttControlPacketPublishSendBuffer), 0);
   if( errno != 0 ) { handle_error("send() mqttControlPacketPublishSendBuffer"); };
   
   sem_post(&semQueueEmpty);
   pthread_mutex_unlock(&mutex);
  }
  // mqttControlPacketDisconnect vorbereiten und in Puffer zum Versand ablegen 
  mqttControlPacketDisconnectTpl *mqttControlPacketDisconnect = (mqttControlPacketDisconnectTpl*) malloc(sizeof(mqttControlPacketDisconnectTpl));
 }
 /*
 if( errno != 0 ) { handle_error("send() mqttControlPacketSubackBuffer"); };
 switch(mqttControlPacketSubackBuffer[4]) {
  case 128:
   errno=1;
   if( errno != 0 ) { handle_error("recv() mqttControlPacketSuback failed"); }; 
  break;
  case 2:
   errno=1;
   if( errno != 0 ) { handle_error("recv() mqttControlPacketSuback QoS 2 is not supported"); }; 
  break;
  case 1:
   errno=1;
   if( errno != 0 ) { handle_error("recv() mqttControlPacketSuback QoS 1 is not supported"); }; 
  break;
 }
 */
}

bool mqttControlPacketConnectCheckMQTT(char mqttVariableHeaderProtocolNameChar0, char mqttVariableHeaderProtocolNameChar1, char mqttVariableHeaderProtocolNameChar2, char mqttVariableHeaderProtocolNameChar3) {
   if(mqttVariableHeaderProtocolNameChar0 != 'M' || mqttVariableHeaderProtocolNameChar1 != 'Q' || mqttVariableHeaderProtocolNameChar2 != 'T' || mqttVariableHeaderProtocolNameChar3 != 'T') {
    return false;
   }
   return true; 
}