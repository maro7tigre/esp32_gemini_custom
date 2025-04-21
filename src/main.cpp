#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "credentials.h" // Contains WIFI_SSID, WIFI_PASSWORD and GEMINI_API_KEY
#include "custom_cam.h"

// Pin definitions
#define TRIGGER_PIN  12   // Input pin to trigger image capture
#define OUTPUT_PIN   13   // Output pin for signaling results

// Waste type definitions
#define TYPE_PLASTIC    1
#define TYPE_CARDBOARD  2
#define TYPE_PAPER      3
#define TYPE_OTHER      4
#define TYPE_NONE       5
#define TYPE_ERROR      6

WebServer server(80);
bool processingImage = false;
bool wifiTrigger = false;
char* lastJsonPayload = NULL;

// Default prompt for trash classification
const char* DEFAULT_PROMPT = "I want a short answer for which trash type do you see in the image [plastic, cardboard, paper or other], don't write anything else other than one of this list, if you can't see any trash just say None";

// Function prototypes
void setupServer();
int parseGeminiResponse(const char* response);
void signalResult(int wasteType);

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println("ESP32-CAM Trash Classifier");
  
  // Setup pins
  pinMode(TRIGGER_PIN, INPUT_PULLDOWN);
  pinMode(OUTPUT_PIN, OUTPUT);
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // Add connection timeout/retry logic here if needed
  }
  
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize camera
  if (!initCamera()) {
    Serial.println("Camera init failed! Restarting...");
    delay(1000);
    ESP.restart();
  }
  Serial.println("Camera initialized");
  
  // Setup and start server
  setupServer();
  server.begin();
  
  Serial.println("Waiting for trigger...");
}

void loop() {
  // Handle web requests
  server.handleClient();
  
  // Check if trigger pin is HIGH or WiFi trigger is set, and not already processing
  if ((digitalRead(TRIGGER_PIN) == HIGH || wifiTrigger) && !processingImage) {
    processingImage = true;
    wifiTrigger = false; // Reset WiFi trigger flag
    Serial.println("Taking image...");
    
    // Capture image as JSON for Gemini
    size_t encodedSize = 0;
    char* jsonPayload = captureImageAsGeminiJson(DEFAULT_PROMPT, &encodedSize, GEMINI_API_KEY);
    
    if (!jsonPayload) {
      Serial.println("Capture failed");
      processingImage = false;
      return;
    }
    
    // Send to Gemini API
    char* geminiResponse = sendToGeminiAPI(jsonPayload, GEMINI_API_KEY);
    
    if (!geminiResponse) {
      Serial.println("API request failed");
      // Save JSON for web viewing even if Gemini fails
      if (lastJsonPayload) free(lastJsonPayload);
      lastJsonPayload = jsonPayload;
      processingImage = false;
      return;
    }
    
    // Parse response and signal result
    int wasteType = parseGeminiResponse(geminiResponse);
    Serial.print("Result: ");
    
    switch(wasteType) {
      case TYPE_PLASTIC: Serial.println("Plastic"); break;
      case TYPE_CARDBOARD: Serial.println("Cardboard"); break;
      case TYPE_PAPER: Serial.println("Paper"); break;
      case TYPE_OTHER: Serial.println("Other"); break;
      case TYPE_NONE: Serial.println("None"); break;
      default: Serial.println("Error"); break;
    }
    
    signalResult(wasteType);
    
    // Cleanup
    free(geminiResponse);
    if (lastJsonPayload) free(lastJsonPayload);
    lastJsonPayload = jsonPayload;
    
    // Wait for trigger to go LOW again
    while (digitalRead(TRIGGER_PIN) == HIGH) {
      server.handleClient();
      delay(10);
    }
    
    Serial.println("Waiting for trigger...");
    processingImage = false;
  }
  
  delay(10); // Small delay to prevent watchdog issues
}

void setupServer() {
  // Home page
  server.on("/", HTTP_GET, []() {
    String html = "<html><body>";
    html += "<h1>ESP32-CAM Trash Classifier</h1>";
    html += "<p><a href='/photo'>View Latest Capture</a></p>";
    html += "<p><a href='/trigger'>Trigger New Capture</a></p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });
  
  // Latest image JSON
  server.on("/photo", HTTP_GET, []() {
    if (!lastJsonPayload) {
      server.send(404, "text/plain", "No image captured yet");
      return;
    }
    server.send(200, "application/json", lastJsonPayload);
  });
  
  // WiFi trigger endpoint
  server.on("/trigger", HTTP_GET, []() {
    if (!processingImage) {
      wifiTrigger = true;
      server.send(200, "text/plain", "Triggered successfully");
    } else {
      server.send(409, "text/plain", "Already processing an image");
    }
  });
}

int parseGeminiResponse(const char* response) {
  String resp = String(response);
  resp.toLowerCase();
  
  if (resp.indexOf("plastic") >= 0) return TYPE_PLASTIC;
  if (resp.indexOf("cardboard") >= 0) return TYPE_CARDBOARD;
  if (resp.indexOf("paper") >= 0) return TYPE_PAPER;
  if (resp.indexOf("other") >= 0) return TYPE_OTHER;
  if (resp.indexOf("none") >= 0) return TYPE_NONE;
  
  return TYPE_ERROR;
}

void signalResult(int wasteType) {
  digitalWrite(OUTPUT_PIN, HIGH);
  delay(50 * wasteType);  // Length corresponds to waste type
  digitalWrite(OUTPUT_PIN, LOW);
}