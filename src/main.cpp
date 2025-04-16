#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "credentials.h"
#include "custom_cam.h"

WebServer server(80);
bool cameraInitialized = false;

// Function prototypes
void handleRoot();
void handleCapture();

void setup() {
  // Start serial communication
  Serial.begin(9600);
  Serial.println("\n\nESP32-CAM Image Server");
  Serial.println("---------------------");
  
  // Initialize WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Connected to: ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize camera with VGA resolution (640x480) and good quality (10)
  esp_err_t err = custom_cam_init(FRAMESIZE_VGA, 10);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  cameraInitialized = true;
  Serial.println("Camera initialized successfully");
  
  // Define server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/capture", HTTP_GET, handleCapture);
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("\nReady! Use 'http://" + WiFi.localIP().toString() + "/capture' to take a photo");
}

void loop() {
  server.handleClient();
}

// Serve simple HTML page
void handleRoot() {
  server.send(200, "text/plain", "ESP32-CAM Server is running. Use /capture to take a photo.");
}

// Capture and send image
void handleCapture() {
  if (!cameraInitialized) {
    server.send(500, "text/plain", "Camera not initialized");
    return;
  }
  
  Serial.println("Taking picture...");
  
  // Take picture with flash
  camera_fb_t *fb = custom_cam_take_picture(true, 100);
  if (!fb) {
    Serial.println("Camera capture failed");
    server.send(500, "text/plain", "Failed to capture image");
    return;
  }
  
  Serial.printf("Picture taken! Size: %zu bytes\n", fb->len);
  
  // Set content type and send image
  server.sendHeader("Content-Type", "image/jpeg");
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.setContentLength(fb->len);
  server.send(200, "image/jpeg", "");
  
  // Send the actual image data
  WiFiClient client = server.client();
  client.write(fb->buf, fb->len);
  
  // Return the frame buffer
  custom_cam_return_fb(fb);
  
  Serial.println("Image sent");
}