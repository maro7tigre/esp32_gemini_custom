#ifndef CUSTOM_CAM_H
#define CUSTOM_CAM_H

#include <Arduino.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the camera with UXGA quality settings
 * @return true if successful, false on error
 */
bool initCamera(void);

/**
 * Capture a stable frame by detecting minimal changes between consecutive frames
 * 
 * This function temporarily disables auto-exposure to get stable frames
 * and returns the current frame when it's static enough
 * 
 * @return Pointer to captured frame buffer or NULL on failure (must be freed with esp_camera_fb_return())
 */
camera_fb_t* captureStaticFrame(void);

/**
 * Capture an image and convert it to a JSON payload for Gemini API
 * @param prompt The text prompt to send to Gemini
 * @param encoded_size Optional pointer to receive the JSON size
 * @param gemini_key The Gemini API key
 * @return Pointer to the JSON payload (must be freed with free())
 */
char* captureImageAsGeminiJson(const char* prompt, size_t* encoded_size, const char* gemini_key);

/**
 * Send the image to Gemini API and get response
 * @param json_payload The JSON payload (from captureImageAsGeminiJson)
 * @param gemini_key The Gemini API key
 * @return Pointer to the response string (must be freed with free())
 */
char* sendToGeminiAPI(const char* json_payload, const char* gemini_key);

#ifdef __cplusplus
}
#endif

#endif /* CUSTOM_CAM_H */