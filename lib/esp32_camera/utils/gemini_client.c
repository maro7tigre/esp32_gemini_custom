/**
 * @file gemini_client.c
 * @brief HTTP client specifically for Gemini API integration
 */

 #include "gemini_client.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "esp_log.h"
 #include "esp_http_client.h"
 #include "esp_tls.h"
 
 static const char *TAG = "gemini_client";
 
 // Gemini client structure
 struct gemini_client {
     esp_http_client_handle_t http_client;
     char *api_key;
     char *model;
     char *api_url;
     int timeout_ms;
     bool request_started;
     esp_err_t last_error;
 }
 
 void gemini_client_free(void *ptr) {
     if (ptr) {
         free(ptr);
     }
 };
 
 /**
  * Implementation of Extract text from Gemini API response
  * 
  * This function looks for the text field in a JSON response like:
  * {
  *   "candidates": [
  *     {
  *       "content": {
  *         "parts": [
  *           {
  *             "text": "This is the response text"
  *           }
  *         ]
  *       }
  *     }
  *   ]
  * }
  * 
  * This is a simple parser that assumes a valid JSON structure.
  */
 char* gemini_client_extract_text(const char *response, size_t response_len) {
     if (!response || response_len == 0) {
         ESP_LOGE(TAG, "Empty response");
         return NULL;
     }
     
     // Look for "text": " pattern
     const char *text_marker = "\"text\": \"";
     char *text_start = strstr(response, text_marker);
     
     if (!text_start) {
         ESP_LOGE(TAG, "Text field not found in response");
         return NULL;
     }
     
     // Move past the marker
     text_start += strlen(text_marker);
     
     // Find the closing quote, respecting escaping
     char *text_end = text_start;
     bool in_escape = false;
     
     while (*text_end) {
         if (in_escape) {
             in_escape = false;
         } else if (*text_end == '\\') {
             in_escape = true;
         } else if (*text_end == '"') {
             break;
         }
         text_end++;
     }
     
     if (*text_end != '"') {
         ESP_LOGE(TAG, "End of text field not found");
         return NULL;
     }
     
     // Calculate length and allocate buffer
     size_t text_len = text_end - text_start;
     char *text = (char*)malloc(text_len + 1);
     
     if (!text) {
         ESP_LOGE(TAG, "Failed to allocate memory for text");
         return NULL;
     }
     
     // Copy and unescape the text
     size_t j = 0;
     in_escape = false;
     
     for (size_t i = 0; i < text_len; i++) {
         char c = text_start[i];
         
         if (in_escape) {
             // Handle escape sequences
             switch (c) {
                 case 'n': text[j++] = '\n'; break;
                 case 'r': text[j++] = '\r'; break;
                 case 't': text[j++] = '\t'; break;
                 case 'b': text[j++] = '\b'; break;
                 case 'f': text[j++] = '\f'; break;
                 default: text[j++] = c; break;
             }
             in_escape = false;
         } else if (c == '\\') {
             in_escape = true;
         } else {
             text[j++] = c;
         }
     }
     
     text[j] = '\0';
     return text;
 }
 
 /**
  * Check for error in the Gemini API response
  * 
  * This function looks for the error field in a JSON response like:
  * {
  *   "error": {
  *     "code": 400,
  *     "message": "Error message"
  *   }
  * }
  * 
  * Returns error message if found, NULL otherwise
  */
 static char* check_api_error(const char *response) {
     if (!response) {
         return NULL;
     }
     
     // Look for "error": { pattern
     const char *error_marker = "\"error\": {";
     char *error_start = strstr(response, error_marker);
     
     if (!error_start) {
         return NULL;  // No error found
     }
     
     // Look for "message": " pattern
     const char *message_marker = "\"message\": \"";
     char *message_start = strstr(error_start, message_marker);
     
     if (!message_start) {
         return strdup("Unknown API error");
     }
     
     // Move past the marker
     message_start += strlen(message_marker);
     
     // Find the closing quote
     char *message_end = message_start;
     bool in_escape = false;
     
     while (*message_end) {
         if (in_escape) {
             in_escape = false;
         } else if (*message_end == '\\') {
             in_escape = true;
         } else if (*message_end == '"') {
             break;
         }
         message_end++;
     }
     
     if (*message_end != '"') {
         return strdup("Error message parsing failed");
     }
     
     // Calculate length and allocate buffer
     size_t message_len = message_end - message_start;
     char *message = (char*)malloc(message_len + 1);
     
     if (!message) {
         return strdup("Memory allocation failed");
     }
     
     // Copy and unescape the message
     size_t j = 0;
     in_escape = false;
     
     for (size_t i = 0; i < message_len; i++) {
         char c = message_start[i];
         
         if (in_escape) {
             switch (c) {
                 case 'n': message[j++] = '\n'; break;
                 case 'r': message[j++] = '\r'; break;
                 case 't': message[j++] = '\t'; break;
                 case 'b': message[j++] = '\b'; break;
                 case 'f': message[j++] = '\f'; break;
                 default: message[j++] = c; break;
             }
             in_escape = false;
         } else if (c == '\\') {
             in_escape = true;
         } else {
             message[j++] = c;
         }
     }
     
     message[j] = '\0';
     return message;
 }
 
 gemini_client_handle_t gemini_client_init(const gemini_client_config_t *config) {
     if (!config || !config->api_key || !config->model || !config->api_url) {
         ESP_LOGE(TAG, "Invalid config");
         return NULL;
     }
     
     struct gemini_client *client = (struct gemini_client *)calloc(1, sizeof(struct gemini_client));
     if (!client) {
         ESP_LOGE(TAG, "Failed to allocate memory for client");
         return NULL;
     }
     
     // Copy configuration
     client->api_key = strdup(config->api_key);
     client->model = strdup(config->model);
     client->api_url = strdup(config->api_url);
     client->timeout_ms = config->timeout_ms > 0 ? config->timeout_ms : 10000;  // Default 10s
     
     if (!client->api_key || !client->model || !client->api_url) {
         ESP_LOGE(TAG, "Failed to allocate memory for config strings");
         gemini_client_cleanup(client);
         return NULL;
     }
     
     // URL format: {api_url}/models/{model}:generateContent?key={api_key}
     size_t url_len = strlen(client->api_url) + strlen(client->model) + 
                      strlen(client->api_key) + 50;  // Extra for format and null
     
     char *url = (char *)malloc(url_len);
     if (!url) {
         ESP_LOGE(TAG, "Failed to allocate memory for URL");
         gemini_client_cleanup(client);
         return NULL;
     }
     
     snprintf(url, url_len, "%s/models/%s:generateContent?key=%s", 
              client->api_url, client->model, client->api_key);
     
     ESP_LOGI(TAG, "Gemini API URL: %s", url);
     
     // Initialize HTTP client
     esp_http_client_config_t http_config = {
         .url = url,
         .method = HTTP_METHOD_POST,
         .timeout_ms = client->timeout_ms,
         .disable_auto_redirect = false,
         .max_redirection_count = 5,
         .skip_cert_common_name_check = true   // Often needed for HTTPS requests
     };
     
     client->http_client = esp_http_client_init(&http_config);
     free(url);  // URL is copied by esp_http_client_init
     
     if (!client->http_client) {
         ESP_LOGE(TAG, "Failed to initialize HTTP client");
         gemini_client_cleanup(client);
         return NULL;
     }
     
     client->request_started = false;
     client->last_error = ESP_OK;
     
     return client;
 }
 
 void gemini_client_cleanup(gemini_client_handle_t client) {
     if (!client) {
         return;
     }
     
     if (client->http_client) {
         esp_http_client_cleanup(client->http_client);
     }
     
     if (client->api_key) free(client->api_key);
     if (client->model) free(client->model);
     if (client->api_url) free(client->api_url);
     
     free(client);
 }
 
 esp_err_t gemini_client_start_request(gemini_client_handle_t client, 
                                       const char *content_type,
                                       int content_length) {
     if (!client || !client->http_client || !content_type) {
         return ESP_ERR_INVALID_ARG;
     }
     
     if (client->request_started) {
         ESP_LOGE(TAG, "Request already started");
         return ESP_ERR_INVALID_STATE;
     }
     
     // Set headers
     esp_http_client_set_header(client->http_client, "Content-Type", content_type);
     esp_http_client_set_header(client->http_client, "Accept", "application/json");
     
     // Set content length if specified
     if (content_length > 0) {
         char content_length_str[16];
         snprintf(content_length_str, sizeof(content_length_str), "%d", content_length);
         esp_http_client_set_header(client->http_client, "Content-Length", content_length_str);
     } else {
         // Use chunked transfer
         esp_http_client_set_header(client->http_client, "Transfer-Encoding", "chunked");
     }
     
     // Open connection
     esp_err_t err = esp_http_client_open(client->http_client, content_length);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
         client->last_error = err;
         return err;
     }
     
     client->request_started = true;
     return ESP_OK;
 }
 
 esp_err_t gemini_client_write(gemini_client_handle_t client, 
                               const char *data, size_t len) {
     if (!client || !client->http_client || !data) {
         return ESP_ERR_INVALID_ARG;
     }
     
     if (!client->request_started) {
         ESP_LOGE(TAG, "Request not started");
         return ESP_ERR_INVALID_STATE;
     }
     
     int written = esp_http_client_write(client->http_client, data, len);
     if (written < 0) {
         ESP_LOGE(TAG, "Failed to write request data");
         client->last_error = ESP_FAIL;
         return ESP_FAIL;
     }
     
     if ((size_t)written != len) {
         ESP_LOGW(TAG, "Partial write: %d of %d bytes", written, len);
     }
     
     return ESP_OK;
 }
 
 esp_err_t gemini_client_finish_request(gemini_client_handle_t client, 
                                       char **response, size_t *response_len) {
     if (!client || !client->http_client || !response || !response_len) {
         return ESP_ERR_INVALID_ARG;
     }
     
     if (!client->request_started) {
         ESP_LOGE(TAG, "Request not started");
         return ESP_ERR_INVALID_STATE;
     }
     
     // Finalize request
     esp_err_t err = esp_http_client_perform(client->http_client);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
         client->last_error = err;
         return err;
     }
     
     int status_code = esp_http_client_get_status_code(client->http_client);
     if (status_code != 200) {
         ESP_LOGE(TAG, "HTTP request returned status code %d", status_code);
         
         // Try to read response for error details
         int content_length = esp_http_client_get_content_length(client->http_client);
         if (content_length <= 0) {
             content_length = 1024;  // Default size if content length is unknown
         }
         
         char *error_buf = (char *)malloc(content_length + 1);
         if (!error_buf) {
             ESP_LOGE(TAG, "Failed to allocate memory for error response");
             return ESP_ERR_NO_MEM;
         }
         
         int read_len = esp_http_client_read_response(client->http_client, error_buf, content_length);
         if (read_len > 0) {
             error_buf[read_len] = '\0';
             char *error_message = check_api_error(error_buf);
             if (error_message) {
                 ESP_LOGE(TAG, "API error: %s", error_message);
                 free(error_message);
             }
         }
         
         free(error_buf);
         return ESP_FAIL;
     }
     
     // Get content length
     int content_length = esp_http_client_get_content_length(client->http_client);
     if (content_length <= 0) {
         // Content length unknown, read in chunks
         const int chunk_size = 1024;
         int total_len = 0;
         int chunk_count = 0;
         char *buffer = NULL;
         
         while (1) {
             chunk_count++;
             char *new_buffer = (char *)realloc(buffer, chunk_count * chunk_size);
             if (!new_buffer) {
                 free(buffer);
                 ESP_LOGE(TAG, "Failed to allocate memory for response");
                 return ESP_ERR_NO_MEM;
             }
             
             buffer = new_buffer;
             int read_len = esp_http_client_read(client->http_client, 
                                                buffer + total_len, 
                                                chunk_size - 1);
             if (read_len <= 0) {
                 break;
             }
             
             total_len += read_len;
         }
         
         if (total_len == 0) {
             free(buffer);
             ESP_LOGE(TAG, "Empty response");
             return ESP_FAIL;
         }
         
         buffer[total_len] = '\0';
         *response = buffer;
         *response_len = total_len;
     } else {
         // Content length known
         char *buffer = (char *)malloc(content_length + 1);
         if (!buffer) {
             ESP_LOGE(TAG, "Failed to allocate memory for response");
             return ESP_ERR_NO_MEM;
         }
         
         int read_len = esp_http_client_read_response(client->http_client, buffer, content_length);
         if (read_len <= 0) {
             free(buffer);
             ESP_LOGE(TAG, "Failed to read response");
             return ESP_FAIL;
         }
         
         buffer[read_len] = '\0';
         *response = buffer;
         *response_len = read_len;
     }
     
     // Check for API errors
     char *error_message = check_api_error(*response);
     if (error_message) {
         ESP_LOGE(TAG, "API error: %s", error_message);
         free(error_message);
         free(*response);
         *response = NULL;
         *response_len = 0;
         return ESP_FAIL;
     }
     
     // Reset state for next request
     client->request_started = false;
     return ESP_OK;