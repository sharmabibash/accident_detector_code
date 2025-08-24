#include <ESP8266WiFi.h>
#include <ESP_Mail_Client.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// WiFi Credentials
const char* ssid = "aaaaa";
const char* password = "11111111";

// SMTP Configuration
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465  

// Email sender credentials
const char* emailSender = "kumarbibash769@gmail.com";
const char* emailPassword = "egjy kefd hmvl stbk";  // Use App Password for Gmail

// Email recipient
const char* emailRecipient = "sharmabibash77@gmail.com";

// SMTP session object
SMTPSession smtp;

// GPS & Accelerometer Setup
TinyGPSPlus gps;
SoftwareSerial gpsSerial(D7, D8); // GPS Module TX → D7 (GPIO13), RX → D8 (GPIO15)
const int xPin = A0; // ADXL335 X-axis
const int yPin = A0; // ADXL335 Y-axis
const int zPin = A0; // ADXL335 Z-axis
float prevX, prevY, prevZ;

// Function to Send Email with GPS Location
void sendAccidentAlert(float latitude, float longitude) {
    Serial.println("📨 Sending Email...");

    ESP_Mail_Session session;
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = emailSender;
    session.login.password = emailPassword;
    session.login.user_domain = "smtp.gmail.com";

    SMTP_Message message;
    message.sender.name = "ESP8266 Accident Alert";
    message.sender.email = emailSender;
    message.subject = "🚨 Accident Detected! Location Details";
    message.addRecipient("Receiver", emailRecipient);

    String body = "🚗 Accident Detected!\n\n";
    body += "Live Location:\n";
    body += "Latitude: " + String(latitude, 6) + "\n";
    body += "Longitude: " + String(longitude, 6) + "\n";
    body += "Google Maps Link:\n";
    body += "https://www.google.com/maps?q=" + String(latitude, 6) + "," + String(longitude, 6);
    
    message.text.content = body.c_str();
    message.text.charSet = "utf-8";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    if (!smtp.connect(&session)) {
        Serial.println("❌ SMTP Connection Failed!");
        return;
    }

    if (smtp.isAuthenticated()) {
        if (MailClient.sendMail(&smtp, &message)) {
            Serial.println("✅ Email Sent Successfully!");
        } else {
            Serial.println("❌ Email Sending Failed!");
        }
    } else {
        Serial.println("❌ SMTP Authentication Failed!");
    }
}

void setup() {
    Serial.begin(115200);
    gpsSerial.begin(9600); // Start GPS Module

    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ Connected to WiFi!");

    // Initialize Accelerometer
    prevX = analogRead(xPin);
    prevY = analogRead(yPin);
    prevZ = analogRead(zPin);
}

void loop() {
    // Read Accelerometer Data
    float xValue = analogRead(xPin);
    float yValue = analogRead(yPin);
    float zValue = analogRead(zPin);

    // Detect Sudden Change (Accident)
    if (abs(xValue - prevX) > 50 || abs(yValue - prevY) > 50 || abs(zValue - prevZ) > 50) {
        Serial.println("🚨 Accident Detected! Fetching GPS Location...");

        // Fetch GPS Coordinates
        float latitude = 0.0, longitude = 0.0;
        unsigned long startTime = millis();
        while (millis() - startTime < 5000) { // Try for 5 seconds
            while (gpsSerial.available()) {
                gps.encode(gpsSerial.read());
                if (gps.location.isUpdated()) {
                    latitude = gps.location.lat();
                    longitude = gps.location.lng();
                    break;
                }
            }
        }

        if (latitude != 0.0 && longitude != 0.0) {
            Serial.print("🌍 Location: ");
            Serial.print("Lat: "); Serial.print(latitude, 6);
            Serial.print(", Lng: "); Serial.println(longitude, 6);

            sendAccidentAlert(latitude, longitude); // Send Email
        } else {
            Serial.println("⚠️ GPS Location Not Found!");
        }
    }

    // Update Previous Values
    prevX = xValue;
    prevY = yValue;
    prevZ = zValue;

    delay(1000); // Delay to avoid false detections
}
