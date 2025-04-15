/**
 * @file esp_cam_json.c
 * @brief Lightweight JSON builder for ESP32-CAM Gemini integration
 */

 #include "esp_cam_json.h"
 #include <string.h>
 #include "esp_log.h"
 
 static const char *TAG = "esp_cam_json";
 
 // Helper to check if we have enough space and update the buffer
 static esp_err_t write_to_buffer(char *buf, size_t *len, size_t max_len, const char *str) {
     size_t str_len = strlen(str);
     
     if (*len + str_len >= max_len) {
         ESP_LOGE(TAG, "JSON buffer overflow: %d + %d >= %d", (int)*len, (int)str_len, (int)max_len);
         return ESP_ERR_NO_MEM;
     }
     
     strcpy(buf + *len, str);
     *len += str_len;
     
     return ESP_OK;
 }
 
 // Initialize JSON builder state
 void esp_cam_json_init(esp_cam_json_state_t *state) {
     state->depth = 0;
     state->in_object = false;
     state->in_array = false;
     state->needs_separator = false;
 }
 
 // Start a JSON object
 esp_err_t esp_cam_json_start_object(esp_cam_json_state_t *state, char *buf, 
                                     size_t *len, size_t max_len) {
     esp_err_t ret;
     
     // If we're inside a structure and not at the beginning, add a separator first
     if (state->needs_separator) {
         ret = write_to_buffer(buf, len, max_len, ",");
         if (ret != ESP_OK) return ret;
     }
     
     ret = write_to_buffer(buf, len, max_len, "{");
     if (ret != ESP_OK) return ret;
     
     state->depth++;
     state->in_object = true;
     state->in_array = false;
     state->needs_separator = false;
     
     return ESP_OK;
 }
 
 // End a JSON object
 esp_err_t esp_cam_json_end_object(esp_cam_json_state_t *state, char *buf, 
                                   size_t *len, size_t max_len) {
     esp_err_t ret;
     
     if (state->depth == 0 || !state->in_object) {
         ESP_LOGE(TAG, "JSON syntax error: trying to end an object when not in one");
         return ESP_ERR_INVALID_STATE;
     }
     
     ret = write_to_buffer(buf, len, max_len, "}");
     if (ret != ESP_OK) return ret;
     
     state->depth--;
     
     // Update state based on parent container
     if (state->depth > 0) {
         state->needs_separator = true;
     } else {
         state->in_object = false;
         state->needs_separator = false;
     }
     
     return ESP_OK;
 }
 
 // Start a JSON array
 esp_err_t esp_cam_json_start_array(esp_cam_json_state_t *state, char *buf, 
                                    size_t *len, size_t max_len) {
     esp_err_t ret;
     
     // If we're inside a structure and not at the beginning, add a separator first
     if (state->needs_separator) {
         ret = write_to_buffer(buf, len, max_len, ",");
         if (ret != ESP_OK) return ret;
     }
     
     ret = write_to_buffer(buf, len, max_len, "[");
     if (ret != ESP_OK) return ret;
     
     state->depth++;
     state->in_array = true;
     state->in_object = false;
     state->needs_separator = false;
     
     return ESP_OK;
 }
 
 // End a JSON array
 esp_err_t esp_cam_json_end_array(esp_cam_json_state_t *state, char *buf, 
                                  size_t *len, size_t max_len) {
     esp_err_t ret;
     
     if (state->depth == 0 || !state->in_array) {
         ESP_LOGE(TAG, "JSON syntax error: trying to end an array when not in one");
         return ESP_ERR_INVALID_STATE;
     }
     
     ret = write_to_buffer(buf, len, max_len, "]");
     if (ret != ESP_OK) return ret;
     
     state->depth--;
     
     // Update state based on parent container
     if (state->depth > 0) {
         state->needs_separator = true;
     } else {
         state->in_array = false;
         state->needs_separator = false;
     }
     
     return ESP_OK;
 }
 
 // Add a key to a JSON object
 esp_err_t esp_cam_json_key(esp_cam_json_state_t *state, char *buf, 
                            size_t *len, size_t max_len, const char *key) {
     esp_err_t ret;
     
     if (!state->in_object) {
         ESP_LOGE(TAG, "JSON syntax error: key outside of object");
         return ESP_ERR_INVALID_STATE;
     }
     
     // If we're inside an object and not at the beginning, add a separator first
     if (state->needs_separator) {
         ret = write_to_buffer(buf, len, max_len, ",");
         if (ret != ESP_OK) return ret;
     }
     
     // Write the key with quotes and colon
     ret = write_to_buffer(buf, len, max_len, "\"");
     if (ret != ESP_OK) return ret;
     
     ret = write_to_buffer(buf, len, max_len, key);
     if (ret != ESP_OK) return ret;
     
     ret = write_to_buffer(buf, len, max_len, "\":");
     if (ret != ESP_OK) return ret;
     
     state->needs_separator = false;
     
     return ESP_OK;
 }
 
 // Add a string value to a JSON object or array
 esp_err_t esp_cam_json_string(esp_cam_json_state_t *state, char *buf, 
                               size_t *len, size_t max_len, const char *value) {
     esp_err_t ret;
     
     // If we're inside a structure and not at the beginning, add a separator first
     if (state->needs_separator) {
         ret = write_to_buffer(buf, len, max_len, ",");
         if (ret != ESP_OK) return ret;
     }
     
     // Write the string value with quotes
     ret = write_to_buffer(buf, len, max_len, "\"");
     if (ret != ESP_OK) return ret;
     
     // Handle special characters in the string (escape)
     for (size_t i = 0; i < strlen(value); i++) {
         char c = value[i];
         
         if (c == '\"' || c == '\\') {
             // Add escape character
             if (*len + 2 >= max_len) {
                 ESP_LOGE(TAG, "JSON buffer overflow during string escaping");
                 return ESP_ERR_NO_MEM;
             }
             buf[*len] = '\\';
             buf[*len + 1] = c;
             *len += 2;
             buf[*len] = '\0';
         } else if (c < 32) {
             // Control characters need special handling
             char escape[7];
             switch (c) {
                 case '\b': strcpy(escape, "\\b"); break;
                 case '\f': strcpy(escape, "\\f"); break;
                 case '\n': strcpy(escape, "\\n"); break;
                 case '\r': strcpy(escape, "\\r"); break;
                 case '\t': strcpy(escape, "\\t"); break;
                 default:
                     // Use \uXXXX format for other control chars
                     sprintf(escape, "\\u%04x", c);
             }
             ret = write_to_buffer(buf, len, max_len, escape);
             if (ret != ESP_OK) return ret;
         } else {
             // Normal character
             if (*len + 1 >= max_len) {
                 ESP_LOGE(TAG, "JSON buffer overflow during string copy");
                 return ESP_ERR_NO_MEM;
             }
             buf[*len] = c;
             *len += 1;
             buf[*len] = '\0';
         }
     }
     
     ret = write_to_buffer(buf, len, max_len, "\"");
     if (ret != ESP_OK) return ret;
     
     state->needs_separator = true;
     
     return ESP_OK;
 }
 
 // Add a number value to a JSON object or array
 esp_err_t esp_cam_json_number(esp_cam_json_state_t *state, char *buf, 
                               size_t *len, size_t max_len, const char *value) {
     esp_err_t ret;
     
     // If we're inside a structure and not at the beginning, add a separator first
     if (state->needs_separator) {
         ret = write_to_buffer(buf, len, max_len, ",");
         if (ret != ESP_OK) return ret;
     }
     
     // Write the number value as is (assumed to be a valid JSON number)
     ret = write_to_buffer(buf, len, max_len, value);
     if (ret != ESP_OK) return ret;
     
     state->needs_separator = true;
     
     return ESP_OK;
 }
 
 // Add a boolean value to a JSON object or array
 esp_err_t esp_cam_json_bool(esp_cam_json_state_t *state, char *buf, 
                             size_t *len, size_t max_len, bool value) {
     esp_err_t ret;
     
     // If we're inside a structure and not at the beginning, add a separator first
     if (state->needs_separator) {
         ret = write_to_buffer(buf, len, max_len, ",");
         if (ret != ESP_OK) return ret;
     }
     
     // Write the boolean value as true or false
     ret = write_to_buffer(buf, len, max_len, value ? "true" : "false");
     if (ret != ESP_OK) return ret;
     
     state->needs_separator = true;
     
     return ESP_OK;
 }
 
 // Add a null value to a JSON object or array
 esp_err_t esp_cam_json_null(esp_cam_json_state_t *state, char *buf, 
                             size_t *len, size_t max_len) {
     esp_err_t ret;
     
     // If we're inside a structure and not at the beginning, add a separator first
     if (state->needs_separator) {
         ret = write_to_buffer(buf, len, max_len, ",");
         if (ret != ESP_OK) return ret;
     }
     
     // Write null value
     ret = write_to_buffer(buf, len, max_len, "null");
     if (ret != ESP_OK) return ret;
     
     state->needs_separator = true;
     
     return ESP_OK;
 }
 
 // Add a key-string pair to a JSON object
 esp_err_t esp_cam_json_key_string(esp_cam_json_state_t *state, char *buf, 
                                   size_t *len, size_t max_len, 
                                   const char *key, const char *value) {
     esp_err_t ret;
     
     ret = esp_cam_json_key(state, buf, len, max_len, key);
     if (ret != ESP_OK) return ret;
     
     ret = esp_cam_json_string(state, buf, len, max_len, value);
     return ret;
 }
 
 // Add a key-number pair to a JSON object
 esp_err_t esp_cam_json_key_number(esp_cam_json_state_t *state, char *buf, 
                                   size_t *len, size_t max_len, 
                                   const char *key, const char *value) {
     esp_err_t ret;
     
     ret = esp_cam_json_key(state, buf, len, max_len, key);
     if (ret != ESP_OK) return ret;
     
     ret = esp_cam_json_number(state, buf, len, max_len, value);
     return ret;
 }
 
 // Add a key-boolean pair to a JSON object
 esp_err_t esp_cam_json_key_bool(esp_cam_json_state_t *state, char *buf, 
                                 size_t *len, size_t max_len, 
                                 const char *key, bool value) {
     esp_err_t ret;
     
     ret = esp_cam_json_key(state, buf, len, max_len, key);
     if (ret != ESP_OK) return ret;
     
     ret = esp_cam_json_bool(state, buf, len, max_len, value);
     return ret;
 }
 
 // Add a key-null pair to a JSON object
 esp_err_t esp_cam_json_key_null(esp_cam_json_state_t *state, char *buf, 
                                 size_t *len, size_t max_len, const char *key) {
     esp_err_t ret;
     
     ret = esp_cam_json_key(state, buf, len, max_len, key);
     if (ret != ESP_OK) return ret;
     
     ret = esp_cam_json_null(state, buf, len, max_len);
     return ret;
 }