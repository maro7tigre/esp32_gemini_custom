/**
 * @file esp_cam_utils.c
 * @brief Main API implementation for ESP32-CAM Gemini Integration
 */

 #include "esp_cam_utils.h"
 #include "esp_cam_base64.h"
 #include "esp_cam_json.h"
 #include "gemini_client.h"
 #include "esp_log.h"
 #include "esp_heap_caps.h"
 #include <string.h>
 
 static const char *TAG = "esp_cam_utils";
 
 // Chunk sizes for processing
 #define JSON_BUFFER_SIZE 1024
 #define BASE64_CHUNK_SIZE 1024
 
 esp_err_t ESP_CAM_Init(framesize_t frame_size, int jpeg_quality) {
     // Camera configuration
     camera_config_t camera_config = {
         .pin_pwdn = CONFIG_CAMERA_PIN_PWDN,
         .pin_reset = CONFIG_CAMERA_PIN_RESET,
         .pin_xclk = CONFIG_CAMERA_PIN_XCLK,
         .pin_sccb_sda = CONFIG_CAMERA_PIN_SIOD,
         .pin_sccb_scl = CONFIG_CAMERA_PIN_SIOC,
         .pin_d7 = CONFIG_CAMERA_PIN_D7,
         .pin_d6 = CONFIG_CAMERA_PIN_D6,
         .pin_d5 = CONFIG_CAMERA_PIN_D5,
         .pin_d4 = CONFIG_CAMERA_PIN_D4,
         .pin_d3 = CONFIG_CAMERA_PIN_D3,
         .pin_d2 = CONFIG_CAMERA_PIN_D2,
         .pin_d1 = CONFIG_CAMERA_PIN_D1,
         .pin_d0 = CONFIG_CAMERA_PIN_D0,
         .pin_vsync = CONFIG_CAMERA_PIN_VSYNC,
         .pin_href = CONFIG_CAMERA_PIN_HREF,
         .pin_pclk = CONFIG_CAMERA_PIN_PCLK,
         
         .xclk_freq_hz = 20000000,
         .ledc_timer = LEDC_TIMER_0,
         .ledc_channel = LEDC_CHANNEL_0,
         
         .pixel_format = PIXFORMAT_JPEG,
         .frame_size = frame_size,
         .jpeg_quality = jpeg_quality,
         .fb_count = 1,
         .grab_mode = CAMERA_GRAB_WHEN_EMPTY
     };
     
     // Initialize the camera
     esp_err_t ret = esp_camera_init(&camera_config);
     if (ret != ESP_OK) {
         ESP_LOGE(TAG, "Camera initialization failed with error 0x%x", ret);
         return ret;
     }
     
     ESP_LOGI(TAG, "Camera initialized successfully");
     return ESP_OK;
 }
 
 void ESP_CAM_FlushBuffer(void) {
     // Capture and discard one frame to ensure we get a fresh frame next time
     camera_fb_t *fb = esp_camera_fb_get();
     if (fb) {
         esp_camera_fb_return(fb);
         vTaskDelay(50 / portTICK_PERIOD_MS); // Short delay to allow the camera to update
     }
 }
 
 void ESP_CAM_FreePayload(char* payload) {
     if (payload) {
         free(payload);
     }
 }
 
 // Helper to extract model name from URL
 static char* extract_model_from_url(const char* url) {
     // Format example: https://generativelanguage.googleapis.com/v1beta/models/gemini-pro-vision:generateContent
     
     // Find last slash
     const char* last_slash = strrchr(url, '/');
     if (!last_slash) return NULL;
     
     // Find colon
     const char* colon = strchr(last_slash, ':');
     if (!colon) return NULL;
     
     // Extract model name
     size_t model_len = colon - (last_slash + 1);
     char* model = (char*)malloc(model_len + 1);
     if (!model) return NULL;
     
     memcpy(model, last_slash + 1, model_len);
     model[model_len] = '\0';
     
     return model;
 }
 
 char* ESP_CAM_CaptureAndGetGeminiPayload(const char* gemini_url, const char* api_key, const char* prompt) {
     // Sanity checks
     if (!gemini_url || !api_key || !prompt) {
         ESP_LOGE(TAG, "Invalid parameters");
         return NULL;
     }
     
     // Capture an image
     ESP_LOGI(TAG, "Capturing image...");
     camera_fb_t *fb = esp_camera_fb_get();
     if (!fb) {
         ESP_LOGE(TAG, "Camera capture failed");
         return NULL;
     }
     
     ESP_LOGI(TAG, "Image captured: %u bytes", fb->len);
     
     // Allocate memory for the JSON
     // Calculate approximate size needed for JSON
     size_t base64_len = esp_cam_base64_encode_len(fb->len);
     size_t json_size = base64_len + strlen(prompt) + 200; // 200 bytes for JSON structure
     
     char *json_payload = (char*)malloc(json_size);
     if (!json_payload) {
         ESP_LOGE(TAG, "Failed to allocate memory for JSON payload");
         esp_camera_fb_return(fb);
         return NULL;
     }
     
     // Initialize JSON builder
     esp_cam_json_state_t json_state;
     esp_cam_json_init(&json_state);
     
     // Start building JSON
     size_t json_len = 0;
     esp_cam_json_start_object(&json_state, json_payload, &json_len, json_size);
     
     // Add contents array
     esp_cam_json_key(&json_state, json_payload, &json_len, json_size, "contents");
     esp_cam_json_start_array(&json_state, json_payload, &json_len, json_size);
     
     // Add first content object
     esp_cam_json_start_object(&json_state, json_payload, &json_len, json_size);
     
     // Add parts array
     esp_cam_json_key(&json_state, json_payload, &json_len, json_size, "parts");
     esp_cam_json_start_array(&json_state, json_payload, &json_len, json_size);
     
     // Add text part
     esp_cam_json_start_object(&json_state, json_payload, &json_len, json_size);
     esp_cam_json_key_string(&json_state, json_payload, &json_len, json_size, "text", prompt);
     esp_cam_json_end_object(&json_state, json_payload, &json_len, json_size);
     
     // Add image part
     esp_cam_json_start_object(&json_state, json_payload, &json_len, json_size);
     
     // Add inline_data object
     esp_cam_json_key(&json_state, json_payload, &json_len, json_size, "inline_data");
     esp_cam_json_start_object(&json_state, json_payload, &json_len, json_size);
     
     // Add mime_type
     esp_cam_json_key_string(&json_state, json_payload, &json_len, json_size, "mime_type", "image/jpeg");
     
     // Add data key
     esp_cam_json_key(&json_state, json_payload, &json_len, json_size, "data");
     
     // Reserve space for base64 data (add opening quote)
     if (json_len + 1 >= json_size) {
         ESP_LOGE(TAG, "JSON buffer overflow");
         free(json_payload);
         esp_camera_fb_return(fb);
         return NULL;
     }
     json_payload[json_len++] = '"';
     
     // Encode image to base64 directly into the JSON buffer, with error handling
     size_t remaining = json_size - json_len - 2; // Leave space for closing quote and null
     char *base64_output = json_payload + json_len;
     if (esp_cam_base64_encode(fb->buf, fb->len, base64_output) != ESP_OK) {
         ESP_LOGE(TAG, "Base64 encoding failed");
         free(json_payload);
         esp_camera_fb_return(fb);
         return NULL;
     }
     
     // Update json_len to include base64 data
     json_len += strlen(base64_output);
     
     // Add closing quote
     if (json_len + 1 >= json_size) {
         ESP_LOGE(TAG, "JSON buffer overflow after base64");
         free(json_payload);
         esp_camera_fb_return(fb);
         return NULL;
     }
     json_payload[json_len++] = '"';
     json_payload[json_len] = '\0';
     
     // Complete the JSON structure
     esp_cam_json_end_object(&json_state, json_payload, &json_len, json_size); // end inline_data
     esp_cam_json_end_object(&json_state, json_payload, &json_len, json_size); // end image part
     
     esp_cam_json_end_array(&json_state, json_payload, &json_len, json_size); // end parts array
     esp_cam_json_end_object(&json_state, json_payload, &json_len, json_size); // end content object
     
     esp_cam_json_end_array(&json_state, json_payload, &json_len, json_size); // end contents array
     
     // Add generation config
     esp_cam_json_key(&json_state, json_payload, &json_len, json_size, "generationConfig");
     esp_cam_json_start_object(&json_state, json_payload, &json_len, json_size);
     esp_cam_json_key_number(&json_state, json_payload, &json_len, json_size, "maxOutputTokens", "100");
     esp_cam_json_end_object(&json_state, json_payload, &json_len, json_size);
     
     esp_cam_json_end_object(&json_state, json_payload, &json_len, json_size); // end root object
     
     // We're done with the camera frame buffer
     esp_camera_fb_return(fb);
     
     ESP_LOGI(TAG, "JSON payload created, size: %u bytes", json_len);
     return json_payload;
 }
 
 char* ESP_CAM_SendGeminiRequest(const char* gemini_url, const char* api_key, const char* payload) {
     if (!gemini_url || !api_key || !payload) {
         ESP_LOGE(TAG, "Invalid parameters");
         return NULL;
     }
     
     // Extract model from URL
     char* model = extract_model_from_url(gemini_url);
     if (!model) {
         ESP_LOGE(TAG, "Failed to extract model from URL");
         return NULL;
     }
     
     // Initialize Gemini client
     gemini_client_config_t config = {
         .api_key = api_key,
         .model = model,
         .api_url = "https://generativelanguage.googleapis.com/v1beta",
         .timeout_ms = 30000 // 30 second timeout
     };
     
     gemini_client_handle_t client = gemini_client_init(&config);
     free(model); // Done with model name
     
     if (!client) {
         ESP_LOGE(TAG, "Failed to initialize Gemini client");
         return NULL;
     }
     
     // Start the request
     ESP_LOGI(TAG, "Sending request to Gemini API...");
     esp_err_t err = gemini_client_start_request(client, "application/json", strlen(payload));
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "Failed to start request: %s", esp_err_to_name(err));
         gemini_client_cleanup(client);
         return NULL;
     }
     
     // Send the payload
     err = gemini_client_write(client, payload, strlen(payload));
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "Failed to write payload: %s", esp_err_to_name(err));
         gemini_client_cleanup(client);
         return NULL;
     }
     
     // Get the response
     char *response = NULL;
     size_t response_len = 0;
     err = gemini_client_finish_request(client, &response, &response_len);
     gemini_client_cleanup(client);
     
     if (err != ESP_OK || !response) {
         ESP_LOGE(TAG, "Failed to get response: %s", esp_err_to_name(err));
         return NULL;
     }
     
     ESP_LOGI(TAG, "Received response, size: %u bytes", response_len);
     return response;
 }
 
 char* ESP_CAM_ExtractGeminiResponse(const char* json_response) {
     if (!json_response) {
         ESP_LOGE(TAG, "Invalid JSON response");
         return NULL;
     }
     
     return gemini_client_extract_text(json_response, strlen(json_response));
 }
 
 char* ESP_CAM_CaptureAndAnalyze(const char* gemini_url, const char* api_key, 
                                const char* model, const char* prompt) {
     // Capture and generate payload
     char* payload = ESP_CAM_CaptureAndGetGeminiPayload(gemini_url, api_key, prompt);
     if (!payload) {
         return NULL;
     }
     
     // Send request
     char* response = ESP_CAM_SendGeminiRequest(gemini_url, api_key, payload);
     ESP_CAM_FreePayload(payload); // Free the payload as we don't need it anymore
     
     if (!response) {
         return NULL;
     }
     
     // Extract text from response
     char* text = ESP_CAM_ExtractGeminiResponse(response);
     ESP_CAM_FreePayload(response); // Free the response as we don't need it anymore
     
     return text;
 }