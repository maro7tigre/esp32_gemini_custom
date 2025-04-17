#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "credentials.h" // Keep your existing credentials.h with WIFI_SSID and WIFI_PASSWORD
#include "custom_cam.h"

// Add this to your credentials.h or define it here
// #define GEMINI_API_KEY "your-api-key-here"

WebServer server(80);
bool cameraInitialized = false;
unsigned long lastRequestTime = 0;
const unsigned long requestDelay = 1000; // 1 second between requests

// Default prompt for trash classification
const char* DEFAULT_PROMPT = "I want a short answer for which trash type do you see in the image [plastic, cardboard, paper or other], don't write anything else other than one of this list, if you can't see any trash just say None";

void setup() {
  // Start serial communication
  Serial.begin(9600); // keep it for compatibility with older versions
  delay(1000); // Allow serial to initialize
  
  Serial.println("\n\nESP32-CAM Base64 Image Server with Gemini AI");
  Serial.println("---------------------");
  
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
  
  // Define server routes
  server.on("/", HTTP_GET, []() {
    String html = "<html><body>";
    html += "<h1>ESP32-CAM Server with Gemini AI</h1>";
    html += "<p><a href='/photo'>Take Photo and Analyze</a></p>";
    html += "<p><a href='/photo?ai=false'>Take Photo without AI</a></p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });
  
  server.on("/photo", HTTP_GET, []() {
    // Check if we're being spammed with requests
    unsigned long currentTime = millis();
    if (currentTime - lastRequestTime < requestDelay) {
      server.send(429, "text/plain", "Too many requests. Please wait.");
      return;
    }
    lastRequestTime = currentTime;
    
    if (!cameraInitialized) {
      server.send(500, "text/plain", "Camera not initialized");
      return;
    }
    
    // Check if AI analysis is requested
    bool useAI = true;
    if (server.hasArg("ai")) {
      useAI = server.arg("ai").equalsIgnoreCase("true");
    }
    
    // Get custom prompt if provided
    const char* prompt = DEFAULT_PROMPT;
    if (server.hasArg("prompt")) {
      prompt = server.arg("prompt").c_str();
    }
    
    Serial.println("Photo requested" + String(useAI ? " with AI analysis" : ""));
    
    // Get image as JSON payload for Gemini
    size_t encoded_size = 0;
    char* json_payload = captureImageAsGeminiJson(prompt, &encoded_size, GEMINI_API_KEY);
    
    if (!json_payload) {
      server.send(503, "text/plain", "Failed to capture image");
      return;
    }
    
    // Process with Gemini if AI is requested, but just log the response
    if (useAI) {
      Serial.println("Sending to Gemini API...");
      char* gemini_response = sendToGeminiAPI(json_payload, GEMINI_API_KEY);
      
      if (gemini_response) {
        // Print response to serial only
        Serial.println("Gemini API Response:");
        Serial.println(gemini_response);
        
        // Free the response memory
        free(gemini_response);
      }
    }
    
    // Send as JSON
    server.setContentLength(encoded_size);
    server.sendHeader("Content-Type", "application/json");
    server.sendHeader("Content-Disposition", "inline; filename=capture.json");
    
    // Send the HTTP response header
    WiFiClient client = server.client();
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(encoded_size);
    client.println("Connection: close");
    client.println();
    
    // Send the JSON data in chunks
    const size_t CHUNK_SIZE = 8192; // 8KB chunks
    size_t remaining = encoded_size;
    char *pos = json_payload;
    
    while (remaining > 0) {
      size_t chunk = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;
      size_t sent = client.write((uint8_t*)pos, chunk);
      if (sent == 0) {
        Serial.println("Failed to send data chunk");
        break;
      }
      
      pos += sent;
      remaining -= sent;
      delay(1);  // Small delay to prevent WDT reset
    }
    
    // Free the allocated memory
    free(json_payload);
    
    client.stop();
    Serial.println("JSON payload sent successfully");
  });
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("\nReady! Use 'http://" + WiFi.localIP().toString() + "/photo' for JSON with Base64 encoded image and Gemini analysis");
}

void loop() {
  server.handleClient();
  delay(10); // Small delay to prevent watchdog timer issues
}