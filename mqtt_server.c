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
#define DEBUG

const char    *SERVER_IP = "192.168.1.2";
const char    *SERVER_IF = "eth0";
const uint16_t SERVER_PORT = 1883;

// Aus bind(2) Beispiel 
#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

void *callBackConnectAck(void*);

int main(int argc, char *argv) {
 int serverSocketFD, clientSocketFD;
 struct sockaddr_in serverSocketSettings, clientSocketSettings;
 
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
 
 int i = 0;
 while(1) {
  errno = 0;
  clientSocketFD = accept(serverSocketFD, (struct sockaddr*)&clientSocketSettings, &clientSocketSettingsLength);
  if( errno != 0 ) { handle_error("accept() serverSocketFD"); };
  #ifdef DEBUG 
  printf("debug: accept() serverSocketFD\n"); 
  #endif

  pthread_t threadId = (pthread_t) malloc(sizeof(pthread_t));
  errno = 0;
  pthread_create( &threadId, NULL, callBackConnectAck, (void *) &clientSocketFD);
  if( errno != 0 ) { handle_error("close() clientSocketFD"); };
  #ifdef DEBUG 
  printf("debug: pthread_create clientSocketFD\n"); 
  #endif
 
  pthread_join(threadId, NULL);

  close(clientSocketFD);
  if( errno != 0 ) { handle_error("close() clientSocketFD"); };
  #ifdef DEBUG 
  printf("debug: close() clientSocketFD\n"); 
  #endif  
 }
 errno = 0;
 
 close(serverSocketFD);
 if( errno != 0 ) { handle_error("close() serverSocketFD"); };
 #ifdef DEBUG 
 printf("debug: close() serverSocketFD\n");
 #endif
 
 return EXIT_SUCCESS;
}

void *callBackConnectAck(void *clientSocketFD) {
 int clientSocketFDTmp = *(int*) clientSocketFD;

 uint8_t mqttControlPacketConnectBuffer[17];
 uint32_t mqttControlPacketConnectackBuffer;
 
 errno = 0;
 recv(clientSocketFDTmp, &mqttControlPacketConnectBuffer, sizeof(mqttControlPacketConnectBuffer), 0);

 // mqttControlPacketConnectCheckMQTT()
 for(int i = 0; i <sizeof(mqttControlPacketConnectBuffer); i++) {
   printf("%d %d\n", i, mqttControlPacketConnectBuffer[i]);
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