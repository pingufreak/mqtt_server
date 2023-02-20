#include <stdint.h>

enum mqttControlPacketTypes {
 CONNECT = 1,
 CONNACK = 2,
 PUBLISH = 3,
 SUBSCRIBE = 8,
 SUBACK = 9,
 UNSUBSCRIBE = 10,
 UNSUBACK = 11,
 PINGREQ = 13,
 PINGRESP = 13,
 DISCONNECT = 14
};

typedef struct {
 union {
  uint8_t mqttFixedHeaderByte1;
  struct {
   unsigned mqttControlPacketFlags : 4;
   unsigned mqttControlPacketType : 4;
  } mqttFixedHeaderByte1Bits;
 };
} mqttFixedHeaderTpl;

typedef struct {
 union {
  uint8_t mqttFixedHeaderByte1;
  struct {
   unsigned mqttControlPacketFlags : 4;
   unsigned mqttControlPacketType : 4;
  } mqttFixedHeaderByte1Bits;
 };
 uint8_t mqttFixedHeaderRemainingLength; 
 uint8_t mqttVariableHeaderProtocolNameMSB;           // byte 1
 uint8_t mqttVariableHeaderProtocolNameLSB;           // byte 2
 uint8_t mqttVariableHeaderProtocolNameChar0;         // byte 3
 uint8_t mqttVariableHeaderProtocolNameChar1;         // byte 4
 uint8_t mqttVariableHeaderProtocolNameChar2;         // byte 5
 uint8_t mqttVariableHeaderProtocolNameChar3;         // byte 6
 uint8_t mqttVariableHeaderProtocolLevel;             // byte 7
 
 union {
  uint8_t mqttVariableHeaderConnectFlags;             // byte 8
  struct {
   unsigned mqttVariableHeaderConnectFlagsReserved     : 1;  
   unsigned mqttVariableHeaderConnectFlagsCleanSession : 1;
   unsigned mqttVariableHeaderConnectFlagsWillFlag     : 1;
   unsigned mqttVariableHeaderConnectFlagsWillQoS      : 2;
   unsigned mqttVariableHeaderConnectFlagsWillRetain   : 1;
   unsigned mqttVariableHeaderConnectFlagsPassword     : 1;
   unsigned mqttVariableHeaderConnectFlagsUsername     : 1;
  } mqttVariableHeaderConnectFlagsBits;
 };
 uint8_t mqttVariableHeaderKeepAliveMSB;              // byte 9
 uint8_t mqttVariableHeaderKeepAliveLSB;              // byte 10
 uint8_t clientIdMSB;            
 uint8_t clientIdLSB;
 uint8_t clientIdChar0;
 uint8_t clientIdChar1;
 uint8_t clientIdChar2;
} mqttControlPacketConnectTpl;

typedef struct {
 union {
  uint8_t mqttFixedHeaderByte1;
  struct {
   unsigned mqttControlPacketFlags : 4;
   unsigned mqttControlPacketType : 4;
  } mqttFixedHeaderByte1Bits;
 };
 uint8_t mqttFixedHeaderRemainingLength; 
 uint8_t mqttVariableHeaderConnectackFlags;        // byte 1
 uint8_t mqttVariableHeaderConnectackReturncode;   // byte 2
} mqttControlPacketConnectackTpl; 

typedef struct {
 union {
  uint8_t mqttFixedHeaderByte1;
  struct {
   unsigned mqttControlPacketFlags : 4;
   unsigned mqttControlPacketType : 4;
  } mqttFixedHeaderByte1Bits;
 };
 uint8_t mqttFixedHeaderRemainingLength; 
 uint8_t mqttVariableHeaderTopicNameMSB;           // byte 1
 uint8_t mqttVariableHeaderTopicNameLSB;           // byte 2
 uint8_t mqttVariableHeaderTopicNameChar0;         // byte 3
 uint8_t mqttVariableHeaderTopicNameChar1;         // byte 4
 uint8_t mqttVariableHeaderTopicNameChar2;         // byte 5
 uint8_t mqttVariableHeaderPayloadChar0;           // byte 6
 uint8_t mqttVariableHeaderPayloadChar1;           // byte 7
 uint8_t mqttVariableHeaderPayloadChar2;           // byte 8
 uint8_t mqttVariableHeaderPayloadChar3;           // byte 9
 uint8_t mqttVariableHeaderPayloadChar4;           // byte 10
 uint8_t mqttVariableHeaderPayloadChar5;           // byte 11
 uint8_t mqttVariableHeaderPayloadChar6;           // byte 12
} mqttControlPacketPublishTpl;

typedef struct {
 union {
  uint8_t mqttFixedHeaderByte1;
  struct {
   unsigned mqttControlPacketFlags : 4;
   unsigned mqttControlPacketType : 4;
  } mqttFixedHeaderByte1Bits;
 };
 uint8_t mqttFixedHeaderRemainingLength; 
 uint8_t mqttVariableHeaderPacketIdentifierMSB;    // byte 1
 uint8_t mqttVariableHeaderPacketIdentifierLSB;    // byte 2
 uint8_t mqttVariableHeaderTopicFilterMSB;         // byte 3
 uint8_t mqttVariableHeaderTopicFilterLSB;         // byte 4
 uint8_t mqttVariableHeaderTopicFilterChar0;       // byte 5
 uint8_t mqttVariableHeaderTopicFilterChar1;       // byte 6
 uint8_t mqttVariableHeaderTopicFilterChar2;       // byte 7
 uint8_t mqttVariableHeaderTopicFilterQos;         // byte 8
} mqttControlPacketSubscribeTpl;

typedef struct {
 union {
  uint8_t mqttFixedHeaderByte1;
  struct {
   unsigned mqttControlPacketFlags : 4;
   unsigned mqttControlPacketType : 4;
  } mqttFixedHeaderByte1Bits;
 };
 uint8_t mqttFixedHeaderRemainingLength; 
 uint8_t mqttVariableHeaderPacketIdentifierMSB;    // byte 1
 uint8_t mqttVariableHeaderPacketIdentifierLSB;    // byte 2
 union {
  uint8_t mqttVariableHeaderSubackReturncode;      // byte 3
  struct {
   unsigned success : 2;
   unsigned reserved : 5;
   unsigned failed : 1;
  } mqttVariableHeaderSubackReturncodeBits;
 };
} mqttControlPacketSubackTpl;

typedef struct {
 union {
  uint8_t mqttFixedHeaderByte1;
  struct {
   unsigned mqttControlPacketFlags : 4;
   unsigned mqttControlPacketType : 4;
  } mqttFixedHeaderByte1Bits;
 };
 uint8_t mqttFixedHeaderRemainingLength; 
 uint8_t mqttVariableHeaderPacketIdentifierMSB;    // byte 1
 uint8_t mqttVariableHeaderPacketIdentifierLSB;    // byte 2
 uint8_t mqttVariableHeaderTopicFilterMSB;         // byte 3
 uint8_t mqttVariableHeaderTopicFilterLSB;         // byte 4
 uint8_t mqttVariableHeaderTopicFilterChar0;       // byte 5
 uint8_t mqttVariableHeaderTopicFilterChar1;       // byte 6
 uint8_t mqttVariableHeaderTopicFilterChar2;       // byte 7
} mqttControlPacketUnsubscribeTpl;

typedef struct {
 union {
  uint8_t mqttFixedHeaderByte1;
  struct {
   unsigned mqttControlPacketFlags : 4;
   unsigned mqttControlPacketType : 4;
  } mqttFixedHeaderByte1Bits;
 };
 uint8_t mqttFixedHeaderRemainingLength; 
 uint8_t mqttVariableHeaderPacketIdentifierMSB;    // byte 1
 uint8_t mqttVariableHeaderPacketIdentifierLSB;    // byte 2
} mqttControlPacketUnsubackTpl;

typedef struct {
 union {
  uint8_t mqttFixedHeaderByte1;
  struct {
   unsigned mqttControlPacketFlags : 4;
   unsigned mqttControlPacketType : 4;
  } mqttFixedHeaderByte1Bits;
 };
 uint8_t mqttFixedHeaderRemainingLength; 
} mqttControlPacketDisconnectTpl;

typedef struct {
 union {
  uint8_t mqttFixedHeaderByte1;
  struct {
   unsigned mqttControlPacketFlags : 4;
   unsigned mqttControlPacketType : 4;
  } mqttFixedHeaderByte1Bits;
 };
 uint8_t mqttFixedHeaderRemainingLength; 
} mqttControlPacketPingrespTpl; 
