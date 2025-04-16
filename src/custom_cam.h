/**
 * custom_cam.h - Simplified OV2640 camera implementation for ESP32-CAM
 * 
 * This file provides a minimal API for capturing JPEG images
 * from an OV2640 camera on ESP32-CAM with flash control.
 * 
 * Copyright (c) 2025 
 * Licensed under MIT License
 */

 #ifndef CUSTOM_CAM_H
 #define CUSTOM_CAM_H
 
 #include "esp_err.h"
 #include "esp_camera.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * Initialize camera with minimal configuration
  * 
  * @param framesize Resolution to use (from framesize_t enum)
  *        Recommended values:
  *        - FRAMESIZE_VGA (640x480)
  *        - FRAMESIZE_SVGA (800x600)
  *        - FRAMESIZE_XGA (1024x768)
  *        - FRAMESIZE_UXGA (1600x1200)
  * @param jpeg_quality JPEG quality (0-63, lower means better quality but larger size)
  * @return ESP_OK if successful, otherwise an error code
  */
 esp_err_t custom_cam_init(framesize_t framesize, int jpeg_quality);
 
 /**
  * Take a picture with optional flash
  * 
  * @param use_flash Whether to use the flash LED during capture
  * @param flash_delay_ms How long to keep the flash on before capture (milliseconds)
  * @return Camera frame buffer pointer if successful, NULL otherwise
  *         (must be returned with custom_cam_return_fb when done)
  */
 camera_fb_t* custom_cam_take_picture(bool use_flash, int flash_delay_ms);
 
 /**
  * Return a camera frame buffer when done using it
  * 
  * @param fb Frame buffer returned by custom_cam_take_picture
  */
 void custom_cam_return_fb(camera_fb_t* fb);
 
 /**
  * Deinitialize the camera and free resources
  * 
  * @return ESP_OK if successful, otherwise an error code
  */
 esp_err_t custom_cam_deinit(void);
 
 /**
  * Get the camera sensor to adjust settings
  * 
  * @return Pointer to sensor_t structure or NULL if error
  */
 sensor_t* custom_cam_get_sensor(void);
 
 /**
  * Helper function to set camera parameters via the sensor API
  * Common parameters to adjust: 
  * - framesize (0-10, see enum in esp_camera.h)
  * - quality (0-63, lower means better quality)
  * - brightness (-2 to 2)
  * - contrast (-2 to 2)
  * - saturation (-2 to 2)
  * - sharpness (-2 to 2)
  * - denoise (0 to 255)
  * - special_effect (0=none, 1=negative, 2=grayscale, 3=red tint, 4=green tint, 5=blue tint, 6=sepia)
  * - hmirror (0=disable, 1=enable)
  * - vflip (0=disable, 1=enable)
  * - awb (0=disable, 1=enable auto white balance)
  * - aec (0=disable, 1=enable auto exposure)
  *
  * @param key Setting name
  * @param value Setting value
  * @return ESP_OK if successful, ESP_FAIL if error
  */
 esp_err_t custom_cam_set_parameter(const char* key, int value);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* CUSTOM_CAM_H */