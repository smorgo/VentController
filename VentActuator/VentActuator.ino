/*
 * My keys.h file is loaded from a library, so that these sensitive values
 * are kept outside of the publicly-available code. It should look something
 * like this:
 * 
 * #ifndef __KEYS_H__
 * #define __KEYS_H__
 * #define WIFI_SSID "<Your WiFi SSID>"
 * #define WIFI_PWD "<Your WiFi Passkey>"
 * #define SENSORS_TOPIC "/sensors"
 * #define SENSORS_CMD_TOPIC "/senscmd"
 * #define SERVER "<Your MQTT broker URL or IP address>"
 * #define SENSORS_CLIENT_NAME "SENSORS1"
 * // This secret API_KEY will be shared with other devices that will
 * // publish messages to this device or subscribe to messages from it.
 * // It's used to validate the integrity of the message.
 * #define API_KEY "<32-digit hexadecimal secret key>"
 * #define API_KEY_LENGTH (32)
 * #endif
 */

// Messages received will be processed, even if the hash 
// is incorrect when IS_TEST_MODE is true.
#define IS_TEST_MODE true

#include <keys.h>
#include <MD5.h>
#include <MQTT.h>
#include <PubSubClient.h>

#include <ESP8266WiFi.h>
#include <Wire.h>

//////////////////////
// WiFi Definitions //
//////////////////////
const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PWD;
const char topic[] = ACTUATOR_STATUS_TOPIC;
const char cmd_topic[] = ACTUATOR_CMD_TOPIC;
const char server[] = SERVER;
const char clientName[] = ACTUATOR_CLIENT_NAME;
const char apiKey[] = API_KEY;
const int apiKeyLength = API_KEY_LENGTH;

#pragma message("ssid: " WIFI_SSID)
#pragma message("server: " SERVER)
#pragma message("clientName: " ACTUATOR_CLIENT_NAME)
#pragma message("apiKey: " API_KEY)

WiFiClient wifiClient;
PubSubClient client(wifiClient, server, 1883);
bool seeded = false;

/////////////////////
// Pin Definitions //
/////////////////////
const int LED_PIN = 5; // Thing's onboard, green LED
const int BUTTON_PIN = A0; 

/////////////////
// Post Timing //
/////////////////
const unsigned long postRate = 15000;
unsigned long lastPost = 0;
unsigned long counter = 0;

/////////////////
// State       //
/////////////////
int currentPosition = -1;
int targetPosition = 0;
int buttonState = -1;
int rainState = -1;
 
String getHash(String payload)
{
  int payloadLength = payload.length();
  int bufferLength = apiKeyLength + payloadLength + 1;
  char *buffer = (char *)malloc(bufferLength);
  memcpy(buffer, apiKey, apiKeyLength);
  payload.toCharArray(buffer + apiKeyLength, payloadLength + 1);
  
  unsigned char* hash=MD5::make_hash(buffer);
  char *md5str = MD5::make_digest(hash, 16);
  String calcHash = String(md5str);
  free(md5str);
  free(hash);
  free(buffer);

  return calcHash;
}

void processMessage(String recv_topic, String msg)
{
  Serial.print(recv_topic);
  Serial.print(" => ");
  Serial.println(msg);

  if(msg == "RESTART")
  {
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
}

String getValidMessage(String msg)
{
 int delimIx = msg.indexOf(':');

  if(delimIx < 0 || delimIx == (msg.length() - 1))
  {
    // No hash
    return IS_TEST_MODE ? msg : String();
  }

  String payload = msg.substring(0,delimIx);
  String payloadHash = msg.substring(delimIx + 1);

  String calcHash = getHash(payload);
    
  if(calcHash.equalsIgnoreCase(payloadHash))
  {
    Serial.println("HASH MATCH");
    return payload;    
  }

  Serial.println("**HASH DOESN'T MATCH**");
  if(IS_TEST_MODE)
  {
    Serial.print("Received ");
    Serial.println(payloadHash);
    Serial.print("Calculated ");
    Serial.println(calcHash);
    return payload;
  }

  return String();
}


bool connectBroker()
{
  Serial.println("Connecting to MQTT broker");
  if (client.connect(clientName)) 
  {
    Serial.println("Connected to MQTT broker");
    Serial.print("Topic is: ");
    Serial.println(topic);
    Serial.print("Subscribing to ");
    Serial.println(cmd_topic);
    client.subscribe(cmd_topic);
    client.publish(ANNOUNCE_TOPIC,clientName);
    return true;
  }    
  else 
  {
    Serial.println("MQTT connect failed");
    Serial.println("Will reset and try again...");
    return false;
  }
}

void connectWiFi()
{
  byte ledStatus = LOW;

  Serial.println("Connecting to WiFi");
  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);

  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  // to the stated [ssid], using the [passkey] as a WPA, WPA2,
  // or WEP passphrase.
  WiFi.begin(ssid, password);

  // Use the WiFi.status() function to check if the ESP8266
  // is connected to a WiFi network.
  while (WiFi.status() != WL_CONNECTED)
  {
    // Blink the LED
    digitalWrite(LED_PIN, ledStatus); // Write LED high/low
    ledStatus = (ledStatus == HIGH) ? LOW : HIGH;

    // Delays allow the ESP8266 to perform critical tasks
    // defined outside of the sketch. These tasks include
    // setting up, and maintaining, a WiFi connection.
    delay(100);
    // Potentially infinite loops are generally dangerous.
    // Add delays -- allowing the processor to perform other
    // tasks -- wherever possible.
  }
  Serial.println("Connected to WiFi");

  connectBroker();
}

void initHardware()
{
  Serial.begin(9600);
  Serial.println("");
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.println("");
}

int postStatus()
{
  // LED turns on when we enter, it'll go off when we 
  // successfully post.
  digitalWrite(LED_PIN, HIGH);

  if(IS_TEST_MODE) Serial.println("Publishing status");
   
  if(WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();
  }

  // TODO: Get some actual data to pass to the broker!
  
  // Add a random value to our message, so that the MD5 hash will be different,
  // even when all of the sensor readings are the same.
  if(!seeded)
  {
    randomSeed(millis());
    seeded = true;
  }
  
  long rand = random(2147483647);
  
  String msg = "R(" + String(rand) + "),POS(" + String(currentPosition) + "),TPOS(" + String(targetPosition) + "),BN(" + String(buttonState) + "),RA(" + String(rainState) + ")";

  String calcHash = getHash(msg);
  
  msg += ":";
  msg += calcHash;

  Serial.print(++counter);
  Serial.print(" = ");
  Serial.println(msg);

  bool connected = client.connected();

  if(!connected) 
  {
    connected = connectBroker();
  }
  
  if(connected)
  {
    if(!client.publish(topic, msg))
    {
      Serial.println("Publish FAILED");
    } 
  }

  // Before we exit, turn the LED off.
  digitalWrite(LED_PIN, LOW);

  if(IS_TEST_MODE) Serial.println("Done publishing update");
  return 1; // Return success
}

void callback(const MQTT::Publish& pub) 
{
  String msg = getValidMessage(pub.payload_string());

  if(msg.length() > 0)
  {
    processMessage(pub.topic(), msg);
  }
}

void setup() 
{
  initHardware();

  client.set_callback(callback);
  
  connectWiFi();
  digitalWrite(LED_PIN, HIGH);
}

void loop() 
{
  client.loop();
  if (lastPost + postRate <= millis())
  {
    if (postStatus())
    {
      lastPost = millis();
    }
    else
    {
      delay(100);    
    }
  }
}



