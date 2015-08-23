/*
 * My keys.h file is loaded from a library, so that these sensitive values
 * are kept outside of the publicly-available code. It should look something
 * like this:
 * 
 * #ifndef __KEYS_H__
 * #define __KEYS_H__
 * #define WIFI_SSID "<Your WiFi SSID>"
 * #define WIFI_PWD "<Your WiFi Passkey>"
 * #define ANNOUNCE_TOPIC "/announce"
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
#define IS_TEST_MODE false

#include <keys.h>
#include <MD5.h>
#include <MQTT.h>
#include <PubSubClient.h>

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

// Define supported sensors
#define INCLUDE_TSL2561
#define INCLUDE_HTU21DF
#define INCLUDE_T5403

#ifdef INCLUDE_TSL2561
#pragma message("Including support for TSL2561 Luminosity sensor")
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
#endif

#ifdef INCLUDE_HTU21DF
#pragma message("Including support for HTU21DF Humidity sensor")
#include <Adafruit_HTU21DF.h>
#endif

#ifdef INCLUDE_T5403
#pragma message("Including support for T5403 Barometric Pressure sensor")
#include <SparkFunT5403.h>
#endif

#include <ESP8266WiFi.h>
#include <Wire.h>

#ifdef INCLUDE_HTU21DF
Adafruit_HTU21DF htu21df = Adafruit_HTU21DF();
#endif

#ifdef INCLUDE_T5403
SparkFun_T5403 t5403 = SparkFun_T5403();
#endif

float temperatureA;
float temperatureB;
float humidity;
float luminosity;
float pressure;

//////////////////////
// WiFi Definitions //
//////////////////////
const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PWD;
const char server[] = SERVER;
const char clientName[] = SENSORS_CLIENT_NAME;
const char apiKey[] = API_KEY;
const int apiKeyLength = API_KEY_LENGTH;
String privateCmdTopic;

#pragma message("ssid: " WIFI_SSID)
#pragma message("server: " SERVER)
#pragma message("clientName: " SENSORS_CLIENT_NAME)
#pragma message("apiKey: " API_KEY)

WiFiClient wifiClient;
PubSubClient client(wifiClient, server, 1883);
bool seeded = false;

/////////////////////
// Pin Definitions //
/////////////////////
const int LED_PIN = 5; // Thing's onboard, green LED
const int ANALOG_PIN = A0; // The only analog pin on the Thing
const int I2C_SDA = 2;
const int I2C_SCL = 14;

/////////////////
// Post Timing //
/////////////////
const unsigned long postRate = 15000;
unsigned long lastPost = 0;
unsigned long counter = 0;

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

void callback(const MQTT::Publish& pub) 
{
  String msg = getValidMessage(pub.payload_string());

  if(msg.length() > 0)
  {
    processMessage(pub.topic(), msg);
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
    default:
      Serial.println("Unrecognised message");
  }
}

void setup() 
{
  privateCmdTopic = String(TOPIC_CMD) + "/" + String(SENSORS_CLIENT_NAME);
  
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
    if (postSensorReadings())
    {
      lastPost = millis();
    }
    else
    {
      delay(100);    
    }
  }
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

String getSensorMessage()
{
  String msg = getBaseMessage(MSG_TYPE_SENSOR);

  String msg2 = ",TA(" + String(temperatureA,2) + "),TB(" + String(temperatureB,2) + "),HU(" + String(humidity,2) + "),LX(" + String(luminosity,2) + "),PA(" + String(pressure,0) + ")";

  return msg + msg2;
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

#ifdef INCLUDE_TSL2561
  tsl.begin();
  configureTSL2561();
#endif

#ifdef INCLUDE_HTU21DF
  htu21df.begin();
#endif

#ifdef INCLUDE_T5403
  t5403.begin();
#endif

  Serial.println("");
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

int postSensorReadings()
{
  // LED turns on when we enter, it'll go off when we 
  // successfully post.
  digitalWrite(LED_PIN, HIGH);

  if(IS_TEST_MODE) Serial.println("Reading sensors");
   
  if(WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();
  }

#ifdef INCLUDE_HTU21DF
  htu21df.reset();
  temperatureA = htu21df.readTemperature();
  humidity = htu21df.readHumidity();
#endif

#ifdef INCLUDE_TSL2561
  sensors_event_t event;
  tsl.getEvent(&event);
  luminosity = event.light;
#endif

#ifdef INCLUDE_T5403
  temperatureB = float(t5403.getTemperature(temperature_units::CELSIUS))/100.0;
  pressure = t5403.getPressure(MODE_ULTRA);
#endif

  publish(TOPIC_STATUS,getSensorMessage());

  // Before we exit, turn the LED off.
  digitalWrite(LED_PIN, LOW);

  if(IS_TEST_MODE) Serial.println("Done reading sensors");
  return 1; // Return success
}

#ifdef INCLUDE_TSL2561
void configureTSL2561(void)
{
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */  
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         "); Serial.println("Auto");
  Serial.print  ("Timing:       "); Serial.println("402 ms");
  Serial.println("------------------------------------");
}
#endif

