/*
 PubSubClient.h - A simple client for MQTT.
  Nick O'Leary
  http://knolleary.net
*/

#ifndef PubSubClient_h
#define PubSubClient_h

#include "Client.h"
#include "IPAddress.h"
#include "Stream.h"
#include <Arduino.h>

#define MQTT_VERSION_3_1 3
#define MQTT_VERSION_3_1_1 4

// MQTT_VERSION : Pick the version
// #define MQTT_VERSION MQTT_VERSION_3_1
#ifndef MQTT_VERSION
#define MQTT_VERSION MQTT_VERSION_3_1_1
#endif

// MQTT_MAX_PACKET_SIZE : Maximum packet size. Override with setBufferSize().
#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 256
#endif

// MQTT_KEEPALIVE : keepAlive interval in Seconds. Override with setKeepAlive()
#ifndef MQTT_KEEPALIVE
#define MQTT_KEEPALIVE 15
#endif

// MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds. Override with setSocketTimeout()
#ifndef MQTT_SOCKET_TIMEOUT
#define MQTT_SOCKET_TIMEOUT 15
#endif

// MQTT_MAX_TRANSFER_SIZE : limit how much data is passed to the network client
//  in each write call. Needed for the Arduino Wifi Shield. Leave undefined to
//  pass the entire MQTT packet in each write call.
// #define MQTT_MAX_TRANSFER_SIZE 80

// Possible values for client.state()
#define MQTT_CONNECTION_TIMEOUT -4
#define MQTT_CONNECTION_LOST -3
#define MQTT_CONNECT_FAILED -2
#define MQTT_DISCONNECTED -1
#define MQTT_CONNECTED 0
#define MQTT_CONNECT_BAD_PROTOCOL 1
#define MQTT_CONNECT_BAD_CLIENT_ID 2
#define MQTT_CONNECT_UNAVAILABLE 3
#define MQTT_CONNECT_BAD_CREDENTIALS 4
#define MQTT_CONNECT_UNAUTHORIZED 5
#define MQTT_READ_TIMEOUT 6

#define MQTTCONNECT 1 << 4      // Client request to connect to Server        C->S
#define MQTTCONNACK 2 << 4      // Connect Acknowledgment                     S->C
#define MQTTPUBLISH 3 << 4      // Publish message                            C<>S
#define MQTTPUBACK 4 << 4       // Publish Acknowledgment                     C<>S
#define MQTTPUBREC 5 << 4       // Publish Received (assured delivery part 1) C<>S
#define MQTTPUBREL 6 << 4       // Publish Release (assured delivery part 2)  C<>S
#define MQTTPUBCOMP 7 << 4      // Publish Complete (assured delivery part 3) C<>S
#define MQTTSUBSCRIBE 8 << 4    // Client Subscribe request                   C->S
#define MQTTSUBACK 9 << 4       // Subscribe Acknowledgment                   S->C
#define MQTTUNSUBSCRIBE 10 << 4 // Client Unsubscribe request                 C->S
#define MQTTUNSUBACK 11 << 4    // Unsubscribe Acknowledgment                 S->C
#define MQTTPINGREQ 12 << 4     // PING Request                               C->S
#define MQTTPINGRESP 13 << 4    // PING Response                              S->C
#define MQTTDISCONNECT 14 << 4  // Client is Disconnecting                    C->S
#define MQTTReserved 15 << 4    // Reserved

#define MQTTTYPE(t) ((t) & 0xF0)
#define MQTTQOS(t) ((t) & 0x0E)

#define MQTTQOS0 (0 << 1)
#define MQTTQOS1 (1 << 1)
#define MQTTQOS2 (2 << 1)

// Maximum size of fixed header and variable length size header
#define MQTT_MAX_HEADER_SIZE 5

#if defined(ESP8266) || defined(ESP32)
#include <functional>
#define MQTT_CALLBACK_SIGNATURE std::function<void(char*, uint8_t*, unsigned int)> callback
#else
#define MQTT_CALLBACK_SIGNATURE void (*callback)(char*, uint8_t*, unsigned int)
#endif

#define CHECK_STRING_LENGTH(l, s)                                \
  if (l + 2 + strnlen(s, this->bufferSize) > this->bufferSize) { \
    _client->stop();                                             \
    return false;                                                \
  }

class PubSubWriter {
public:
  // Interface for receiving payload for subscribed messages. If the message payload
  // fits into the buffer there is a single call to the packetReceived function containing
  // the payload. If the payload is larger than the buffer size streaming is used. Then at
  // first startStream is called indicating the expected length. Then multiple call to
  // write are performed to flush the received buffer. At last endStream will be called
  // to indicate that the transfer is complete. If the length passed in endStream is
  // smaller than the length in startStream an error occured and transfer of the payload
  // was aborted.
  virtual size_t write(const uint8_t* buffer, size_t size) = 0;
  virtual void startStream(const char* topic, uint16_t msgId, uint32_t length) = 0;
  virtual void endStream(const char* topic, uint16_t msgId, uint32_t length) = 0;
  virtual void packetReceived(const char* topic, uint16_t msgId, const uint8_t* payload, uint32_t length) = 0;
};

class PubSubClient : public Print {
private:
  Client* _client;
  uint8_t* buffer;
  uint16_t bufferSize;
  uint16_t keepAlive;
  uint16_t socketTimeout;
  uint16_t nextMsgId;
  unsigned long lastOutActivity;
  unsigned long lastInActivity;
  bool pingOutstanding;
  MQTT_CALLBACK_SIGNATURE; // kept for compatibility

  boolean readPacketHeader(uint8_t* type, uint32_t* length);
  boolean handlePublishPacket(uint8_t type, uint32_t remaining);
  boolean sendPubAck(uint16_t msgId);
  boolean skipData(uint32_t remaining);
  boolean readByte(uint8_t* result);
  boolean readByte(uint8_t* result, uint16_t* index);
  size_t readBytes(size_t length);
  boolean write(uint8_t header, uint8_t* buf, uint16_t length);
  uint16_t writeString(const char* string, uint8_t* buf, uint16_t pos);
  // Build up the header ready to send
  // Returns the size of the header
  // Note: the header is built at the end of the first MQTT_MAX_HEADER_SIZE bytes, so will start
  //       (MQTT_MAX_HEADER_SIZE - <returned size>) bytes into the buffer
  size_t buildHeader(uint8_t header, uint8_t* buf, uint16_t length);
  IPAddress ip;
  const char* domain;
  uint16_t port;
  PubSubWriter* writer;
  int _state;
  boolean useStreamingOnlyForLargePackets;

public:
  PubSubClient();
  PubSubClient(Client& client);
  PubSubClient(IPAddress, uint16_t, Client& client);
  PubSubClient(IPAddress, uint16_t, Client& client, PubSubWriter&);
  PubSubClient(IPAddress, uint16_t, MQTT_CALLBACK_SIGNATURE, Client& client);
  PubSubClient(IPAddress, uint16_t, MQTT_CALLBACK_SIGNATURE, Client& client, PubSubWriter&);
  PubSubClient(uint8_t*, uint16_t, Client& client);
  PubSubClient(uint8_t*, uint16_t, Client& client, PubSubWriter&);
  PubSubClient(uint8_t*, uint16_t, MQTT_CALLBACK_SIGNATURE, Client& client);
  PubSubClient(uint8_t*, uint16_t, MQTT_CALLBACK_SIGNATURE, Client& client, PubSubWriter&);
  PubSubClient(const char*, uint16_t, Client& client);
  PubSubClient(const char*, uint16_t, Client& client, PubSubWriter&);
  PubSubClient(const char*, uint16_t, MQTT_CALLBACK_SIGNATURE, Client& client);
  PubSubClient(const char*, uint16_t, MQTT_CALLBACK_SIGNATURE, Client& client, PubSubWriter&);

  ~PubSubClient();

  PubSubClient& setServer(IPAddress ip, uint16_t port);
  PubSubClient& setServer(uint8_t* ip, uint16_t port);
  PubSubClient& setServer(const char* domain, uint16_t port);
  PubSubClient& setCallback(MQTT_CALLBACK_SIGNATURE);

  PubSubClient& setClient(Client& client);
  PubSubClient& setWriter(PubSubWriter& writer);
  PubSubClient& setKeepAlive(uint16_t keepAlive);
  PubSubClient& setSocketTimeout(uint16_t timeout);

  boolean setBufferSize(uint16_t size);
  uint16_t getBufferSize();

  boolean connect(const char* id);
  boolean connect(const char* id, const char* user, const char* pass);
  boolean connect(const char* id, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
  boolean connect(const char* id, const char* user, const char* pass, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
  boolean connect(const char* id, const char* user, const char* pass, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage, boolean cleanSession);
  void disconnect();
  boolean publish(const char* topic, const char* payload);
  boolean publish(const char* topic, const char* payload, boolean retained);
  boolean publish(const char* topic, const uint8_t* payload, unsigned int plength);
  boolean publish(const char* topic, const uint8_t* payload, unsigned int plength, boolean retained);
  boolean publish_P(const char* topic, const char* payload, boolean retained);
  boolean publish_P(const char* topic, const uint8_t* payload, unsigned int plength, boolean retained);
  // Start to publish a message.
  // This API:
  //   beginPublish(...)
  //   one or more calls to write(...)
  //   endPublish()
  // Allows for arbitrarily large payloads to be sent without them having to be copied into
  // a new buffer and held in memory at one time
  // Returns 1 if the message was started successfully, 0 if there was an error
  boolean beginPublish(const char* topic, unsigned int plength, boolean retained);
  // Finish off this publish message (started with beginPublish)
  // Returns 1 if the packet was sent successfully, 0 if there was an error
  int endPublish();
  // Write a single byte of payload (only to be used with beginPublish/endPublish)
  virtual size_t write(uint8_t);
  // Write size bytes from buffer into the payload (only to be used with beginPublish/endPublish)
  // Returns the number of bytes written
  virtual size_t write(const uint8_t* buffer, size_t size);
  boolean subscribe(const char* topic);
  boolean subscribe(const char* topic, uint8_t qos);
  boolean unsubscribe(const char* topic);
  boolean loop();
  boolean connected();
  int state();
};

#endif
