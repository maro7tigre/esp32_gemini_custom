/**
 * custom_cam.cpp - Simplified camera implementation for ESP32-CAM
 * 
 * This file implements a minimal API for capturing images as Base64 strings
 * with fixed configuration for ease of use and memory optimization.
 * 
 * Copyright (c) 2025 
 * Licensed under MIT License
 */

 #include "custom_cam.h"
 #include <Arduino.h>
 #include "esp_camera.h"
 #include "esp_log.h"
 #include "driver/gpio.h"
 #include <string.h>
 
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
 
 /**
  * Initialize the camera with optimized settings
  */
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
         .frame_size = FRAMESIZE_VGA,  // 640x480
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
 static size_t calculateBase64Length(size_t input_length) {
     return ((input_length + 2) / 3 * 4) + 1;  // +1 for null terminator
 }
 
 /**
  * Convert binary data to Base64 string
  */
 static size_t encodeBase64(const uint8_t* input, size_t input_length, char* output, size_t output_size) {
     if (!input || !output || input_length == 0) {
         return 0;
     }
     
     size_t required_size = calculateBase64Length(input_length);
     if (output_size < required_size) {
         ESP_LOGE(TAG, "Output buffer too small for Base64 encoding");
         return 0;
     }
     
     size_t i, j;
     uint32_t a, b, c;
     
     for (i = 0, j = 0; i < input_length; i += 3, j += 4) {
         // Get next three bytes
         a = i < input_length ? input[i] : 0;
         b = (i + 1) < input_length ? input[i + 1] : 0;
         c = (i + 2) < input_length ? input[i + 2] : 0;
         
         // Pack into 24-bit integer
         uint32_t triple = (a << 16) | (b << 8) | c;
         
         // Encode to 4 Base64 characters
         output[j] = base64_table[(triple >> 18) & 0x3F];
         output[j + 1] = base64_table[(triple >> 12) & 0x3F];
         output[j + 2] = (i + 1) < input_length ? base64_table[(triple >> 6) & 0x3F] : '=';
         output[j + 3] = (i + 2) < input_length ? base64_table[triple & 0x3F] : '=';
     }
     
     // Null-terminate the string
     output[j] = '\0';
     
     return j;
 }
 
 /**
  * Capture an image and convert it to Base64
  */
 char* captureImageAsBase64(size_t* encoded_size) {
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
     
     // Calculate required buffer size and allocate
     size_t required_size = calculateBase64Length(fb->len);
     char* output_buffer = (char*)malloc(required_size);
     if (!output_buffer) {
         ESP_LOGE(TAG, "Failed to allocate memory for Base64 encoding");
         esp_camera_fb_return(fb);
         return NULL;
     }
     
     // Encode to Base64
     size_t encoded_len = encodeBase64(fb->buf, fb->len, output_buffer, required_size);
     
     // Free the camera frame buffer
     esp_camera_fb_return(fb);
     
     // Set encoded size if requested
     if (encoded_size) {
         *encoded_size = encoded_len;
     }
     
     ESP_LOGI(TAG, "Image encoded to Base64: %u bytes", encoded_len);
     return output_buffer;
 }