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


// MQTT Topics
const char TOPIC_CMD[] ="/cmd";
const char TOPIC_STATUS[] = "/status";
const char TOPIC_EVENT[] = "/event";

// Message Types
#define MSG_TYPE_SURVEY (1)
#define MSG_TYPE_HELLO (2)
#define MSG_TYPE_SENSOR (3)
#define MSG_TYPE_ACTUATOR (4)
#define MSG_TYPE_ACTSTAT (5)
#define MSG_TYPE_RESET (6)
#define MSG_TYPE_OVERRIDE (7)
#define MSG_TYPE_BUTTON (8)

//////////////////////
// WiFi Definitions //
//////////////////////
const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PWD;
const char server[] = SERVER;
const char clientName[] = ACTUATOR_CLIENT_NAME;
const char apiKey[] = API_KEY;
const int apiKeyLength = API_KEY_LENGTH;
String privateCmdTopic;

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
const int RAIN_PIN = 4;
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
const int WET = 0;
const int DRY = 1;
const int BTN_NONE = 0;
const int BTN_1 = 1;
const int BTN_2 = 2;
const int BTN_BOTH = 3;

// Button thresholds
const int BUTTON_HIGH_THRESHOLD = 840;
const int BUTTON1_HIGH_THRESHOLD = 520;
const int BUTTON2_HIGH_THRESHOLD = 150;

int currentPosition = VENT_OPEN; // On boot, we'll assume it's open and close it.
int targetPosition = NONE;
int newPosition = NONE; // If it's raining, this is zero, otherwise, it's targetPosition
int buttonState = NONE;
int rainState = NONE;
int rainStateCounter = 0;
int previousRainState = NONE;
int previousButtonState = NONE;
int buttonStateCounter = 0;

unsigned long startMillis = 0; // When the current movement started
unsigned long endMillis = 0; // When the current movement should end

bool isMoving = false;
bool directionOpen = false;
bool currentOpenRelayState = HIGH;
bool currentCloseRelayState = HIGH;
bool ignoreRainSensor = false;

String getParam(String msg, String name)
{
  String tmpMsg = "," + msg;
  String lookFor = "," + name + "(";

  int startIx = tmpMsg.indexOf(lookFor);
  if(startIx < 0)
  {
    return "";
  }
  int endIx = tmpMsg.indexOf(")", startIx);
  if(endIx < startIx)
  {
    return "";
  }
  String val =  msg.substring(startIx + lookFor.length()-1, endIx-1);
  Serial.print("Looked for ");
  Serial.print(lookFor);
  Serial.print(" found ");
  Serial.println(val);

  return val;
}

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
  int pos = getParam(msg,"TPOS").toInt();

  Serial.print("Position requested: ");
  Serial.println(pos);
  
  setPosition(pos);
}

bool isMyTopic(String topic)
{
  return (topic == TOPIC_CMD) ||
         (topic == privateCmdTopic);
}

void processMessage(String recv_topic, String msg)
{
  Serial.print(recv_topic);
  Serial.print(" => ");
  Serial.println(msg);

  if(!isMyTopic(recv_topic))
  {
    return;
  }
  
  int msgType = getParam(msg,"M").toInt();

  switch(msgType)
  {
    case MSG_TYPE_SURVEY:
      publish(TOPIC_STATUS,getHelloMessage());
      break;
    case MSG_TYPE_RESET:
      Serial.println("Restarting...");
      delay(1000);
      ESP.restart();
      break;
    case MSG_TYPE_ACTUATOR:
      processPositionMessage(msg);
      break;
    default:
      Serial.println("Unrecognised message");
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

void subscribe(String topic)
{
  Serial.print("Subscribing to MQTT topic: ");
  Serial.println(topic);
  client.subscribe(topic);
}

void publishToConnection(String topic, String msg)
{
  String calcHash = getHash(msg);
  
  msg += ":";
  msg += calcHash;

  Serial.print("[");
  Serial.print(++counter);
  Serial.print("] ");
  Serial.print("Publish to ");
  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(msg);
  
  if(!client.publish(topic, msg))
  {
    Serial.println("Publish FAILED");
  } 
}

void publish(String topic, String msg)
{
  bool connected = client.connected();

  if(!connected) 
  {
    connected = connectBroker();
  }
  
  if(connected)
  {
    publishToConnection(topic, msg);
  }
}

String getBaseMessage(int messageType)
{
  // Add a random value to our message, so that the MD5 hash will be different,
  // even when all of the sensor readings are the same.
  if(!seeded)
  {
    randomSeed(millis());
    seeded = true;
  }
  
  long rand = random(2147483647);
  
  String msg = "R(" + String(rand) + "),M(" + String(messageType) + "),DEV(" + clientName + ")";

  return msg;
}

String getHelloMessage()
{
  String msg = getBaseMessage(MSG_TYPE_HELLO);

  // Add Topics
  msg += ",PUB(" + String(TOPIC_STATUS) + ")" +
         ",SUB(" + String(TOPIC_CMD) + ")" +
         ",SUB(" + privateCmdTopic + ")";

  return msg;
}

String getActStatMessage()
{
  String msg = getBaseMessage(MSG_TYPE_ACTSTAT);

  String msg2 = ",POS(" + String(currentPosition) + "),TPOS(" + String(targetPosition) + "),BN(" + String(buttonState) + "),RA(" + String(rainState) + ")";

  return msg + msg2;
}

String getButtonMessage()
{
  String msg = getBaseMessage(MSG_TYPE_BUTTON);

  String msg2 = ",BN(" + String(buttonState) + ")";

  return msg + msg2;
}

bool connectBroker()
{
  Serial.println("Connecting to MQTT broker");
  if (client.connect(clientName)) 
  {
    Serial.println("Connected to MQTT broker");
    subscribe(TOPIC_CMD);
    subscribe(privateCmdTopic);
    publishToConnection(TOPIC_STATUS,getHelloMessage());
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
  pinMode(RAIN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT);
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

  publish(TOPIC_STATUS,getActStatMessage());

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
    currentPosition = newPosition;
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
  
  targetPosition = position;
  
  if(rainState == WET && !ignoreRainSensor)
  {
    newPosition = 0;
  }
  else
  {
    newPosition = targetPosition;
  }

  Serial.print("Selecting position ");
  Serial.println(String(newPosition));
  
  int steps = abs(newPosition - currentPosition);

  if(steps == 0)
  {
    // Nowhere to go
    return;
  }

  startMillis = millis();
  endMillis = startMillis + steps * stepTransitTime;
  directionOpen = (newPosition > currentPosition);
  isMoving = true;

  Serial.print("Timers: startMillis = ");
  Serial.print(String(startMillis));
  Serial.print(" endMillis = ");
  Serial.println(String(endMillis));
  
  setRelays();
}

void setup() 
{
  privateCmdTopic = String(TOPIC_CMD) + "/" + String(ACTUATOR_CLIENT_NAME);
  
  initHardware();

  client.set_callback(callback);
  
  connectWiFi();

  setPosition(VENT_CLOSED);
  digitalWrite(LED_PIN, HIGH);
}

void checkRain()
{
  int rain = digitalRead(RAIN_PIN) == LOW ? WET : DRY;

  // Allow the pin to settle
  if(rain != previousRainState)
  {
    previousRainState = rain;
    rainStateCounter = 0;
    return;
  }

  if(++rainStateCounter > 50)
  {
    if(rain != rainState)
    {
      Serial.println(rain == WET ? "It's RAINING" : "It's DRY");
      rainState = rain;
      setPosition(targetPosition); // This will either cause the vent to close if it's raining, or move to its target position if it's not
    }
    rainStateCounter = 0;
  }
}

void checkButtons()
{
  int btn = analogRead(BUTTON_PIN);

//  Serial.print("Button pin: ");
//  Serial.println(btn);

  int btn_state = (btn > BUTTON_HIGH_THRESHOLD) ? BTN_NONE :
                  (btn > BUTTON1_HIGH_THRESHOLD) ? BTN_1 :
                  (btn > BUTTON2_HIGH_THRESHOLD) ? BTN_2 :
                  BTN_BOTH;   

  if(btn_state != previousButtonState)
  {
      previousButtonState = btn_state;
      buttonStateCounter = 0;
      return;
  }

  if(++buttonStateCounter > 10)
  {
    if(btn_state != buttonState)
    {
      Serial.print("Button state: ");
      Serial.println(btn_state);
      buttonState = btn_state;
      publish(TOPIC_EVENT,getButtonMessage());
    }
    buttonStateCounter = 0;
  }
}

void loop() 
{
  checkRain();
  checkButtons();
  
  setRelays();
  client.loop();
  if (lastPost + postRate <= millis())
  {
    if (postStatus())
    {
      lastPost = millis();
    }
  }

  delay(100);    
}



