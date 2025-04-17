#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "credentials.h" // Keep your existing credentials
#include "custom_cam.h"
#include "encoder.h"

WebServer server(80);
bool cameraInitialized = false;

// Function prototypes
void handleRoot();
void handleBase64Capture();
void serveBase64();

void setup() {
  // Start serial communication
  Serial.begin(9600); // Increased baud rate for better debugging
  Serial.println("\n\nESP32-CAM Base64 Image Server");
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
  
  // Configure camera settings for better performance
  sensor_t* sensor = custom_cam_get_sensor();
  if (sensor) {
    sensor->set_framesize(sensor, FRAMESIZE_VGA);
    sensor->set_quality(sensor, 10);
    sensor->set_brightness(sensor, 0);
    sensor->set_saturation(sensor, 0);
  }
  
  Serial.println("Camera initialized successfully");
  
  // Define server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/base64", HTTP_GET, handleBase64Capture);
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("\nReady! Use:");
  Serial.println("- 'http://" + WiFi.localIP().toString() + "/base64' for Base64 encoded image");
}

void loop() {
  server.handleClient();
}

// Serve simple HTML page
void handleRoot() {
  String html = "<html><body>";
  html += "<h1>ESP32-CAM Server</h1>";
  html += "<p><a href='/base64'>Take Photo (Base64)</a></p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Handle Base64 encoded capture request
void handleBase64Capture() {
  if (!cameraInitialized) {
    server.send(500, "text/plain", "Camera not initialized");
    return;
  }
  
  Serial.println("Taking sequential pictures (Base64)...");
  serveBase64();
}

// Function to capture and send Base64 encoded image
void serveBase64() {
  // Allocate memory for the Base64 string
  size_t jpeg_size = 0;
  size_t encoded_size = 0;
  
  // Take the first image and discard it
  Serial.println("Taking first image (to discard)...");
  camera_fb_t *first_fb = custom_cam_take_picture(true, 50);
  if (first_fb) {
    // Return the frame buffer immediately
    custom_cam_return_fb(first_fb);
  }
  
  // Wait for 50ms
  delay(50);
  
  // Take the second image to use
  Serial.println("Taking second image (to use)...");
  
  // Use the efficient memory-managed function to capture and encode
  char* base64_data = capture_jpeg_as_base64_alloc(true, &encoded_size, &jpeg_size);
  
  if (!base64_data) {
    Serial.println("Failed to capture or encode image");
    server.send(503, "text/plain", "Failed to capture or encode image");
    return;
  }
  
  Serial.printf("Picture encoded! Original: %zu bytes, Base64: %zu bytes\n", 
                jpeg_size, encoded_size);
  
  // Send as plain text - browsers will display directly
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
    
    // Send chunk and check if it was successful
    size_t sent = client.write((uint8_t*)pos, chunk);
    if (sent == 0) {
      Serial.println("Failed to send data chunk");
      break;
    }
    
    pos += sent;
    remaining -= sent;
    // Small delay to prevent WDT reset
    delay(1);
  }
  
  // Free the allocated memory
  free(base64_data);
  
  client.stop();
  Serial.println("Base64 image sent successfully");
}