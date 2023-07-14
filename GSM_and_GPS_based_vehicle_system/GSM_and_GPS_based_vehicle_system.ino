#include <SoftwareSerial.h> // Library for software serial communication
#include <TinyGPS++.h>     // Library for parsing GPS data
#include <FirebaseESP8266.h> // Library for Firebase integration

SoftwareSerial gsmSerial(7, 8);  // RX, TX for SIM900A GSM module
SoftwareSerial gpsSerial(9, 10);  // RX, TX for GY-GPS6MV2 GPS module

TinyGPSPlus gps;
FirebaseData firebaseData;
FirebaseJson json;

const int vibrationPin = A0;  // Vibration sensor analog pin
const int abortButtonPin = 4;  // Abort button pin
const int ledPin = 13;  // LED pin

bool accidentDetected = false;
bool abortButtonPressed = false;
unsigned long previousMillis = 0;
unsigned long alertInterval = 120000;  // 2 minutes
unsigned long ledDuration = 3000;  // 3 seconds

void setup() {
  Serial.begin(9600);
  gsmSerial.begin(9600);
  gpsSerial.begin(9600);

  pinMode(vibrationPin, INPUT);
  pinMode(abortButtonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  
 Firebase.begin("https://gsm-and-gps-vehicle--system-default-rtdb.firebaseio.com/", "ROTiyMsWXyxsQeZ0kkGLTwVe5233he5E97XMu3yZ");
}

void loop() {
  // Check if the abort button is pressed
  if (digitalRead(abortButtonPin) == LOW) {
    abortButtonPressed = true;
  }
  
  // Check for vibrations
  if (analogRead(vibrationPin) > 200) {  // Adjust the threshold value according to your sensor
    accidentDetected = true;
    previousMillis = millis();
    digitalWrite(ledPin, HIGH);  // Turn on the LED
  }
  
  // Check if enough time has passed since the accident and the abort button is not pressed
  if (accidentDetected && !abortButtonPressed && (millis() - previousMillis >= alertInterval)) {
    // Get GPS coordinates
    float latitude = gps.location.lat();
    float longitude = gps.location.lng();
    
    // Prepare data to send to Firebase
    json.clear();
    json.add("latitude", latitude);
    json.add("longitude", longitude);
    
    // Send data to Firebase
    if (Firebase.pushJSON(firebaseData, "/accidents", json)) {
      Serial.println("Accident data sent to Firebase.");
    } else {
      Serial.println("Error sending accident data to Firebase.");
    }
    
    // Send SMS alert
    sendSMSAlert(latitude, longitude);
    
    // Reset variables
    accidentDetected = false;
    abortButtonPressed = false;
    digitalWrite(ledPin, LOW);  // Turn off the LED
  }
  
  // Turn off the LED after the specified duration
  if (accidentDetected && millis() - previousMillis >= ledDuration) {
    digitalWrite(ledPin, LOW);  // Turn off the LED
  }
  
  // Process GPS data
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      // Uncomment the following lines to print GPS data
      // Serial.print("Latitude: ");
      // Serial.println(gps.location.lat(), 6);
      // Serial.print("Longitude: ");
      // Serial.println(gps.location.lng(), 6);
    }
  }
}

void sendSMSAlert(float latitude, float longitude) {
  // GSM module configuration commands
  gsmSerial.println("AT");  // Check if the module is ready
  delay(1000);
  gsmSerial.println("AT+CMGF=1");  // Set SMS text mode
  delay(1000);
  
  // Compose the SMS message
  String message = "Accident detected!\nLatitude: " + String(latitude, 6) +
                   "\nLongitude: " + String(longitude, 6);
  
  // Send the SMS message
  gsmSerial.println("AT+CMGS=\"" + recipientNumber + "\"");  // Replace recipientNumber with the actual phone number
  delay(1000);
  gsmSerial.println(message);
  delay(1000);
  gsmSerial.println((char)26);  // Ctrl+Z to send the message
  delay(1000);
  
  Serial.println("SMS alert sent.");
}
