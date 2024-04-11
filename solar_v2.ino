// Solar Sound Installation by Anvay K and Jo S
// Resources:
// https://usankycheng.notion.site/Sending-data-from-a-MCU-to-a-MQTT-broker-c91cc63b8cb84f839694b3c10c4cc235
// https://www.keyoftech.com/mastering-the-arduino-watchdog-timer-a-comprehensive-tutorial/

#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include <wdt_samd21.h>
#include <ArduinoLowPower.h>

const int pinVolume = A0;
const int pinMotor = 10;
const int pinVoltage = 9;

const int volThreshold = 375;

#define DC_OFFSET 0  // DC offset in mic signal - if unusure, leave 0
#define NOISE 100    // Noise/hum/interference in mic signal

const char broker[] = "9.tcp.ngrok.io";  //IP address of the EMQX broker.
int port = 24004;

// The SSID and password we are going to use in NAVY YARD, if you want to test, use your own wifi ssid and password
// char ssid[] = "NewLabMember 2.4GHz Only";  // your network SSID (name)
// char pass[] = "!Welcome2NewLab!";          // your network password

char ssid[] = "sandbox370";         // your network SSID (name)
char pass[] = "+s0a+s03!2gether?";  // your network password

const char publish_topic[] = "/energy_solar_project/solar_sound";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

char mqtt_user[] = "energy";
char mqtt_pass[] = "password";

void setup() {
  pinMode(pinVolume, INPUT);
  pinMode(pinMotor, OUTPUT);
  pinMode(pinVoltage, INPUT);

  Serial.begin(9600);
  while (!Serial) {
    ;
  }
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  delay(1000);

  WiFi.begin(ssid, pass);
  delay(2000);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("You're connected to the network");
  Serial.println();

  // You can provide a username and password for authentication
  mqttClient.setUsernamePassword(mqtt_user, mqtt_pass);

  Serial.print("Attempting to connect to the MQTT broker.");
  Serial.println();

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1)
      ;
  }

  Serial.println("You're connected to the MQTT broker!");

  Serial.print("Subscribing to topic: ");
  Serial.println(publish_topic);

  // initiate watchdog timer with a 8sec count down
  // you can look up differnt duration here: https://github.com/gpb01/wdt_samd21
  wdt_init(WDT_CONFIG_PER_8K);
}

void loop() {
  int volume = analogRead(pinVolume);                 // raw reading
  volume = abs(volume - 512 - DC_OFFSET);             // center on zero
  volume = (volume <= NOISE) ? 0 : (volume - NOISE);  // remove noise/hum
  // int voltage = analogRead(pinVoltage);
  float voltage = (analogRead(pinVoltage) * 3.3) / 4095.0;

  checkMqttConnection();

  Serial.print("Voltage: ");
  Serial.println(voltage);
  Serial.print("Volume: ");
  Serial.println(volume);

  if (volume) {
    mqttClient.beginMessage(publish_topic);
    mqttClient.print("{\"volume\":");
    mqttClient.print(volume);
    mqttClient.print("}, {\"voltage\":");
    mqttClient.print(voltage);
    mqttClient.print("}");
    bool success = mqttClient.endMessage();

    if (!success) {
      // Log or handle the failure to send the message
      Serial.println("Failed to send MQTT message.");
      // Implement additional error handling or retry logic here if necessary
    }
  }
  if (volume < volThreshold) {
    digitalWrite(pinMotor, HIGH);

  } else {
    digitalWrite(pinMotor, LOW);
    Serial.println("volume too high");
  }

  delay(3000);

  // reset the watchdog timer, or the watch dog will trigger the MCU to restart
  // system will be reset if the software gets stuck or takes too long to complete an operation
  wdt_reset();
}

void checkMqttConnection() {
  if (!mqttClient.connected()) {
    Serial.print("MQTT not connected. Attempting to reconnect... error code:");
    Serial.println(mqttClient.connectError());

    while (!mqttClient.connect(broker, port)) { // attempt to reconnect
      Serial.print(".");
      delay(5000);  // Wait 5 seconds before retrying
      wdt_reset();
    }

    Serial.println("\nReconnected to MQTT broker.");
    Serial.print("Resubscribing to topic: ");
    Serial.println(publish_topic);
  }
  
}
