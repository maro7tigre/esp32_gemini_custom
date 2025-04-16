/**
 * custom_cam.c - Simplified OV2640 camera implementation for ESP32-CAM
 * 
 * This file provides a minimal implementation for capturing JPEG images
 * from an OV2640 camera on ESP32-CAM with flash control.
 * 
 * Copyright (c) 2025 
 * Licensed under MIT License
 */

 #include <stdio.h>
 #include <string.h>
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "esp_camera.h"
 #include "esp_log.h"
 #include "driver/gpio.h"
 
 static const char* TAG = "custom_cam";
 
 // ESP32-CAM (AI-Thinker) pin definitions - hardcoded for simplicity
 #define CAM_PIN_PWDN    32
 #define CAM_PIN_RESET   -1 // Not connected, use software reset
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
 
 // Flash LED pin
 #define FLASH_GPIO_PIN  4
 
 /**
  * Initialize camera with minimal configuration
  * 
  * @param framesize Resolution to use (from FRAMESIZE_* enum)
  * @param jpeg_quality JPEG quality (0-63, lower means better quality but larger size)
  * @return ESP_OK if successful, otherwise an error code
  */
 esp_err_t custom_cam_init(framesize_t framesize, int jpeg_quality)
 {
     // Sanity checks
     if (jpeg_quality < 0) jpeg_quality = 0;
     if (jpeg_quality > 63) jpeg_quality = 63;
 
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
 
         // Use JPEG mode for lower memory usage and direct format for photos
         .pixel_format = PIXFORMAT_JPEG,
         .frame_size = framesize,
         .jpeg_quality = jpeg_quality,
         
         // Use minimum frame buffers to save memory
         .fb_count = 1,
         .fb_location = CAMERA_FB_IN_PSRAM,
         .grab_mode = CAMERA_GRAB_WHEN_EMPTY
     };
 
     // Initialize flash LED pin
     gpio_config_t gpio_conf = {
         .pin_bit_mask = (1ULL << FLASH_GPIO_PIN),
         .mode = GPIO_MODE_OUTPUT,
         .pull_up_en = GPIO_PULLUP_DISABLE,
         .pull_down_en = GPIO_PULLDOWN_DISABLE,
         .intr_type = GPIO_INTR_DISABLE
     };
     
     esp_err_t err = gpio_config(&gpio_conf);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "Error configuring flash LED GPIO: %s", esp_err_to_name(err));
         // Continue anyway, camera can work without flash
     }
     
     // Initialize the camera
     err = esp_camera_init(&camera_config);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "Camera initialization failed with error 0x%x", err);
         return err;
     }
     
     ESP_LOGI(TAG, "Camera initialized successfully");
     return ESP_OK;
 }
 
 /**
  * Take a picture with optional flash
  * 
  * @param use_flash Whether to use the flash LED during capture
  * @param flash_delay_ms How long to keep the flash on before capture (milliseconds)
  * @return Camera frame buffer pointer if successful, NULL otherwise
  *         (must be returned with custom_cam_return_fb when done)
  */
 camera_fb_t* custom_cam_take_picture(bool use_flash, int flash_delay_ms)
 {
     // Turn on flash if requested
     if (use_flash) {
         gpio_set_level(FLASH_GPIO_PIN, 1);
         // Wait for flash to stabilize and scene to be properly exposed
         if (flash_delay_ms > 0) {
             vTaskDelay(flash_delay_ms / portTICK_PERIOD_MS);
         } else {
             vTaskDelay(75 / portTICK_PERIOD_MS); // Default delay
         }
     }
     
     // Capture frame
     camera_fb_t* fb = esp_camera_fb_get();
     
     // Turn off flash immediately after capture
     if (use_flash) {
         gpio_set_level(FLASH_GPIO_PIN, 0);
     }
     
     if (!fb) {
         ESP_LOGE(TAG, "Camera capture failed");
         return NULL;
     }
     
     // Verify we got a JPEG image by checking for JPEG header/footer
     if (fb->format != PIXFORMAT_JPEG) {
         ESP_LOGE(TAG, "Captured image is not in JPEG format");
         esp_camera_fb_return(fb);
         return NULL;
     }
     
     // Basic validation of JPEG data - check for JPEG SOI marker (0xFFD8) at start
     if (fb->len < 2 || fb->buf[0] != 0xFF || fb->buf[1] != 0xD8) {
         ESP_LOGE(TAG, "JPEG data appears to be invalid (missing SOI marker)");
         esp_camera_fb_return(fb);
         return NULL;
     }
     
     ESP_LOGI(TAG, "Picture taken successfully: %dx%d, size: %u bytes", 
              fb->width, fb->height, fb->len);
     
     return fb;
 }
 
 /**
  * Return a camera frame buffer when done using it
  * 
  * @param fb Frame buffer returned by custom_cam_take_picture
  */
 void custom_cam_return_fb(camera_fb_t* fb)
 {
     if (fb) {
         esp_camera_fb_return(fb);
     }
 }
 
 /**
  * Deinitialize the camera and free resources
  * 
  * @return ESP_OK if successful, otherwise an error code
  */
 esp_err_t custom_cam_deinit(void)
 {
     return esp_camera_deinit();
 }
 
 /**
  * Get the camera sensor to adjust settings
  * 
  * @return Pointer to sensor_t structure or NULL if error
  */
 sensor_t* custom_cam_get_sensor(void)
 {
     return esp_camera_sensor_get();
 }
 
 /**
  * Helper function to set camera parameters via the sensor API
  * Common parameters to adjust: 
  * - brightness (-2 to 2)
  * - contrast (-2 to 2)
  * - saturation (-2 to 2)
  * - sharpness (-2 to 2)
  * - denoise (0 to 255)
  * - special_effect (0=none, 1=negative, 2=grayscale, 3=red tint, 4=green tint, 5=blue tint, 6=sepia)
  *
  * @param key Setting name
  * @param value Setting value
  * @return ESP_OK if successful, ESP_FAIL if error
  */
 esp_err_t custom_cam_set_parameter(const char* key, int value)
 {
     sensor_t* sensor = custom_cam_get_sensor();
     if (!sensor) {
         ESP_LOGE(TAG, "Failed to get camera sensor");
         return ESP_FAIL;
     }
     
     if (strcmp(key, "framesize") == 0) {
         return sensor->set_framesize(sensor, (framesize_t)value);
     } else if (strcmp(key, "quality") == 0) {
         return sensor->set_quality(sensor, value);
     } else if (strcmp(key, "contrast") == 0) {
         return sensor->set_contrast(sensor, value);
     } else if (strcmp(key, "brightness") == 0) {
         return sensor->set_brightness(sensor, value);
     } else if (strcmp(key, "saturation") == 0) {
         return sensor->set_saturation(sensor, value);
     } else if (strcmp(key, "sharpness") == 0) {
         return sensor->set_sharpness(sensor, value);
     } else if (strcmp(key, "denoise") == 0) {
         return sensor->set_denoise(sensor, value);
     } else if (strcmp(key, "special_effect") == 0) {
         return sensor->set_special_effect(sensor, value);
     } else if (strcmp(key, "hmirror") == 0) {
         return sensor->set_hmirror(sensor, value);
     } else if (strcmp(key, "vflip") == 0) {
         return sensor->set_vflip(sensor, value);
     } else if (strcmp(key, "awb") == 0) { // Auto white balance
         return sensor->set_whitebal(sensor, value);
     } else if (strcmp(key, "aec") == 0) { // Auto exposure control
         return sensor->set_exposure_ctrl(sensor, value);
     }
     
     ESP_LOGW(TAG, "Unknown parameter: %s", key);
     return ESP_FAIL;
 }