#include <WiFiNINA.h>
#include <PubSubClient.h>

const char* ssid = "NAMASKAR";
const char* password = "12345678";

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic_sensors = "home/smoke/sensors";
const char* mqtt_topic_control = "home/smoke/control";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

// Smoke sensors
const int smokeSensor1 = A0;
const int smokeSensor2 = A1;
const int smokeThreshold = 700;

// Actuators
const int buzzerPin1 = 8;
const int ledPin1 = 6;
const int buzzerPin2 = 9;
const int ledPin2 = 7;

// Switch
const int switchPin = 2;

bool systemOn = true;
bool blinkState = false;
int blinkInterval = 500;

unsigned long lastBlinkTime = 0;
unsigned long lastSensorPublish = 0;
const unsigned long sensorInterval = 1000;

void setup() {
  Serial.begin(9600);

  pinMode(buzzerPin1, OUTPUT);
  pinMode(ledPin1, OUTPUT);
  pinMode(buzzerPin2, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(switchPin, INPUT_PULLUP); // Enable internal pull-up

  digitalWrite(buzzerPin1, LOW);
  digitalWrite(ledPin1, LOW);
  digitalWrite(buzzerPin2, LOW);
  digitalWrite(ledPin2, LOW);

  connectToWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  unsigned long currentTime = millis();

  // Switch check (LOW means ON because of pull-up logic)
  systemOn = digitalRead(switchPin) == LOW;

  // Sensor reading and MQTT publish
  if (currentTime - lastSensorPublish >= sensorInterval) {
    int smoke1 = analogRead(smokeSensor1);
    int smoke2 = analogRead(smokeSensor2);

    String payload = "SYSTEM:" + String(systemOn ? "ON" : "OFF") +
    ",S1:" + smoke1 +
    ",S2:" + smoke2;
    client.publish(mqtt_topic_sensors, payload.c_str());
    lastSensorPublish = currentTime;

    if (systemOn) {
      blinkInterval = (smoke1 > smokeThreshold || smoke2 > smokeThreshold) ? 100 : 300;
    } else {
      blinkInterval = 0;
      turnOffActuators();
    }
  }

  // Blinking logic
  if (systemOn && blinkInterval > 0) {
    if (currentTime - lastBlinkTime >= blinkInterval) {
      blinkState = !blinkState;
      digitalWrite(ledPin1, blinkState);
      digitalWrite(buzzerPin1, blinkState);
      digitalWrite(ledPin2, blinkState);
      digitalWrite(buzzerPin2, blinkState);
      lastBlinkTime = currentTime;
    }
  }
}

void turnOffActuators() {
  digitalWrite(ledPin1, LOW);
  digitalWrite(buzzerPin1, LOW);
  digitalWrite(ledPin2, LOW);
  digitalWrite(buzzerPin2, LOW);
}

void connectToWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    delay(3000);
  }
  Serial.println("Connected to WiFi!");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("SmokeDetectorArduino")) {
      Serial.println("connected!");
      client.subscribe(mqtt_topic_control);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      delay(3000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)message[i];
  }

  msg.trim();
  Serial.print("Message received: ");
  Serial.println(msg);

  if (msg == "CLEAR" || msg == "ALERT" || msg == "HIGHALERT") {
    // systemOn is now controlled only by switch
    Serial.println("MQTT command received, but systemOn is now based on switch state only.");
  }
}
