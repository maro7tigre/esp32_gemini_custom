/**
 * custom_cam.h - Simplified camera interface for ESP32-CAM with Gemini integration
 * 
 * This file provides a minimal API for capturing images as Base64 strings
 * with Gemini API integration for image analysis.
 * 
 * Licensed under GNU General Public License v3.0
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
  * Capture an image and convert it to a JSON payload with Base64 encoding
  * for Gemini API, using the flash LED automatically
  * 
  * @param prompt The text prompt to send to Gemini
  * @param encoded_size Optional pointer to receive the size of the encoded JSON payload
  * @param gemini_key The Gemini API key to use
  * @return Pointer to the JSON payload string (must be freed with free())
  */
 char* captureImageAsGeminiJson(const char* prompt, size_t* encoded_size, const char* gemini_key);
 
 /**
  * Send the image to Gemini API and get response
  * 
  * @param json_payload The JSON payload to send (created by captureImageAsGeminiJson)
  * @param gemini_key The Gemini API key to use
  * @return Pointer to the response string (must be freed with free()) or NULL on error
  */
 char* sendToGeminiAPI(const char* json_payload, const char* gemini_key);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* CUSTOM_CAM_H */