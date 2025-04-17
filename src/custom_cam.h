/**
 * custom_cam.h - Simplified camera interface for ESP32-CAM
 * 
 * This file provides a minimal API for capturing images as Base64 strings
 * with no control parameters needed by the user.
 * 
 * Copyright (c) 2025 
 * Licensed under MIT License
 */

 #ifndef CUSTOM_CAM_H
 #define CUSTOM_CAM_H
 
 #include <Arduino.h>
 #include <stdbool.h>
 #include <stdint.h>
 #include <stdlib.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * Initialize the camera with optimized default settings
  * Uses VGA resolution (640x480) and good quality settings
  * 
  * @return true if successful, false on error
  */
 bool initCamera(void);
 
 /**
  * Capture an image and convert it to Base64
  * Uses the flash LED automatically
  * 
  * @param encoded_size Optional pointer to receive the size of the encoded data
  * @return Pointer to the Base64 encoded string (must be freed with free())
  */
 char* captureImageAsBase64(size_t* encoded_size);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* CUSTOM_CAM_H */