#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "credentials.h" // Keep your existing credentials
#include "custom_cam.h"

WebServer server(80);
bool cameraInitialized = false;
unsigned long lastRequestTime = 0;
const unsigned long requestDelay = 1000; // 1 second between requests

void setup() {
  // Start serial communication
  Serial.begin(9600); // keep it for compatibility with older versions
  delay(1000); // Allow serial to initialize
  
  Serial.println("\n\nESP32-CAM Base64 Image Server");
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
    html += "<h1>ESP32-CAM Server</h1>";
    html += "<p><a href='/photo'>Take Photo</a></p>";
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
    
    Serial.println("Photo requested");
    
    // Get base64 encoded image
    size_t encoded_size = 0;
    char* base64_data = captureImageAsBase64(&encoded_size);
    
    if (!base64_data) {
      server.send(503, "text/plain", "Failed to capture image");
      return;
    }
    
    // Send as plain text
    server.setContentLength(encoded_size);
    server.sendHeader("Content-Type", "text/plain");
    server.sendHeader("Content-Disposition", "inline; filename=capture.txt");
    
    // Send the HTTP response header
    WiFiClient client = server.client();
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.print("Content-Length: ");
    client.println(encoded_size);
    client.println("Connection: close");
    client.println();
    
    // Send the Base64 data in chunks
    const size_t CHUNK_SIZE = 8192; // 8KB chunks
    size_t remaining = encoded_size;
    char *pos = base64_data;
    
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
    free(base64_data);
    
    client.stop();
    Serial.println("Base64 image sent successfully");
  });
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("\nReady! Use 'http://" + WiFi.localIP().toString() + "/photo' for Base64 encoded image");
}

void loop() {
  server.handleClient();
  delay(10); // Small delay to prevent watchdog timer issues
}