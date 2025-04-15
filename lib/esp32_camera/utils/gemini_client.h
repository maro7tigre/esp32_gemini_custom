/**
 * @file gemini_client.h
 * @brief HTTP client specifically for Gemini API integration
 */

 #pragma once

 #include <stddef.h>
 #include <stdbool.h>
 #include "esp_err.h"
 #include "esp_http_client.h"
 
 /**
  * @brief Configuration for Gemini client
  */
 typedef struct {
     const char *api_key;     // Gemini API key
     const char *model;       // Gemini model name (e.g., "gemini-pro-vision")
     const char *api_url;     // Base URL for Gemini API
     int timeout_ms;          // Request timeout in milliseconds
 } gemini_client_config_t;
 
 /**
  * @brief Gemini client handle
  */
 typedef struct gemini_client *gemini_client_handle_t;
 
 /**
  * @brief Create a new Gemini client
  * 
  * @param config Client configuration
  * @return gemini_client_handle_t Client handle or NULL on error
  */
 gemini_client_handle_t gemini_client_init(const gemini_client_config_t *config);
 
 /**
  * @brief Free a Gemini client
  * 
  * @param client Client handle
  */
 void gemini_client_cleanup(gemini_client_handle_t client);
 
 /**
  * @brief Start a new request to the Gemini API
  * 
  * @param client Client handle
  * @param content_type Content type for the request
  * @param content_length Estimated content length (can be 0 for chunked)
  * @return ESP_OK on success
  */
 esp_err_t gemini_client_start_request(gemini_client_handle_t client, 
                                       const char *content_type,
                                       int content_length);
 
 /**
  * @brief Write a chunk of data to the request body
  * 
  * @param client Client handle
  * @param data Data buffer
  * @param len Data length
  * @return ESP_OK on success
  */
 esp_err_t gemini_client_write(gemini_client_handle_t client, 
                               const char *data, size_t len);
 
 /**
  * @brief Finish the request and get the response
  * 
  * @param client Client handle
  * @param response Pointer to store response buffer (must be freed by caller)
  * @param response_len Pointer to store response length
  * @return ESP_OK on success
  */
 esp_err_t gemini_client_finish_request(gemini_client_handle_t client, 
                                       char **response, size_t *response_len);
 
 /**
  * @brief Extract text from Gemini API response
  * 
  * This function parses the JSON response and extracts the generated text.
  * The returned string must be freed by the caller.
  * 
  * @param response Response JSON string
  * @param response_len Response length
  * @return char* Extracted text or NULL on error
  */
 char* gemini_client_extract_text(const char *response, size_t response_len);
 
 /**
  * @brief Free memory allocated by gemini_client functions
  * 
  * @param ptr Pointer to free
  */
 void gemini_client_free(void *ptr);