// === Smoke Detector Arduino Code ===

// Analog smoke sensor pins
const int smokeSensor1 = A0;
const int smokeSensor2 = A1;

// LED and Buzzer pins
const int led1 = 7;
const int led2 = 6;
const int buzzer1 = 8;
const int buzzer2 = 8;

// Sliding switch pin
const int switchPin = 2;

// Threshold value
const int threshold = 1000;

// Blinking timing
unsigned long lastBlinkTime = 0;
unsigned long blinkInterval = 500;

bool ledState1 = false;
bool buzzerState1 = false;
bool ledState2 = false;
bool buzzerState2 = false;

void setup() {
  Serial.begin(9600);

  pinMode(smokeSensor1, INPUT);
  pinMode(smokeSensor2, INPUT);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(buzzer1, OUTPUT);
  pinMode(buzzer2, OUTPUT);

  pinMode(switchPin, INPUT_PULLUP);
}

void loop() {
  bool systemOn = (digitalRead(switchPin) == LOW);

  if (systemOn) {
    unsigned long currentMillis = millis();
    int value1 = analogRead(smokeSensor1);
    int value2 = analogRead(smokeSensor2);

    // Send serial output
    Serial.print("SYSTEM:");
    Serial.print(systemOn ? "ON" : "OFF");
    Serial.print(",S1:");
    Serial.print(value1);
    Serial.print(",S2:");
    Serial.println(value2);

    // Blinking logic
    if (currentMillis - lastBlinkTime >= blinkInterval) {
      lastBlinkTime = currentMillis;
      ledState1 = !ledState1;
      buzzerState1 = !buzzerState1;
      ledState2 = !ledState2;
      buzzerState2 = !buzzerState2;
    }

    // Smoke detection
    bool smoke1 = value1 > threshold;
    bool smoke2 = value2 > threshold;

    // Control LED and buzzer
    digitalWrite(led1, smoke1 ? HIGH : ledState1);
    digitalWrite(buzzer1, smoke1 ? HIGH : buzzerState1);
    digitalWrite(led2, smoke2 ? HIGH : ledState2);
    digitalWrite(buzzer2, smoke2 ? HIGH : buzzerState2);

    // Listen for Pi command
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();
      if (cmd == "ALERT") {
        blinkInterval = 100;
      } else if (cmd == "CLEAR") {
        blinkInterval = 500;
      }
    }
  } else {
    // System OFF — everything OFF
    digitalWrite(led1, LOW);
    digitalWrite(buzzer1, LOW);
    digitalWrite(led2, LOW);
    digitalWrite(buzzer2, LOW);
  }

  delay(10); // Smooth operation
}

