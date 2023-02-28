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
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#define DEBUG

const char    *SERVER_IP = "192.168.0.2";
const char    *SERVER_IF = "eth0";
const uint16_t SERVER_PORT = 1883;
const uint8_t  SEM_WAIT = 100;

// Aus bind(2) Beispiel 
#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

void *clientThread(void*);

void *subscriberRequestThread(void*);

int checkMqttControlPacketConnect(mqttControlPacketConnectTpl *mqttControlPacketConnect);

int checkMqttControlPacketPublish(mqttControlPacketPublishTpl *mqttControlPacketPublish);

int checkMqttControlPacketDisconnect(mqttControlPacketDisconnectTpl *mqttControlPacketDisconnect);

typedef struct clientThreadStructTpl {
  int clientSocketFD;
  Queue *queue;
} clientThreadStructTpl;

typedef struct subscriberRequestThreadStructTpl {
  int clientSocketFD;
} subscriberRequestThreadStructTpl;

sem_t semQueueFull, semQueueEmpty;
pthread_mutex_t mutex, mutexSubscriberRecv;

// https://github.com/pasce/daemon-skeleton-linux-c
static void skeleton_daemon() {
 pid_t pid;
  
 /* Fork off the parent process */
 pid = fork();
 
 /* An error occurred */
 if (pid < 0)
  exit(EXIT_FAILURE);
 
 /* Success: Let the parent terminate */
 if (pid > 0)
  exit(EXIT_SUCCESS);
    
 /* On success: The child process becomes session leader */
 if (setsid() < 0)
  exit(EXIT_FAILURE);
    
 /* Catch, ignore and handle signals */
 /*TODO: Implement a working signal handler */
 signal(SIGCHLD, SIG_IGN);
 signal(SIGHUP, SIG_IGN);
    
 /* Fork off for the second time*/
 pid = fork();
    
 /* An error occurred */
 if (pid < 0)
  exit(EXIT_FAILURE);
    
 /* Success: Let the parent terminate */
 if (pid > 0)
  exit(EXIT_SUCCESS);
    
 /* Set new file permissions */
 umask(0);
    
 /* Change the working directory to the root directory */
 /* or another appropriated directory */
 chdir("/");
    
 /* Close all open file descriptors */
 int x;
 for (x = sysconf(_SC_OPEN_MAX); x>=0; x--) {
  close (x);
 }
    
 /* Open the log file */
 openlog ("mqtt_server", LOG_PID, LOG_DAEMON);
}

int main(int argc, char *argv) {
 // syslog(LOG_NOTICE, "starting...");
 // skeleton_daemon();
 int serverSocketFD, clientSocketFD;
 struct sockaddr_in serverSocketSettings, clientSocketSettings;

 // Producer Consumer auf Basis von http://shivammitra.com/c/producer-consumer-problem-in-c/#
 pthread_mutex_init(&mutex, NULL);
 pthread_mutex_init(&mutexSubscriberRecv, NULL);
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

  pthread_t threadId = (pthread_t) calloc(1, sizeof(pthread_t));
  errno = 0;
  pthread_create( &threadId, NULL, clientThread, (void *) &clientThreadStruct);
  if( errno != 0 ) { handle_error("pthread_create()"); };
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
 printf("debug: pthread_mutex_destroy(&mutexSubscriberRecv)\n");
 #endif
 errno = 0;
 pthread_mutex_destroy(&mutexSubscriberRecv);
 if( errno != 0 ) { handle_error("pthread_mutex_destroy(&mutexSubscriberRecv)"); };

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

/* 
 Subscriber Request Thread
*/  
void *subscriberRequestThread(void *arg) {
 signal(SIGPIPE, SIG_IGN);

 // ###################### 
 // ### Vordefiniertes ###  
 // ###################### 

 // Struktur für den subscriberRequestThreadStruct wird als Argument übergeben
 subscriberRequestThreadStructTpl *subscriberRequestThreadStruct = (subscriberRequestThreadStructTpl *) arg;

 // In der Struktur befindet sich der Client-Socket
 int clientSocketFDTmp = subscriberRequestThreadStruct->clientSocketFD;

 // Dynamische Längen sind in unserem Anwendungsfall nicht notwendig, daher werden die Puffer-Längen statisch definiert
 uint8_t mqttFixedHeaderBuffer[1], mqttControlPacketPingreqBuffer[0], mqttControlPacketPingrespBuffer[2], mqttControlPacketDisconnectBuffer[1];
 
 // Prüft ob ein Disconnect Paket empfangen wurde
 bool disconnectReceived = false;

 // Index für die Strukturen
 int index = 0;

 while(1) {  
  #ifdef DEBUG 
  printf("debug: starting subscriberRequestThread loop\n");
  #endif
  
  // ###################### 
  // ### recv() FIXEDHD ###  
  // ###################### 

  // Es wird nur das erste Byte vom nächsten Paket gelesen. In dem Byte wird dann unterschieden, ob es sich um 
  // ein PINGREQ- oder DISCONNECT-Paket handelt. Andere Paket-Typen werden abgelehnt. 
  #ifdef DEBUG 
  printf("debug: recv(): mqttFixedHeaderBuffer\n");
  #endif
  errno = 0;
  recv(clientSocketFDTmp, &mqttFixedHeaderBuffer, sizeof(mqttFixedHeaderBuffer), 0);
  if( errno != 0 ) { handle_error("receive() mqttFixedHeaderBuffer"); };

  pthread_mutex_lock(&mutexSubscriberRecv);

  // Erstes Byte vom FixedHeader lesen
  mqttFixedHeaderTpl mqttFixedHeader;
  mqttFixedHeader.mqttFixedHeaderByte1 = mqttFixedHeaderBuffer[0];

  // Prüfen ob es sich um ein PINGREQ-Paket handelt
  if(mqttFixedHeader.mqttFixedHeaderByte1Bits.mqttControlPacketType == PINGREQ) {
   // Restliche Daten vom PINGREQ, mit recv() in Puffer lesen. Fehler abfangen.
   #ifdef DEBUG 
   printf("debug: recv(): mqttControlPacketPingreqBuffer\n");
   #endif
   errno = 0;
   recv(clientSocketFDTmp, &mqttControlPacketPingreqBuffer, sizeof(mqttControlPacketPingreqBuffer), 0);
   if( errno != 0 ) { handle_error("recv() mqttControlPacketPingreqBuffer"); };

   // Speicher für die MQTT-Pingreq Struktur reservieren und den Puffer dort abspeichern.
   mqttControlPacketPingreqTpl *mqttControlPacketPingreq = (mqttControlPacketPingreqTpl*) calloc(1, sizeof(mqttControlPacketPingreqTpl)); 
   mqttControlPacketPingreq->mqttFixedHeaderByte1 = mqttFixedHeader.mqttFixedHeaderByte1;
   mqttControlPacketPingreq->mqttFixedHeaderRemainingLength = mqttControlPacketPingreqBuffer[0];
   
   // mqttControlPacketResp vorbereiten und in Puffer zum Versand ablegen
   // PINGRESP ist notwendig für Telegraf: 2023-02-20T19:53:29Z E! [inputs.mqtt_consumer] Error in plugin: connection lost: pingresp not received, disconnecting
   // FIXME PINGRESP muss in einem weiteren Thread gestartet werden. Bestenfalls für die Subscriber einen Publish- und einen Pingresp-Thread. 
   mqttControlPacketPingrespTpl *mqttControlPacketPingresp = (mqttControlPacketPingrespTpl*) calloc(1, sizeof(mqttControlPacketPingrespTpl));
 
   mqttControlPacketPingresp->mqttFixedHeaderByte1Bits.mqttControlPacketFlags = 0;
   mqttControlPacketPingresp->mqttFixedHeaderByte1Bits.mqttControlPacketType = PINGRESP;
   mqttControlPacketPingrespBuffer[index++] = mqttControlPacketPingresp->mqttFixedHeaderByte1;
   
   mqttControlPacketPingresp->mqttFixedHeaderRemainingLength = 0;
   mqttControlPacketPingrespBuffer[index++] = mqttControlPacketPingresp->mqttFixedHeaderRemainingLength;

   send(clientSocketFDTmp, &mqttControlPacketPingrespBuffer, sizeof(mqttControlPacketPingrespBuffer), 0);

   free(mqttControlPacketPingreq);
   free(mqttControlPacketPingresp);
  }
  else if(mqttFixedHeader.mqttFixedHeaderByte1Bits.mqttControlPacketType == DISCONNECT) {  
   // ######################### 
   // ### recv() DISCONNECT ###  
   // #########################

   // Zum Schluss wird die DISCONNECT-Nachricht gelesen, geprüft und die Verbindung terminiert.
   #ifdef DEBUG 
   printf("debug: recv(): mqttControlPacketDisconnectBuffer\n");
   #endif

   /* Deaktiviert da der Server nicht bei Broken Pipe abstürzen soll, wenn die Gegenseite schon geschlossen ist
   errno = 0;
   if( errno != 0 ) { handle_error("receive() mqttControlPacketDisconnectBuffer"); };
   */ 

   recv(clientSocketFDTmp, &mqttControlPacketDisconnectBuffer, sizeof(mqttControlPacketDisconnectBuffer), 0);
 
   // mqttControlPacketDisconnect vorbereiten und in Puffer zum Versand ablegen 
   mqttControlPacketDisconnectTpl *mqttControlPacketDisconnect = (mqttControlPacketDisconnectTpl*) calloc(1, sizeof(mqttControlPacketDisconnectTpl));
   mqttControlPacketDisconnect->mqttFixedHeaderByte1 = mqttFixedHeader.mqttFixedHeaderByte1;
   mqttControlPacketDisconnect->mqttFixedHeaderRemainingLength = mqttControlPacketDisconnectBuffer[1];

   // Mit der Struktur können die Daten einfach zerlegt und geprüft werden. Dies geschieht 
   // in der Funktion checkMqttControlPacketDisconnect(). Schlägt diese Prüfung fehl, wird 
   // das Paket ignoriert und die Verbindung trotzdem terminiert.
   if(checkMqttControlPacketDisconnect(mqttControlPacketDisconnect) != 0) {
    #ifdef DEBUG 
    printf("debug: checkMqttControlPacketDisconnect(): nicht konform.\n");
    #endif
    disconnectReceived = true;
   }
   else {
    disconnectReceived = true;
   }
   free(mqttControlPacketDisconnect);
  }
  pthread_mutex_unlock(&mutexSubscriberRecv);
  if(disconnectReceived) {
   #ifdef DEBUG 
   printf("debug: stopping subscriberRequestThread loop, close socket\n");
   #endif
   close(clientSocketFDTmp);   
   break;
  }
 }
}

/* 
 Client Thread (Publisher / Subscriber)
*/  
void *clientThread(void *arg) {
 signal(SIGPIPE, SIG_IGN);
 // ###################### 
 // ### Vordefiniertes ###  
 // ###################### 

 // Struktur für den ClientThread wird als Argument übergeben
 clientThreadStructTpl *clientThreadStruct = (clientThreadStructTpl*) calloc(1, sizeof(clientThreadStructTpl));
 clientThreadStruct = arg;

 // In der Struktur befindet sich der Client-Socket
 int clientSocketFDTmp = clientThreadStruct->clientSocketFD;
 
 #ifdef DEBUG 
 printf("debug: socket %d an clientSocketSDTmp zugewiesen aus Struktur clientThreadStruct->clientSocketFD\n", clientSocketFDTmp);
 #endif

 // In der Queue werden:
 // - eingehende PUBLISH-Nachrichten (Publisher) abgelegt
 // - ausgehende PUBLISH-Nachrichten (Subscriber) entfernt
 Queue *queue = clientThreadStruct->queue;

 // Der Index dient zum Inkrementieren der Puffer-Elemente
 int index = 0;

 // Dynamische Längen sind in unserem Anwendungsfall nicht notwendig, daher werden die Puffer-Längen statisch definiert
 uint8_t mqttFixedHeaderBuffer[1], mqttControlPacketConnectBuffer[17], mqttControlPacketConnectackBuffer[4], mqttControlPacketPublishBuffer[14], mqttControlPacketPublishSendBuffer[15], mqttControlPacketSubscribeBuffer[9], mqttControlPacketSubackBuffer[5];
 
 // ###################### 
 // ### recv() CONNECT ###  
 // ###################### 
 
 // Erstes Paket, üblicherweise CONNECT, mit recv() in Puffer lesen. Fehler abfangen.
 #ifdef DEBUG 
 printf("debug: recv(): mqttControlPacketConnectBuffer\n");
 #endif
 errno = 0;
 recv(clientSocketFDTmp, &mqttControlPacketConnectBuffer, sizeof(mqttControlPacketConnectBuffer), 0);
 if( errno != 0 ) { handle_error("send() mqttControlPacketConnectBuffer"); };

 // Speicher für die MQTT-Connect Struktur reservieren und den Puffer dort abspeichern.
 mqttControlPacketConnectTpl *mqttControlPacketConnect = (mqttControlPacketConnectTpl*) calloc(1, sizeof(mqttControlPacketConnectTpl)); 
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

 // Mit der Struktur können die Daten einfach zerlegt und geprüft werden. Dies geschieht 
 // in der Funktion checkMqttControlPacketConnect(). Schlägt diese Prüfung fehl, wird hier
 // schon die Verbindung beendet und Speicher wieder freigegeben.
 if(checkMqttControlPacketConnect(mqttControlPacketConnect) != 0) {
  close(clientSocketFDTmp);
  free(mqttControlPacketConnect);
  #ifdef DEBUG 
  printf("debug: checkMqttControlPacketConnect(): nicht konform.\n");
  #endif
  return NULL;
 }

 // Ist die Prüfung der MQTT Connect Struktur in Ordnung, muss nur noch der Speicher dieser Struktur 
 // freigegeben werden. Der Index kann für das nächste Paket auch wieder zurückgesetzt werden.
 free(mqttControlPacketConnect);
 index = 0;

 // ###################### 
 // ### send() CONNACK ###  
 // ###################### 

 // Speicher für die MQTT-Connectack Struktur reservieren, definieren und die Daten dem Puffer zum Versenden zuweisen.
 mqttControlPacketConnectackTpl *mqttControlPacketConnectack = (mqttControlPacketConnectackTpl*) calloc(1, sizeof(mqttControlPacketConnectackTpl));
 mqttControlPacketConnectack->mqttFixedHeaderByte1Bits.mqttControlPacketFlags = 0;
 mqttControlPacketConnectack->mqttFixedHeaderByte1Bits.mqttControlPacketType = CONNACK;
 mqttControlPacketConnectackBuffer[index++] = mqttControlPacketConnectack->mqttFixedHeaderByte1;
 mqttControlPacketConnectack->mqttFixedHeaderRemainingLength = 2;
 mqttControlPacketConnectackBuffer[index++] = mqttControlPacketConnectack->mqttFixedHeaderRemainingLength;
 mqttControlPacketConnectack->mqttVariableHeaderConnectackFlags = 0;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnectack->mqttVariableHeaderConnectackFlags;
 mqttControlPacketConnectack->mqttVariableHeaderConnectackReturncode = 0;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnectack->mqttVariableHeaderConnectackReturncode;

 // Mit dem Versand des CONNACK Paket wird das konforme CONNECT Paket bestätigt. Fehler abfangen.
 #ifdef DEBUG 
 printf("debug: send(): mqttControlPacketConnectackBuffer\n");
 #endif
 errno = 0;
 send(clientSocketFDTmp, &mqttControlPacketConnectackBuffer, sizeof(mqttControlPacketConnectackBuffer), 0);
 if( errno != 0 ) { handle_error("send() mqttControlPacketConnectackBuffer"); };

 // Nach dem Versenden vom CONNACK Paket, den Speicher wieder freigeben.
 free(mqttControlPacketConnectack);
 index = 0;

 // ###################### 
 // ### recv() FIXEDHD ###  
 // ###################### 

 // Es wird nur das erste Byte vom nächsten Paket gelesen. In dem Byte wird dann unterschieden, ob es sich um 
 // ein PUBLISHer-Client oder SUBSCRIBEr-Client handelt. Andere Paket-Typen werden abgelehnt. 
 #ifdef DEBUG 
 printf("debug: recv(): mqttFixedHeaderBuffer\n");
 #endif
 errno = 0;
 recv(clientSocketFDTmp, &mqttFixedHeaderBuffer, sizeof(mqttFixedHeaderBuffer), 0);
 if( errno != 0 ) { handle_error("receive() mqttFixedHeaderBuffer"); };

 // Erstes Byte vom FixedHeader lesen
 mqttFixedHeaderTpl mqttFixedHeader;
 mqttFixedHeader.mqttFixedHeaderByte1 = mqttFixedHeaderBuffer[0];

 // Prüfen ob es sich um ein PUBLISHer-Client handelt
 if(mqttFixedHeader.mqttFixedHeaderByte1Bits.mqttControlPacketType == PUBLISH) {
  // ###################### 
  // ### recv() PUBLISH ###  
  // ###################### 
 
  // Die restlichen Bytes der PUBLISH-Nachricht werden in den Puffer gelesen. 
  #ifdef DEBUG 
  printf("debug: recv(): mqttControlPacketPublishBuffer\n");
  #endif
  errno = 0;
  recv(clientSocketFDTmp, &mqttControlPacketPublishBuffer, sizeof(mqttControlPacketPublishBuffer), 0);
  if( errno != 0 ) { handle_error("receive() mqttControlPacketPublishBuffer"); };
 
  // Speicher für die MQTT-Publish Struktur reservieren und den Puffer dort abspeichern.
  mqttControlPacketPublishTpl *mqttControlPacketPublish = (mqttControlPacketPublishTpl*) calloc(1, sizeof(mqttControlPacketPublishTpl));
  mqttControlPacketPublish->mqttFixedHeaderRemainingLength = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderTopicNameLSB = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderTopicNameMSB = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderTopicNameChar0 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderTopicNameChar1 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderTopicNameChar2 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderPayloadChar0 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderPayloadChar1 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderPayloadChar2 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderPayloadChar3 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderPayloadChar4 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderPayloadChar5 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderPayloadChar6 = mqttControlPacketPublishBuffer[index++];
  mqttControlPacketPublish->mqttVariableHeaderPayloadChar7 = mqttControlPacketPublishBuffer[index];

  // Mit der Struktur können die Daten einfach zerlegt und geprüft werden. Dies geschieht 
  // in der Funktion checkMqttControlPacketPublish(). Schlägt diese Prüfung fehl, wird hier
  // schon die Verbindung beendet und Speicher wieder freigegeben.
  if(checkMqttControlPacketPublish(mqttControlPacketPublish) != 0) {
   close(clientSocketFDTmp);
   free(mqttControlPacketPublish);
   #ifdef DEBUG 
   printf("debug: checkMqttControlPacketPublish(): nicht konform.\n");
   #endif
   return NULL;
  }

  // Semaphore und gegenseitiger Ausschluss. Die Nachricht wird erst in die Queue geschrieben, wenn
  // weniger als SEM_WAIT (Default: 10) Elemente belegt sind. Dadurch werden Speicherüberläufe
  // vermieden.
  sem_wait(&semQueueEmpty);
  pthread_mutex_lock(&mutex);
 
  // Speicher fuer topic und value reservieren
  char *topic = (char*)calloc(1, 4);
  char *value = (char*)calloc(1, 9);


  // Für den Topic werden statisch 3 Zeichen verwendet und mit \0 terminiert. Dadurch, dass statische
  // Längen verwendet werden, ist die Implementierung einfacher und die Laufzeit schneller, da der 
  // in der Spezifikation angebene Algorithmus zum Berechnen der Längen nicht verwendet wird.
  topic[0] = mqttControlPacketPublish->mqttVariableHeaderTopicNameChar0; 
  topic[1] = mqttControlPacketPublish->mqttVariableHeaderTopicNameChar1; 
  topic[2] = mqttControlPacketPublish->mqttVariableHeaderTopicNameChar2;
  topic[3] = '\0';

  // Analog zu dem Topic gilt das Gleiche für den Value.
  value[0] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar0; 
  value[1] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar1; 
  value[2] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar2;
  value[3] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar3;
  value[4] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar4;
  value[5] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar5;
  value[6] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar6;
  value[7] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar7;
  value[8] = '\0';
 
  // Der Topic und Value werden in einem neuen Element in der Queue gespeichert. Es gilt FIFO.
  #ifdef DEBUG 
  printf("debug: enqueue(): topic: %s, value: %s.\n", topic, value);
  #endif
  enqueue(queue, topic, value);

  // Nachdem die Daten in der Queue abgelegt worden sind, werden Mutex und Semaphore wieder freigegeben.
  pthread_mutex_unlock(&mutex);
  sem_post(&semQueueFull);

  // Nach dem Verarbeiten vom eingheneden PUBLISH Paket, kann der Speicher wieder fregegeben werden.
  free(mqttControlPacketPublish);
  index = 0;
 
  // Socket schließen
  close(clientSocketFDTmp);

  // Speichere wieder freigeben
  free(value);
  free(topic);
 }
 // Prüfen ob es sich um ein SUBSCRIBer-Client handelt
 else if(mqttFixedHeader.mqttFixedHeaderByte1Bits.mqttControlPacketType == SUBSCRIBE) {
  // ######################## 
  // ### recv() SUBSCRIBE ###  
  // ######################## 

  #ifdef DEBUG 
  printf("debug: recv(): mqttFixedHeaderBuffer -> SUBSCRIBE received\n");
  #endif 
  
  #ifdef DEBUG 
  printf("debug: recv(): mqttControlPacketSubscribeBuffer\n");
  #endif

  errno = 0;
  recv(clientSocketFDTmp, &mqttControlPacketSubscribeBuffer, sizeof(mqttControlPacketSubscribeBuffer), 0);
  if( errno != 0 ) { handle_error("receive() mqttControlPacketSubscribeBuffer"); };

  // mqttControlPacketSubscribe vorbereiten und in Puffer zum Versand ablegen 
  mqttControlPacketSubscribeTpl *mqttControlPacketSubscribe = (mqttControlPacketSubscribeTpl*) calloc(1, sizeof(mqttControlPacketSubscribeTpl));
  
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
 
  free(mqttControlPacketSubscribe);

  /*
   send() SUBACK
  */
  
  // mqttControlPacketSuback vorbereiten für den Versand
  mqttControlPacketSubackTpl *mqttControlPacketSuback = (mqttControlPacketSubackTpl*) calloc(1, sizeof(mqttControlPacketSubackTpl));
 
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
  
  free(mqttControlPacketSuback);

  /*
   subscriberRequestThread starten, wartet auf PINGREQ / DISCONNECT
  */

  subscriberRequestThreadStructTpl subscriberRequestThreadStruct;
  subscriberRequestThreadStruct.clientSocketFD = clientSocketFDTmp;

  #ifdef DEBUG 
  printf("debug: socket %d an clientSocketSDTmp zugewiesen von Struktur subscriberRequestThreadStruct.clientSocketF\n", clientSocketFDTmp);
  #endif

  pthread_t threadId = (pthread_t) calloc(1, sizeof(pthread_t));
  errno = 0;
  pthread_create( &threadId, NULL, subscriberRequestThread, (void *) &subscriberRequestThreadStruct);
  if( errno != 0 ) { handle_error("pthread_create()"); };
  #ifdef DEBUG 
  printf("debug: pthread_create clientSocketFD\n"); 
  #endif

  /*
   send() PUBLISH von dequeue
  */

  while(1) {
   sem_wait(&semQueueFull);
   pthread_mutex_lock(&mutex);

   // mqttControlPacketPublish vorbereiten und in Puffer zum Versand ablegen (+1 weil wir hier das erste Byte vom Header wieder braucehn)
   mqttFixedHeaderTpl *mqttFixedHeader = (mqttFixedHeaderTpl*) calloc(1, sizeof(mqttFixedHeaderTpl));

   // mqttControlPacketPublish vorbereiten und in Puffer zum Versand ablegen (+1 weil wir hier das erste Byte vom Header wieder braucehn)
   mqttControlPacketPublishTpl *mqttControlPacketPublish = (mqttControlPacketPublishTpl*) calloc(1, sizeof(mqttControlPacketPublishTpl));

   index = 0;
 
   mqttFixedHeader->mqttFixedHeaderByte1Bits.mqttControlPacketFlags = 0;
   mqttFixedHeader->mqttFixedHeaderByte1Bits.mqttControlPacketType = PUBLISH;
   mqttControlPacketPublishSendBuffer[index++] = mqttFixedHeader->mqttFixedHeaderByte1;
   
   mqttControlPacketPublish->mqttFixedHeaderRemainingLength = 13;
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttFixedHeaderRemainingLength;
 
   mqttControlPacketPublish->mqttVariableHeaderTopicNameLSB = 0;
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderTopicNameLSB;
 
   mqttControlPacketPublish->mqttVariableHeaderTopicNameMSB = 3;
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderTopicNameMSB;

   // Speicher fuer topic und value reservieren
   char *topic = (char*)calloc(1, 3);
   char *value = (char*)calloc(1, 8);

   dequeue(queue, &topic, &value);

   #ifdef DEBUG 
   printf("debug: dequeue(): topic: %s, value: %s.\n", topic, value);
   #endif

   mqttControlPacketPublish->mqttVariableHeaderTopicNameChar0 = topic[0];
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderTopicNameChar0;
   mqttControlPacketPublish->mqttVariableHeaderTopicNameChar1 = topic[1];
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderTopicNameChar1;
   mqttControlPacketPublish->mqttVariableHeaderTopicNameChar2 = topic[2];
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderTopicNameChar2;
 
   mqttControlPacketPublish->mqttVariableHeaderPayloadChar0 = value[0];
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar0;
   mqttControlPacketPublish->mqttVariableHeaderPayloadChar1 = value[1];
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar1; 
   mqttControlPacketPublish->mqttVariableHeaderPayloadChar2 = value[2];  
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar2;
   mqttControlPacketPublish->mqttVariableHeaderPayloadChar3 = value[3];  
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar3;
   mqttControlPacketPublish->mqttVariableHeaderPayloadChar4 = value[4];  
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar4;
   mqttControlPacketPublish->mqttVariableHeaderPayloadChar5 = value[5];  
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar5;
   mqttControlPacketPublish->mqttVariableHeaderPayloadChar6 = value[6];  
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar6;
   mqttControlPacketPublish->mqttVariableHeaderPayloadChar7 = value[7];  
   mqttControlPacketPublishSendBuffer[index++] = mqttControlPacketPublish->mqttVariableHeaderPayloadChar7;

   /* Deaktiviert da der Server nicht bei Broken Pipe abstürzen soll, wenn die Gegenseite zuerst schließt  
   errno = 0;
   if( errno != 0 ) { handle_error("send() mqttControlPacketPublishSendBuffer"); };
   */

   errno = 0;
   send(clientSocketFDTmp, &mqttControlPacketPublishSendBuffer, sizeof(mqttControlPacketPublishSendBuffer), 0);
   if( errno != 0 ) { handle_error("send() mqttControlPacketSubackBuffer"); };  

   // Speichere wieder freigeben
   free(value);
   free(topic);
   free(mqttFixedHeader);
   free(mqttControlPacketPublish);

   pthread_mutex_unlock(&mutex);
   sem_post(&semQueueEmpty);
  }
 } 
 else {
  #ifdef DEBUG 
  printf("debug: close() ungültiger Paket-Typ.\n");
  #endif
  close(clientSocketFDTmp);
 }
}

int checkMqttControlPacketConnect(mqttControlPacketConnectTpl *mqttControlPacketConnect) {
 if(mqttControlPacketConnect->mqttFixedHeaderByte1Bits.mqttControlPacketType != CONNECT) {
  printf("%d != CONNECT(1). Beende Verbindung.\n", mqttControlPacketConnect->mqttFixedHeaderByte1Bits.mqttControlPacketType);
  return 1;
 }

 if(mqttControlPacketConnect->mqttFixedHeaderRemainingLength != 15) {
  printf("Die Paketlänge stimmt nicht, es wird eine RemainingLength von 15 erwarte.\n");
  return 1;
 }

 if(mqttControlPacketConnect->mqttVariableHeaderProtocolNameMSB != 0) {
  printf("mqttVariableHeaderProtocolNameLSB muss 0 sein.\n");
  return 1;
 }

 if(mqttControlPacketConnect->mqttVariableHeaderProtocolNameLSB != 4) {
  printf("mqttVariableHeaderProtocolNameMSB muss 4 sein.\n");
  return 1;
 }

 if(mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar0 != 'M' || mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar1 != 'Q' || mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar2 != 'T' || mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar3 != 'T' ) {
  printf("mqttVariableHeaderTopicNameChar0-3 muss MQTT beinhalten.\n");  
  return 1;
 }

 if(mqttControlPacketConnect->mqttVariableHeaderProtocolLevel != 4) {
  printf("mqttVariableHeaderProtocolLevel muss 4 sein, es wird nur MQTT 3.1.1 unterstützt\n");
  return 1;
 }

 if(mqttControlPacketConnect->mqttVariableHeaderConnectFlagsBits.mqttVariableHeaderConnectFlagsReserved != 0) {
  printf("mqttVariableHeaderConnectFlagsReserved muss 0 sein, ist reserviert.\n");
  return 1;
 }

 if(mqttControlPacketConnect->mqttVariableHeaderConnectFlagsBits.mqttVariableHeaderConnectFlagsCleanSession != 1) {
  printf("mqttVariableHeaderConnectFlagsCleanSession muss 1 sein, wir verwenden kein QoS 1 oder QoS 2. Für QoS 0 ist das Speichern optional.\n");
  return 1;
 }

 if(mqttControlPacketConnect->mqttVariableHeaderConnectFlagsBits.mqttVariableHeaderConnectFlagsWillFlag != 0) {
  printf("mqttVariableHeaderConnectFlagsWillFlag muss 0 sein, wir verwenden kein Will QoS / Retain.\n");
  return 1;
 }

 if(mqttControlPacketConnect->mqttVariableHeaderConnectFlagsBits.mqttVariableHeaderConnectFlagsWillQoS != 0) {
  printf("mqttVariableHeaderConnectFlagsWillQoS muss 0 sein, wir verwenden kein Will QoS / Retain.\n");
  return 1;
 }

 if(mqttControlPacketConnect->mqttVariableHeaderConnectFlagsBits.mqttVariableHeaderConnectFlagsWillRetain != 0) {
  printf("mqttVariableHeaderConnectFlagsWillRetain muss 0 sein, wir verwenden kein Will QoS / Retain.\n");
  return 1;
 }

 if(mqttControlPacketConnect->mqttVariableHeaderConnectFlagsBits.mqttVariableHeaderConnectFlagsPassword != 0) {
  printf("mqttVariableHeaderConnectFlagsPassword muss 0 sein, wir verwenden keine Authentifizierung, rein internes Netz.\n");
  return 1;
 }

 if(mqttControlPacketConnect->mqttVariableHeaderConnectFlagsBits.mqttVariableHeaderConnectFlagsUsername != 0) {
  printf("mqttVariableHeaderConnectFlagsUsername muss 0 sein, wir verwenden keine Authentifizierung, rein internes Netz.\n");
  return 1;
 }

 if(mqttControlPacketConnect->mqttVariableHeaderKeepAliveLSB != 0) {
  printf("mqttVariableHeaderKeepAliveLSB != 0, telegraf mqtt_consumer unterstützt kein keepalive von 0 wegen paho.mqtt.golang.\n");
  return 1;
 }

 if(mqttControlPacketConnect->mqttVariableHeaderKeepAliveMSB != 60) {
  printf("mqttVariableHeaderKeepAliveMSB != 60, telegraf mqtt_consumer unterstützt kein keepalive von 0 wegen paho.mqtt.golang.\n");
  return 1;
 }

 return 0;
}

int checkMqttControlPacketPublish(mqttControlPacketPublishTpl *mqttControlPacketPublish) {
 if(mqttControlPacketPublish->mqttFixedHeaderRemainingLength != 13) {
  printf("mqttFixedHeaderRemainingLength != 13, aus Performance-Gründen nur eine statische Länge\n");
  return 1;
 }

 if(mqttControlPacketPublish->mqttVariableHeaderTopicNameLSB != 0) {
  printf("mqttVariableHeaderTopicNameLSB != 0, aus Performance-Gründen nur eine statische Länge\n");
  return 1;
 }

 if(mqttControlPacketPublish->mqttVariableHeaderTopicNameMSB != 3) {
  printf("mqttVariableHeaderTopicNameMSB != 0, aus Performance-Gründen nur eine statische Länge\n");
  return 1;
 }
 
 return 0;
}

int checkMqttControlPacketDisconnect(mqttControlPacketDisconnectTpl *mqttControlPacketDisconnect) { 
 if(mqttControlPacketDisconnect->mqttFixedHeaderByte1 != 224) {
  printf("mqttFixedHeaderByte1 %d != 224, ungültiger Disconnect Header\n", mqttControlPacketDisconnect->mqttFixedHeaderByte1);
  return 1;
 }

 if(mqttControlPacketDisconnect->mqttFixedHeaderRemainingLength != 0) {
  printf("mqttFixedHeaderRemainingLength != 0, muss 0 sein bei Disconnect.\n");
  return 1;
 }

 return 0;
}
