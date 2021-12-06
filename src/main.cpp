/*
 * Humidity and Temperature Monitor
 * for Adafruit Feather Huzzah, Adafruit Si7021 sensor, and Adafruit FeatherWind 128x64 OLED.
 * 
 * Copyright (c) 2021 Chuck Hunley
 */
#include <config.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_Si7021.h>
#include <Fonts/FreeSans9pt7b.h>

/*
 * MQTT client
 */
#include <PubSubClient.h>

/*
 * Setup Si7021 Sensor
 */
Adafruit_Si7021 sensor = Adafruit_Si7021();

/*
 * Setup OLED display
 */
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);

/*
 * Delay between sensor samplings.
 */
#define DELAY (1*60*1000)     // 1 minute

// set up the 'temperature' and 'humidity' feeds
AdafruitIO_Feed *temperature_feed = io.feed("crawl-space.temperature");
AdafruitIO_Feed *humidity_feed    = io.feed("crawl-space.humidity");

/*
 * MQTT ID and topics
 */
const char *ID                = "Crawl Space";                  // Unique name for device.
const char *HUMIDITY_TOPIC    = "crawlspace/dht22/humidity";    // Topic to subscribe to for humidity
const char *TEMERATURE_TOPIC  = "crawlspace/dht22/temperature"; // Topic to subscribe to for temperature

#define MQTT_RECONNECT_DELAY    (5000)  // Delay in milliseconds to wait before attempting reconnect
#define MQTT_KEEPALIVE_INTERVAL (120)   // Interval to keep connection with broker alive

IPAddress broker(10,228,253,134);                // IP address of your MQTT broker eg. 192.168.1.50
WiFiClient wclient;

PubSubClient client(wclient);                    // Setup MQTT client

// OLED FeatherWing buttons map to different pins depending on board:  Not current used by this project.
#if defined(ESP8266)
  #define BUTTON_A  0
  #define BUTTON_B 16
  #define BUTTON_C  2
#elif defined(ESP32) && !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
  #define BUTTON_A 15
  #define BUTTON_B 32
  #define BUTTON_C 14
#elif defined(ARDUINO_STM32_FEATHER)
  #define BUTTON_A PA15
  #define BUTTON_B PC7
  #define BUTTON_C PC5
#elif defined(TEENSYDUINO)
  #define BUTTON_A  4
  #define BUTTON_B  3
  #define BUTTON_C  8
#elif defined(ARDUINO_NRF52832_FEATHER)
  #define BUTTON_A 31
  #define BUTTON_B 30
  #define BUTTON_C 27
#else // 32u4, M0, M4, nrf52840, esp32-s2 and 328p
  #define BUTTON_A  9
  #define BUTTON_B  6
  #define BUTTON_C  5
#endif

/*
 * Reconnect to MQTT broker
 */
void reconnect() {
  /*
   * Loop until we're reconnected
   */
  while (!client.connected()) {
    Serial.print("Establishing MQTT connection...");

    /*
     * Attempt to connect
     */
    if(client.connect(ID)) {
      Serial.println("connected");
      Serial.println('\n');
    } else {
      Serial.print("Error connecting to MQTT broker. error=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(MQTT_RECONNECT_DELAY);  // Wait 5 seconds before retrying
    }
  }
}


void setup() {
  Serial.begin(115200);

  Serial.println("128x64 OLED FeatherWing enable");
  display.begin(0x3C, true); // Address 0x3C default

  Serial.println("OLED begun");

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(1000);

   // Clear the buffer.
  display.clearDisplay();
  display.display();

  display.setRotation(1);
  
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  // text display tests
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0);
  display.print("Initializing Si7021\n");
  display.display(); // actually display all of the above

  if (!sensor.begin()) {
    Serial.println("Did not find Si7021 sensor.");
    display.print("No Si7021 sensor");
  } else {
    Serial.println("Found Si7021 sensor");
    display.print("Found Si7021 sensor\n");
    sensor.readSerialNumber();
    display.printf("Serial #A: %d\n", sensor.sernum_a);
    display.printf("Serial #B: %d\n", sensor.sernum_b);
    display.display();
    delay(5*1000);

    display.clearDisplay();
    display.display();

    // Connect to io.adafruit.com
    Serial.print("Connecting to Adafruit IO");
    display.setCursor(0,0);
    display.print("Connecting to\nAdafruit IO");
    display.display();
    io.connect();

    // wait for a connection
    while(io.status() < AIO_CONNECTED) {
      Serial.print(".");
      display.print(".");
      display.display();
      delay(500);
    }

    // we are connected
    Serial.println();
    Serial.println(io.statusText());
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(io.statusText());
    display.display();
    delay(5*1000);

    // Connect to MQTT broker
    Serial.println("Setup MQTT broker.");

    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Setup MQTT broker.");
    display.display();
    client.setKeepAlive(MQTT_KEEPALIVE_INTERVAL);
    client.setServer(broker, 1883);
    display.print("MQTT broker enabled.");
    display.display();

    delay(5*1000);

    display.setFont(&FreeSans9pt7b);
  }

}

void loop() {

  /*
   * io.run(); is required for all sketches.
   * it should always be present at the top of your loop
   * function. it keeps the client connected to
   * io.adafruit.com, and processes any incoming data.
   */
  io.run();

  float temperature = sensor.readTemperature() * 1.8 + 32;
  float humidity    = sensor.readHumidity();

  Serial.printf("Temperature: %4.1fF\n", temperature);
  Serial.printf("Humidity: %4.1f\n", humidity);

  display.clearDisplay();

  display.setCursor(0,20);
  display.print("Temp: ");
  display.printf("%4.1fF", temperature);

  display.setCursor(0,60);
  display.print("Hum: ");
  display.printf("%4.1f%%", humidity);
  display.display();

  // Send fahrenheit (or celsius) to Adafruit IO
  temperature_feed->save(temperature);

  // Send humidity to Adafruit IO
  humidity_feed->save(humidity);  

   // Publish to MQTT broker for homekit
  if (!client.connected()) {  // Reconnect if connection is lost
    reconnect();
  }
  
  if (client.connected()) {
    char pubStrBuffer[5];
    int  pubStrBufferL;

    pubStrBufferL = snprintf(pubStrBuffer, sizeof(pubStrBuffer), "%.1f", (temperature - 32) * 5/9);
    pubStrBuffer[pubStrBufferL] = '\0';
    if (client.publish(TEMERATURE_TOPIC, pubStrBuffer)) {
      Serial.print("Published temperature: ");
      Serial.println(pubStrBuffer);
    } else {
      Serial.printf("Publish temperature failed.");
    }

    pubStrBufferL = snprintf(pubStrBuffer, sizeof(pubStrBuffer), "%.1f", humidity);
    pubStrBuffer[pubStrBufferL] = '\0';
    if (client.publish(HUMIDITY_TOPIC, pubStrBuffer)) {
      Serial.print("Published humidity: ");
      Serial.println(pubStrBuffer);
    } else {
      Serial.printf("Publish humidity failed.");
    }
  } else {
    Serial.println("Not connected to MQTT broker.");
  }

  display.display();

  delay(DELAY);
  yield();  // Yield for WiFi
  display.display();
}

