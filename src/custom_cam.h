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
 * Initialize the camera with optimized settings
 * @return true if successful, false on error
 */
bool initCamera(void);

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