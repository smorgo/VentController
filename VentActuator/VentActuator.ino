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
const int MOTOR_OPEN_PIN = 12;
const int MOTOR_CLOSE_PIN = 13;

/////////////////
// Post Timing //
/////////////////
const unsigned long postRate = 15000;
unsigned long lastPost = 0;
unsigned long counter = 0;

/////////////////
// State       //
/////////////////
const int VENT_CLOSED = 0;
const int VENT_OPEN = 4;
const int NONE = -1;
const int fullTransitTime = 8000; // 8 seconds
const int stepTransitTime = fullTransitTime / VENT_OPEN;

int currentPosition = VENT_OPEN; // On boot, we'll assume it's open and close it.
int targetPosition = NONE;
int buttonState = NONE;
int rainState = NONE;

unsigned long startMillis = 0; // When the current movement started
unsigned long endMillis = 0; // When the current movement should end

bool isMoving = false;
bool directionOpen = false;
bool currentOpenRelayState = HIGH;
bool currentCloseRelayState = HIGH;

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

void processPositionMessage(String msg)
{
  if(msg.startsWith("POS("))
  {
    int ix = msg.indexOf(")");
    if(ix < 5)
    {
      return;
    }
    String data = msg.substring(4,ix);
    Serial.print("Position requested: ");
    Serial.println(data);

    int pos = data.toInt();
    setPosition(pos);
  }
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
  else
  {
    processPositionMessage(msg);
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
  pinMode(MOTOR_OPEN_PIN, OUTPUT);
  pinMode(MOTOR_CLOSE_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(MOTOR_OPEN_PIN, HIGH);
  digitalWrite(MOTOR_CLOSE_PIN, HIGH);
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

void setOpenRelay(bool state)
{
  if(state != currentOpenRelayState)
  {
    Serial.print("Open relay = ");
    Serial.println(state ? "OFF" : "ON");
    digitalWrite(MOTOR_OPEN_PIN, state);
    currentOpenRelayState = state;  
  }
}

void setCloseRelay(bool state)
{
  if(state != currentCloseRelayState)
  {
    Serial.print("Close relay = ");
    Serial.println(state ? "OFF" : "ON");
    digitalWrite(MOTOR_CLOSE_PIN, state);
    currentCloseRelayState = state;  
  }
}

void setRelays()
{
  if(!isMoving)
  {
    // Nothing to do
    return;
  }
  
  if(millis() > endMillis)
  {
    Serial.println("Stopping");
    // Time to stop
    setOpenRelay(HIGH);
    setCloseRelay(HIGH);
    isMoving = false;
    directionOpen = false;
    startMillis = 0;
    endMillis = 0;
    currentPosition = targetPosition;
  }
  else
  {
    setOpenRelay(directionOpen ? LOW : HIGH);
    setCloseRelay(directionOpen ? HIGH : LOW);
  }
}

void setPosition(int position)
{
  if(isMoving)
  {
    return;  
  }
  
  Serial.print("Selecting position ");
  Serial.println(String(position));
  
  targetPosition = position;

  int steps = abs(targetPosition - currentPosition);

  if(steps == 0)
  {
    // Nowhere to go
    return;
  }

  startMillis = millis();
  endMillis = startMillis + steps * stepTransitTime;
  directionOpen = (targetPosition > currentPosition);
  isMoving = true;

  Serial.print("Timers: startMillis = ");
  Serial.print(String(startMillis));
  Serial.print(" endMillis = ");
  Serial.println(String(endMillis));
  
  setRelays();
}

void setup() 
{
  initHardware();

  client.set_callback(callback);
  
  connectWiFi();

  setPosition(VENT_CLOSED);
  digitalWrite(LED_PIN, HIGH);
}

void loop() 
{
  setRelays();
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



