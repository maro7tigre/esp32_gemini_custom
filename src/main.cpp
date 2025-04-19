#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "credentials.h" // Keep your existing credentials.h with WIFI_SSID and WIFI_PASSWORD
#include "custom_cam.h"

// Add this to your credentials.h or define it here
// #define GEMINI_API_KEY "your-api-key-here"

// Pin definitions
#define TRIGGER_PIN  12   // Input pin to trigger image capture
#define OUTPUT_PIN   13   // Output pin for signaling results

// Waste type definitions
#define TYPE_PLASTIC    1
#define TYPE_CARDBOARD  2
#define TYPE_PAPER      3
#define TYPE_OTHER      4
#define TYPE_NONE       5
#define TYPE_404        6

WebServer server(80);
bool cameraInitialized = false;
bool processingImage = false;
char* lastJsonPayload = NULL;
size_t lastJsonSize = 0;

// Default prompt for trash classification
const char* DEFAULT_PROMPT = "I want a short answer for which trash type do you see in the image [plastic, cardboard, paper or other], don't write anything else other than one of this list, if you can't see any trash just say None";

// Function prototypes
void setupServer();
int parseGeminiResponse(const char* response);
void signalResult(int wasteType);
char* captureImage();
char* processWithGemini(char* jsonPayload);

// MARK: Setup
void setup() {
  // Start serial communication
  Serial.begin(9600);
  delay(1000); // Allow serial to initialize
  
  Serial.println("\n\nESP32-CAM Trash Classification System");
  Serial.println("---------------------");
  
  // Setup trigger pin as input with pull-down
  pinMode(TRIGGER_PIN, INPUT_PULLDOWN);
  
  // Setup output pin
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);
  
  // Initialize WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  
  // Wait for connection with timeout
  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20) {
    delay(500);
    Serial.print(".");
    wifiTimeout++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed! Restarting...");
    delay(3000);
    ESP.restart();
  }
  
  Serial.println();
  Serial.print("Connected to: ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize camera
  cameraInitialized = initCamera();
  if (!cameraInitialized) {
    Serial.println("Camera initialization failed! Restarting...");
    delay(3000);
    ESP.restart();
  }
  
  Serial.println("Camera initialized successfully");
  
  // Setup server
  setupServer();
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("System ready - waiting for trigger signal on pin 12");
}

// MARK: Loop
void loop() {
  // Handle web requests
  server.handleClient();
  
  // Check if trigger pin is HIGH and not already processing
  if (digitalRead(TRIGGER_PIN) == HIGH && !processingImage && cameraInitialized) {
    processingImage = true;
    
    Serial.println("Trigger detected! Starting image capture process");
    
    // Step 1: Capture image
    Serial.println("Step 1: Capturing image...");
    char* jsonPayload = captureImage();
    
    if (!jsonPayload) {
      Serial.println("Image capture failed");
      processingImage = false;
      return;
    }
    Serial.println("Image captured successfully");
    
    // Step 2: Process with Gemini API
    Serial.println("Step 2: Sending to Gemini API...");
    char* geminiResponse = processWithGemini(jsonPayload);
    
    if (!geminiResponse) {
      Serial.println("Gemini API processing failed");
      
      // Still save the JSON for viewing even if Gemini fails
      if (lastJsonPayload) {
        free(lastJsonPayload);
      }
      lastJsonPayload = jsonPayload;
      
      processingImage = false;
      return;
    }
    Serial.println("Gemini API processing successful :");
    Serial.println(geminiResponse);

    // Step 3: Parse the response
    Serial.println("Step 3: Parsing response...");
    int wasteType = parseGeminiResponse(geminiResponse);
    


    // Step 4: Signal the result
    Serial.println("Step 4: Signaling result...");
    signalResult(wasteType);
    
    // Free the response memory
    free(geminiResponse);
    
    // Save the JSON for the web server
    if (lastJsonPayload) {
      free(lastJsonPayload);
    }
    lastJsonPayload = jsonPayload;
    
    Serial.println("Process complete. Waiting for trigger to go LOW...");
    
    // Step 5: Wait for trigger to go LOW again
    while (digitalRead(TRIGGER_PIN) == HIGH) {
      server.handleClient();
      delay(10);
    }
    
    Serial.println("Trigger signal is LOW. Ready for next trigger.");
    processingImage = false;
  }
  
  // Small delay to prevent watchdog timer issues
  delay(10);
}

void setupServer() {
  // Simple home page
  server.on("/", HTTP_GET, []() {
    String html = "<html><body>";
    html += "<h1>ESP32-CAM Trash Classification System</h1>";
    html += "<p>System running and waiting for trigger signal on pin 12</p>";
    html += "<p><a href='/photo'>View Latest Capture</a></p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });
  
  // Route to serve the latest JSON payload
  server.on("/photo", HTTP_GET, []() {
    if (!lastJsonPayload) {
      server.send(404, "text/plain", "No image has been captured yet");
      return;
    }
    
    // Send as JSON
    server.sendHeader("Content-Type", "application/json");
    server.sendHeader("Content-Disposition", "inline; filename=capture.json");
    
    // Determine the length
    size_t jsonLength = strlen(lastJsonPayload);
    
    // Send the JSON data
    server.send(200, "application/json", lastJsonPayload);
  });
}

char* captureImage() {
  // Get image as JSON payload for Gemini
  size_t encodedSize = 0;
  return captureImageAsGeminiJson(DEFAULT_PROMPT, &encodedSize, GEMINI_API_KEY);
}

char* processWithGemini(char* jsonPayload) {
  return sendToGeminiAPI(jsonPayload, GEMINI_API_KEY);
}

int parseGeminiResponse(const char* response) {
  // Convert to lowercase for case-insensitive comparison
  String resp = String(response);
  resp.toLowerCase();
  
  // Simple keyword detection
  if (resp.indexOf("plastic") >= 0) {
    Serial.println("Detected Plastic");
    return TYPE_PLASTIC;
  } else if (resp.indexOf("cardboard") >= 0) {
    Serial.println("Detected Cardboard");
    return TYPE_CARDBOARD;
  } else if (resp.indexOf("paper") >= 0) {
    Serial.println("Detected Paper");
    return TYPE_PAPER;
  } else if (resp.indexOf("other") >= 0) {
    Serial.println("Detected Other");
    return TYPE_OTHER;
  } else if (resp.indexOf("none") >= 0) {
    Serial.println("Detected None");
    return TYPE_NONE;
  }
  
  // Default to OTHER if we can't determine
  Serial.println("Detected 404");
  return TYPE_404;
}

void signalResult(int wasteType) {
  // Single signal with length corresponding to waste type
  digitalWrite(OUTPUT_PIN, HIGH);
  delay(50 * wasteType);  // Length corresponds to waste type
  digitalWrite(OUTPUT_PIN, LOW);
}