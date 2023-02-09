#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "mqtt_structs.h"
#include <signal.h>

// Signal Handler für CTRL-C, damit das Disconnect Paket noch gesendet wird
// https://stackoverflow.com/questions/4217037/catch-ctrl-c-in-c
// Tag: C001
static volatile sig_atomic_t keep_running = 1;

// Tag: C001
static void sig_handler(int _) {
    (void)_;
    printf("signal handler executed\n");
    keep_running = 0;
}

/*
 testType->zahl32 = atoi(argv[1]);
 testType->zahl16 = atoi(argv[2]);
 testType->zahl8 = atoi(argv[3]);
*/

const char    *SERVER_IP = "192.168.1.2";
const uint16_t SERVER_PORT = 1883;

// Aus bind(2) Beispiel 
#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char *argv[]) {
 // Tag: C001
 signal(SIGINT, sig_handler);

 // Puffer definieren
 uint8_t mqttControlPacketConnectBuffer[17], mqttControlPacketSubscribeBuffer[10], mqttControlPacketPublishBuffer[10], mqttControlPacketSubackBuffer[5], mqttControlPacketConnectackBuffer[4], mqttControlPacketDisconnectBuffer[2], mqttControlPacketUnsubackBuffer[4];
 
 // Index für Puffer
 int index;
 
 // mqttControlPacketConnect vorbereiten und in Puffer zum Versand ablegen  
 mqttControlPacketConnectTpl *mqttControlPacketConnect = (mqttControlPacketConnectTpl*) malloc(sizeof(mqttControlPacketConnectTpl));

 index = 0;

 mqttControlPacketConnect->mqttFixedHeaderByte1Bits.mqttControlPacketFlags = 0;
 mqttControlPacketConnect->mqttFixedHeaderByte1Bits.mqttControlPacketType = CONNECT;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->mqttFixedHeaderByte1;
 
 mqttControlPacketConnect->mqttFixedHeaderRemainingLength = 15;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->mqttFixedHeaderRemainingLength;

 mqttControlPacketConnect->mqttVariableHeaderProtocolNameMSB = 0;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->mqttVariableHeaderProtocolNameMSB;

 mqttControlPacketConnect->mqttVariableHeaderProtocolNameLSB = 4;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->mqttVariableHeaderProtocolNameLSB;

 mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar0 = 'M';
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar0;
 
 mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar1 = 'Q';
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar1;
 
 mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar2 = 'T';
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar2;
 
 mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar3 = 'T';
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->mqttVariableHeaderProtocolNameChar3;
 
 mqttControlPacketConnect->mqttVariableHeaderProtocolLevel = 4;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->mqttVariableHeaderProtocolLevel;

 mqttControlPacketConnect->mqttVariableHeaderConnectFlagsBits.mqttVariableHeaderConnectFlagsUsername = 0;
 mqttControlPacketConnect->mqttVariableHeaderConnectFlagsBits.mqttVariableHeaderConnectFlagsPassword = 0;
 mqttControlPacketConnect->mqttVariableHeaderConnectFlagsBits.mqttVariableHeaderConnectFlagsWillRetain = 0;
 mqttControlPacketConnect->mqttVariableHeaderConnectFlagsBits.mqttVariableHeaderConnectFlagsWillFlag = 0;
 mqttControlPacketConnect->mqttVariableHeaderConnectFlagsBits.mqttVariableHeaderConnectFlagsCleanSession = 1;
 mqttControlPacketConnect->mqttVariableHeaderConnectFlagsBits.mqttVariableHeaderConnectFlagsReserved = 0;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->mqttVariableHeaderConnectFlags;

 mqttControlPacketConnect->mqttVariableHeaderKeepAliveLSB = 0;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->mqttVariableHeaderKeepAliveLSB;

 mqttControlPacketConnect->mqttVariableHeaderKeepAliveMSB = 0;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->mqttVariableHeaderKeepAliveMSB;

 mqttControlPacketConnect->clientIdMSB = 0;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->clientIdMSB;

 mqttControlPacketConnect->clientIdLSB = 3;
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->clientIdLSB;

 mqttControlPacketConnect->clientIdChar0 = 's';
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->clientIdChar0;

 mqttControlPacketConnect->clientIdChar1 = '0';
 mqttControlPacketConnectBuffer[index++] = mqttControlPacketConnect->clientIdChar1;

 mqttControlPacketConnect->clientIdChar2 = '1';
 mqttControlPacketConnectBuffer[index] = mqttControlPacketConnect->clientIdChar2;

 // mqttControlPacketConnack vorbereiten zum vergleichen mit der Antwort vom Server
 mqttControlPacketConnectackTpl *mqttControlPacketConnectack = (mqttControlPacketConnectackTpl*) malloc(sizeof(mqttControlPacketConnectackTpl));

 index = 0;

 mqttControlPacketConnectack->mqttFixedHeaderByte1Bits.mqttControlPacketFlags = 0;
 mqttControlPacketConnectack->mqttFixedHeaderByte1Bits.mqttControlPacketType = CONNACK;
 mqttControlPacketConnectackBuffer[index++] = mqttControlPacketConnectack->mqttFixedHeaderByte1;

 mqttControlPacketConnectack->mqttFixedHeaderRemainingLength = 2;
 mqttControlPacketConnectackBuffer[index++] = mqttControlPacketConnectack->mqttFixedHeaderRemainingLength;

 mqttControlPacketConnectack->mqttVariableHeaderConnectackFlags = 0;
 mqttControlPacketConnectackBuffer[index++] = mqttControlPacketConnectack->mqttVariableHeaderConnectackFlags;

 mqttControlPacketConnectack->mqttVariableHeaderConnectackReturncode = 0;
 mqttControlPacketConnectackBuffer[index++] = mqttControlPacketConnectack->mqttVariableHeaderConnectackReturncode;

 // mqttControlPacketSubscribe vorbereiten und in Puffer zum Versand ablegen 
 mqttControlPacketSubscribeTpl *mqttControlPacketSubscribe = (mqttControlPacketSubscribeTpl*) malloc(sizeof(mqttControlPacketSubscribeTpl));
 
 index = 0;

 mqttControlPacketSubscribe->mqttFixedHeaderByte1Bits.mqttControlPacketFlags = 2;
 mqttControlPacketSubscribe->mqttFixedHeaderByte1Bits.mqttControlPacketType = SUBSCRIBE;
 mqttControlPacketSubscribeBuffer[index++] = mqttControlPacketSubscribe->mqttFixedHeaderByte1;

 mqttControlPacketSubscribe->mqttFixedHeaderRemainingLength = 8;
 mqttControlPacketSubscribeBuffer[index++] = mqttControlPacketSubscribe->mqttFixedHeaderRemainingLength ;

 mqttControlPacketSubscribe->mqttVariableHeaderPacketIdentifierMSB = 0;
 mqttControlPacketSubscribeBuffer[index++] = mqttControlPacketSubscribe->mqttVariableHeaderPacketIdentifierMSB;

 mqttControlPacketSubscribe->mqttVariableHeaderPacketIdentifierLSB = 1;
 mqttControlPacketSubscribeBuffer[index++] = mqttControlPacketSubscribe->mqttVariableHeaderPacketIdentifierLSB;

 mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterMSB = 0;
 mqttControlPacketSubscribeBuffer[index++] = mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterMSB;

 mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterLSB = 3;
 mqttControlPacketSubscribeBuffer[index++] = mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterLSB;

 mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterChar0 = 't';
 mqttControlPacketSubscribeBuffer[index++] = mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterChar0;

 mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterChar1 = '0';
 mqttControlPacketSubscribeBuffer[index++] = mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterChar1;

 mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterChar2 = '1';
 mqttControlPacketSubscribeBuffer[index++] = mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterChar2;

 mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterQos = 0;
 mqttControlPacketSubscribeBuffer[index++] = mqttControlPacketSubscribe->mqttVariableHeaderTopicFilterQos;

 // mqttControlPacketUnsuback vorbereiten zum Vergleichen mit Server
 mqttControlPacketUnsubackTpl *mqttControlPacketUnsuback = (mqttControlPacketUnsubackTpl*) malloc(sizeof(mqttControlPacketUnsubackTpl));
 
 index = 0;

 mqttControlPacketUnsuback->mqttFixedHeaderByte1Bits.mqttControlPacketFlags = 0;
 mqttControlPacketUnsuback->mqttFixedHeaderByte1Bits.mqttControlPacketType = UNSUBACK;
 mqttControlPacketUnsubackBuffer[index++] = mqttControlPacketUnsuback->mqttFixedHeaderByte1;
 mqttControlPacketUnsuback->mqttFixedHeaderRemainingLength = 0;
 mqttControlPacketUnsubackBuffer[index++] = mqttControlPacketUnsuback->mqttFixedHeaderRemainingLength;

 mqttControlPacketUnsuback->mqttVariableHeaderPacketIdentifierMSB = 0;
 mqttControlPacketUnsubackBuffer[index++] = mqttControlPacketUnsuback->mqttVariableHeaderPacketIdentifierMSB;

 mqttControlPacketUnsuback->mqttVariableHeaderPacketIdentifierLSB = 1;
 mqttControlPacketUnsubackBuffer[index++] = mqttControlPacketUnsuback->mqttVariableHeaderPacketIdentifierLSB;

 // mqttControlPacketDisconnect vorbereiten und in Puffer zum Versand ablegen 
 mqttControlPacketDisconnectTpl *mqttControlPacketDisconnect = (mqttControlPacketDisconnectTpl*) malloc(sizeof(mqttControlPacketDisconnectTpl));
 
 index = 0;

 mqttControlPacketDisconnect->mqttFixedHeaderByte1Bits.mqttControlPacketFlags = 0;
 mqttControlPacketDisconnect->mqttFixedHeaderByte1Bits.mqttControlPacketType = DISCONNECT;
 mqttControlPacketDisconnectBuffer[index++] = mqttControlPacketDisconnect->mqttFixedHeaderByte1;
 mqttControlPacketDisconnect->mqttFixedHeaderRemainingLength = 0;
 mqttControlPacketDisconnectBuffer[index++] = mqttControlPacketDisconnect->mqttFixedHeaderRemainingLength;

 int serverSocketFD, clientSocketFD;
 struct sockaddr_in serverSocketSettings, clientSocketSettings;
 
 // serverSocketFD erstellen
 errno = 0;
 serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
 if ( errno != 0 ) { handle_error("serverSocketFD socket()"); } 

 // Speicher für Struktur sockaddr_in setzen
 errno = 0;
 bzero(&serverSocketSettings, sizeof(serverSocketSettings));
 if( errno != 0 ) { handle_error("bzero()"); };
 
 // Speicher für Struktur in_addr setzen
 errno = 0;
 struct in_addr serverSocketIP;
 bzero(&serverSocketIP, sizeof(serverSocketIP));
 if( errno != 0 ) { handle_error("bzero()"); };

 // Zuweisen der Protokollfamilie
 serverSocketSettings.sin_family = AF_INET;
 
 // Zuweisen der IP-Adresse zur in_addr Struktur serverSocketIP
 errno = 0;
 inet_aton(SERVER_IP, &serverSocketIP);
 if( errno != 0 ) { handle_error("inet_aton()"); };
 serverSocketSettings.sin_addr = serverSocketIP;

 // TCP-Port definieren
 errno = 0;
 serverSocketSettings.sin_port = htons(SERVER_PORT);
 if( errno != 0 ) { handle_error("htons()"); };

 // Socket binden
 errno = 0;
 clientSocketFD = connect(serverSocketFD, (struct sockaddr*)&serverSocketSettings, sizeof(serverSocketSettings));
 if( errno != 0 ) { handle_error("connect()"); };
 
 errno = 0;
 send(serverSocketFD, &mqttControlPacketConnectBuffer, sizeof(mqttControlPacketConnectBuffer), 0);
 if( errno != 0 ) { handle_error("send() mqttControlPacketConnectBuffer"); };

 errno = 0;
 recv(serverSocketFD, &mqttControlPacketConnectackBuffer, sizeof(mqttControlPacketConnectackBuffer), 0); 
 if( errno != 0 ) { handle_error("recv() mqttControlPacketConnectackBuffer"); };
 if(mqttControlPacketConnectackBuffer[3] != 0) {
  errno=1;
  if( errno != 0 ) { handle_error("recv() mqttControlPacketConnectackBuffer Returncode != 0"); };   
 }
 if(mqttControlPacketConnectackBuffer[2] != 0) {
  errno=1;
  if( errno != 0 ) { handle_error("recv() mqttControlPacketConnectackBuffer != 0, Session Present is not supported"); };   
 }

 errno = 0;
 send(serverSocketFD, &mqttControlPacketSubscribeBuffer, sizeof(mqttControlPacketSubscribeBuffer), 0);
 if( errno != 0 ) { handle_error("send() mqttControlPacketSubscribe"); };

 errno = 0;
 recv(serverSocketFD, &mqttControlPacketSubackBuffer, sizeof(mqttControlPacketSubackBuffer), 0);
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
 
 // Tag: C001
 while(keep_running) {
  errno = 0;
  recv(serverSocketFD, &mqttControlPacketPublishBuffer, sizeof(mqttControlPacketPublishBuffer), 0);
  if( errno != 0 ) { handle_error("send() mqttControlPacketPublishBuffer"); }; 
  printf("%c%c%c\n", mqttControlPacketPublishBuffer[7], mqttControlPacketPublishBuffer[8], mqttControlPacketPublishBuffer[9]);
 }

 errno = 0;
 recv(serverSocketFD, &mqttControlPacketUnsubackBuffer, sizeof(mqttControlPacketUnsubackBuffer), 0);
 if( errno != 0 ) { handle_error("send() mqttControlPacketUnsubackBuffer"); }; 

 errno = 0;
 send(serverSocketFD, &mqttControlPacketDisconnectBuffer, sizeof(mqttControlPacketDisconnectBuffer), 0);
 if( errno != 0 ) { handle_error("send() mqttControlPacketDisconnectBuffer"); };

 errno = 0; 
 close(clientSocketFD);
 if( errno != 0 ) { handle_error("close() clientSocketFD"); };
 
 return EXIT_SUCCESS;
}