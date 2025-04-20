/**
 * custom_cam.cpp - Simplified camera implementation for ESP32-CAM with Gemini integration
 * 
 * This file implements a minimal API for capturing images as JSON payloads
 * with Gemini API integration for image analysis.
 * 
 * Licensed under GNU General Public License v3.0
 */
 #include "custom_cam.h"
 #include <Arduino.h>
 #include "esp_camera.h"
 #include "esp_log.h"
 #include "driver/gpio.h"
 #include <string.h>
 #include <WiFiClientSecure.h>
 
 // Tag for logging
 static const char* TAG = "custom_cam";
 
 // ESP32-CAM pin definitions (hardcoded for AI-Thinker ESP32-CAM)
 #define CAM_PIN_PWDN    32
 #define CAM_PIN_RESET   -1
 #define CAM_PIN_XCLK    0
 #define CAM_PIN_SIOD    26
 #define CAM_PIN_SIOC    27
 #define CAM_PIN_D7      35
 #define CAM_PIN_D6      34
 #define CAM_PIN_D5      39
 #define CAM_PIN_D4      36
 #define CAM_PIN_D3      21
 #define CAM_PIN_D2      19
 #define CAM_PIN_D1      18
 #define CAM_PIN_D0      5
 #define CAM_PIN_VSYNC   25
 #define CAM_PIN_HREF    23
 #define CAM_PIN_PCLK    22
 
 // Flash LED pin - using GPIO_NUM_4 enum for proper type
 #define FLASH_GPIO_PIN  GPIO_NUM_4
 
 // Base64 encoding table
 static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 
 // Gemini API host
 static const char* GEMINI_HOST = "generativelanguage.googleapis.com";
 static const int GEMINI_PORT = 443;
 
 /**
  * Initialize the camera with optimized settings
  */
 // MARK: -Initialize Camera
 bool initCamera(void) {
     // Set up the flash LED using Arduino functions
     pinMode(4, OUTPUT);
     digitalWrite(4, LOW);
     
     // Camera configuration with hardcoded pins for ESP32-CAM
     camera_config_t camera_config = {
         .pin_pwdn = CAM_PIN_PWDN,
         .pin_reset = CAM_PIN_RESET,
         .pin_xclk = CAM_PIN_XCLK,
         .pin_sccb_sda = CAM_PIN_SIOD,
         .pin_sccb_scl = CAM_PIN_SIOC,
         .pin_d7 = CAM_PIN_D7,
         .pin_d6 = CAM_PIN_D6,
         .pin_d5 = CAM_PIN_D5,
         .pin_d4 = CAM_PIN_D4,
         .pin_d3 = CAM_PIN_D3,
         .pin_d2 = CAM_PIN_D2,
         .pin_d1 = CAM_PIN_D1,
         .pin_d0 = CAM_PIN_D0,
         .pin_vsync = CAM_PIN_VSYNC,
         .pin_href = CAM_PIN_HREF,
         .pin_pclk = CAM_PIN_PCLK,
 
         // Set XCLK frequency to 20MHz for better performance
         .xclk_freq_hz = 20000000,
         .ledc_timer = LEDC_TIMER_0,
         .ledc_channel = LEDC_CHANNEL_0,
 
         // Fixed settings for good quality and memory efficiency
         .pixel_format = PIXFORMAT_JPEG,
         /*
            FRAMESIZE_QQVGA,   // 1: 160x120
            FRAMESIZE_QVGA,    // 2: 320x240
            FRAMESIZE_CIF,     // 3: 400x296
            FRAMESIZE_VGA,     // 4: 640x480
            FRAMESIZE_SVGA,    // 5: 800x600
            FRAMESIZE_XGA,     // 6: 1024x768
            FRAMESIZE_SXGA,    // 7: 1280x1024
            FRAMESIZE_UXGA     // 8: 1600x1200
         */
         .frame_size = FRAMESIZE_UXGA,  // 
         .jpeg_quality = 10,           // Good quality (0-63, lower is better)
         
         // Use 2 frame buffers and get the latest frame
         .fb_count = 2,
         .fb_location = CAMERA_FB_IN_PSRAM,
         .grab_mode = CAMERA_GRAB_LATEST  // Always get the freshest frame
     };
 
     // Initialize the camera
     esp_err_t err = esp_camera_init(&camera_config);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "Camera initialization failed with error 0x%x", err);
         return false;
     }
     
     // Fine-tune camera settings for better quality
     sensor_t* sensor = esp_camera_sensor_get();
     if (sensor) {
         sensor->set_brightness(sensor, 0);     // Default brightness
         sensor->set_contrast(sensor, 0);       // Default contrast
         sensor->set_saturation(sensor, 0);     // Default saturation
         sensor->set_sharpness(sensor, 0);      // Default sharpness
         sensor->set_denoise(sensor, 1);        // Light denoise
         sensor->set_whitebal(sensor, 1);       // Enable auto white balance
         sensor->set_exposure_ctrl(sensor, 1);  // Enable auto exposure
     }
     
     ESP_LOGI(TAG, "Camera initialized successfully");
     return true;
 }
 
 /**
  * Calculate required size for Base64 encoding
  */
 // MARK: -Base64 Length
 static size_t calculateBase64Length(size_t input_length) {
     return ((input_length + 2) / 3 * 4) + 1;  // +1 for null terminator
 }
 
 /**
  * Encode directly to JSON output buffer with minimal memory usage
  * The function writes directly to the output buffer, building the JSON payload as it goes
  */
 // MARK: -Encode to JSON
 static size_t encodeDirectlyToGeminiJson(
     const uint8_t* input_data, 
     size_t input_length, 
     char* output_buffer, 
     size_t output_buffer_size, 
     const char* prompt,
     const char* gemini_key) {
     
     if (!input_data || !output_buffer || input_length == 0) {
         return 0;
     }
     
     // Calculate JSON header length (approximate)
     const char* json_prefix = "{\n"
         "  \"contents\":[\n"
         "    {\n"
         "      \"parts\":[\n"
         "        {\"text\":\"";
     
     const char* prompt_suffix = "\"},\n"
         "        {\"inline_data\":{\n"
         "          \"mime_type\":\"image/jpeg\",\n"
         "          \"data\":\"";
     
     const char* json_suffix = "\"\n"
         "        }}\n"
         "      ]\n"
         "    }\n"
         "  ],\n"
         "  \"generationConfig\":{\n"
         "    \"maxOutputTokens\":5,\n"
         "    \"temperature\":1\n"
         "  }\n"
         "}";
     
     // Calculate total size needed
     size_t base64_length = calculateBase64Length(input_length) - 1; // -1 because we don't need null terminator in JSON
     size_t total_size = 
         strlen(json_prefix) + 
         strlen(prompt) + 
         strlen(prompt_suffix) + 
         base64_length + 
         strlen(json_suffix) + 
         1; // +1 for null terminator
     
     if (total_size > output_buffer_size) {
         ESP_LOGE(TAG, "Output buffer too small for JSON encoding: need %u, have %u", total_size, output_buffer_size);
         return 0;
     }
     
     // Start writing to buffer
     char* pos = output_buffer;
     
     // Copy JSON prefix
     strcpy(pos, json_prefix);
     pos += strlen(json_prefix);
     
     // Copy prompt (with escaping for quotes)
     for (const char* p = prompt; *p; p++) {
         if (*p == '"' || *p == '\\') {
             *pos++ = '\\';  // Escape quote or backslash
         }
         *pos++ = *p;
     }
     
     // Copy prompt suffix
     strcpy(pos, prompt_suffix);
     pos += strlen(prompt_suffix);
     
     // Encode image data to Base64 directly into the buffer
     size_t i;
     uint32_t a, b, c;
     
     for (i = 0; i < input_length; i += 3) {
         // Get next three bytes
         a = i < input_length ? input_data[i] : 0;
         b = (i + 1) < input_length ? input_data[i + 1] : 0;
         c = (i + 2) < input_length ? input_data[i + 2] : 0;
         
         // Pack into 24-bit integer
         uint32_t triple = (a << 16) | (b << 8) | c;
         
         // Encode to 4 Base64 characters
         *pos++ = base64_table[(triple >> 18) & 0x3F];
         *pos++ = base64_table[(triple >> 12) & 0x3F];
         *pos++ = (i + 1) < input_length ? base64_table[(triple >> 6) & 0x3F] : '=';
         *pos++ = (i + 2) < input_length ? base64_table[triple & 0x3F] : '=';
     }
     
     // Copy JSON suffix
     strcpy(pos, json_suffix);
     pos += strlen(json_suffix);
     
     // Return total length
     return pos - output_buffer;
 }
 
 /**
  * Capture an image and convert it to a JSON payload for Gemini API
  */
 // MARK: -Capture Img as JSON
 char* captureImageAsGeminiJson(const char* prompt, size_t* encoded_size, const char* gemini_key) {
     if (!prompt || !gemini_key) {
         ESP_LOGE(TAG, "Prompt or API key is NULL");
         return NULL;
     }
     
     // Turn on flash
     digitalWrite(4, HIGH);
     
     // Wait for flash to stabilize (75ms)
     delay(75);
     
     // Capture frame (get latest frame)
     camera_fb_t* fb = esp_camera_fb_get();
     
     // Turn off flash immediately
     digitalWrite(4, LOW);
     
     if (!fb) {
         ESP_LOGE(TAG, "Camera capture failed");
         return NULL;
     }
     
     // Verify we got a JPEG image
     if (fb->format != PIXFORMAT_JPEG || fb->len < 2 || fb->buf[0] != 0xFF || fb->buf[1] != 0xD8) {
         ESP_LOGE(TAG, "Invalid JPEG data captured");
         esp_camera_fb_return(fb);
         return NULL;
     }
     
     ESP_LOGI(TAG, "Picture taken: %dx%d, size: %u bytes", fb->width, fb->height, fb->len);
     
     // Calculate output size needed for JSON payload
     size_t base64_len = calculateBase64Length(fb->len);
     size_t json_overhead = 500; // JSON structure overhead (conservative estimate)
     size_t prompt_len = strlen(prompt) * 2; // Account for possible escaping of special chars
     size_t buffer_size = base64_len + json_overhead + prompt_len;
     
     // Allocate buffer for JSON output
     char* json_buffer = (char*)malloc(buffer_size);
     if (!json_buffer) {
         ESP_LOGE(TAG, "Failed to allocate memory for JSON encoding");
         esp_camera_fb_return(fb);
         return NULL;
     }
     
     // Encode directly to JSON
     size_t json_len = encodeDirectlyToGeminiJson(fb->buf, fb->len, json_buffer, buffer_size, prompt, gemini_key);
     
     // Free the camera frame buffer as soon as possible
     esp_camera_fb_return(fb);
     
     if (json_len == 0) {
         ESP_LOGE(TAG, "JSON encoding failed");
         free(json_buffer);
         return NULL;
     }
     
     // Set encoded size if requested
     if (encoded_size) {
         *encoded_size = json_len;
     }
     
     ESP_LOGI(TAG, "Image encoded to JSON payload: %u bytes", json_len);
     return json_buffer;
 }
 
 /**
  * Send the JSON payload to Gemini API and get the response
  */
 char* sendToGeminiAPI(const char* json_payload, const char* gemini_key) {
     if (!json_payload || !gemini_key) {
         ESP_LOGE(TAG, "JSON payload or API key is NULL");
         return NULL;
     }
     
     // Create secure client
     WiFiClientSecure client;
     client.setInsecure(); // Skip certificate validation
     
     ESP_LOGI(TAG, "Connecting to Gemini API...");
     
     // Connect to Gemini API
     if (!client.connect(GEMINI_HOST, GEMINI_PORT)) {
         ESP_LOGE(TAG, "Connection to Gemini API failed");
         return NULL;
     }
     
     // Build API URL
     char url[256];
     snprintf(url, sizeof(url), 
              "/v1beta/models/gemini-2.0-flash-lite:generateContent?key=%s", 
              gemini_key);
     
     // Calculate payload length
     size_t payload_len = strlen(json_payload);
     
     // Build HTTP request
     ESP_LOGI(TAG, "Sending request to Gemini API...");
     client.printf("POST %s HTTP/1.1\r\n", url);
     client.printf("Host: %s\r\n", GEMINI_HOST);
     client.println("Content-Type: application/json");
     client.printf("Content-Length: %u\r\n", payload_len);
     client.println("Connection: close");
     client.println();
     
     // Send payload in chunks to avoid memory issues
     const size_t CHUNK_SIZE = 1024;
     const char* pos = json_payload;
     size_t remaining = payload_len;
     
     while (remaining > 0) {
         size_t chunk = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;
         size_t sent = client.write((const uint8_t*)pos, chunk);
         if (sent == 0) {
             ESP_LOGE(TAG, "Failed to send data chunk");
             client.stop();
             return NULL;
         }
         
         pos += sent;
         remaining -= sent;
         // Small delay to avoid overwhelming the WiFi stack
         delay(1);
     }
     
     // Wait for response with timeout
     unsigned long timeout = millis();
     while (client.connected() && !client.available()) {
         if (millis() - timeout > 10000) { // 10 second timeout
             ESP_LOGE(TAG, "Response timeout");
             client.stop();
             return NULL;
         }
         delay(100);
     }
     
     ESP_LOGI(TAG, "Receiving response...");
     
     // Skip HTTP headers
     while (client.connected()) {
         String line = client.readStringUntil('\n');
         if (line == "\r") {
             break; // End of headers
         }
     }
     
     // Read response body
     String response = client.readString();
     client.stop();
     
     // Allocate buffer for response
     char* response_buffer = (char*)malloc(response.length() + 1);
     if (!response_buffer) {
         ESP_LOGE(TAG, "Failed to allocate memory for response");
         return NULL;
     }
     
     // Copy response
     strcpy(response_buffer, response.c_str());
     
     ESP_LOGI(TAG, "Gemini API response received: %u bytes", response.length());
     return response_buffer;
 }