/**
 * @file esp_cam_utils.h
 * @brief Main API for ESP32-CAM Gemini Integration
 */

 #pragma once

 #include "esp_camera.h"
 #include "esp_err.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Initialize camera with specific quality settings
  * 
  * @param frame_size Camera resolution
  * @param jpeg_quality JPEG compression quality (0-63, lower = better quality)
  * @return esp_err_t ESP_OK on success
  */
 esp_err_t ESP_CAM_Init(framesize_t frame_size, int jpeg_quality);
 
 /**
  * @brief Capture image and create Gemini API JSON payload
  * 
  * @param gemini_url Gemini API endpoint URL
  * @param api_key Your Gemini API key
  * @param prompt Text prompt to send with the image
  * @return char* JSON payload string (must be freed with ESP_CAM_FreePayload)
  */
 char* ESP_CAM_CaptureAndGetGeminiPayload(const char* gemini_url, const char* api_key, const char* prompt);
 
 /**
  * @brief Free memory allocated for payload
  * 
  * @param payload Payload created by ESP_CAM_CaptureAndGetGeminiPayload
  */
 void ESP_CAM_FreePayload(char* payload);
 
 /**
  * @brief Send request to Gemini API and get response
  * 
  * @param gemini_url Gemini API endpoint URL
  * @param api_key Your Gemini API key
  * @param payload JSON payload (from ESP_CAM_CaptureAndGetGeminiPayload)
  * @return char* JSON response (must be freed with ESP_CAM_FreePayload)
  */
 char* ESP_CAM_SendGeminiRequest(const char* gemini_url, const char* api_key, const char* payload);
 
 /**
  * @brief Extract text response from Gemini API JSON
  * 
  * @param json_response JSON response from Gemini API
  * @return char* Extracted text (must be freed with ESP_CAM_FreePayload)
  */
 char* ESP_CAM_ExtractGeminiResponse(const char* json_response);
 
 /**
  * @brief Flush camera buffer (discard stale frame)
  */
 void ESP_CAM_FlushBuffer(void);
 
 /**
  * @brief All-in-one function to capture image and get response from Gemini
  * 
  * @param gemini_url Gemini API endpoint URL
  * @param api_key Your Gemini API key
  * @param model Gemini model name (e.g., "gemini-pro-vision")
  * @param prompt Text prompt to send with the image
  * @return char* Response text (must be freed with ESP_CAM_FreePayload)
  */
 char* ESP_CAM_CaptureAndAnalyze(const char* gemini_url, const char* api_key, 
                                const char* model, const char* prompt);
 
 #ifdef __cplusplus
 }
 #endif