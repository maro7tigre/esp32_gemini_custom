/**
 * @file esp_cam_json.h
 * @brief Lightweight JSON builder for ESP32-CAM Gemini integration
 */

 #pragma once

 #include <stddef.h>
 #include <stdbool.h>
 #include "esp_err.h"
 
 /**
  * @brief State for JSON builder
  */
 typedef struct {
     unsigned int depth;     // Current nesting depth
     bool in_object;         // True if inside an object
     bool in_array;          // True if inside an array
     bool needs_separator;   // True if next item needs a separator
 } esp_cam_json_state_t;
 
 /**
  * @brief Initialize JSON builder state
  * 
  * @param state State structure to initialize
  */
 void esp_cam_json_init(esp_cam_json_state_t *state);
 
 /**
  * @brief Start a JSON object
  * 
  * @param state JSON builder state
  * @param buf Buffer to write to
  * @param len Pointer to length counter, will be updated
  * @param max_len Maximum length of buffer
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_json_start_object(esp_cam_json_state_t *state, char *buf, 
                                   size_t *len, size_t max_len);
 
 /**
  * @brief End a JSON object
  * 
  * @param state JSON builder state
  * @param buf Buffer to write to
  * @param len Pointer to length counter, will be updated
  * @param max_len Maximum length of buffer
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_json_end_object(esp_cam_json_state_t *state, char *buf, 
                                 size_t *len, size_t max_len);
 
 /**
  * @brief Start a JSON array
  * 
  * @param state JSON builder state
  * @param buf Buffer to write to
  * @param len Pointer to length counter, will be updated
  * @param max_len Maximum length of buffer
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_json_start_array(esp_cam_json_state_t *state, char *buf, 
                                  size_t *len, size_t max_len);
 
 /**
  * @brief End a JSON array
  * 
  * @param state JSON builder state
  * @param buf Buffer to write to
  * @param len Pointer to length counter, will be updated
  * @param max_len Maximum length of buffer
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_json_end_array(esp_cam_json_state_t *state, char *buf, 
                                size_t *len, size_t max_len);
 
 /**
  * @brief Add a key to a JSON object
  * 
  * @param state JSON builder state
  * @param buf Buffer to write to
  * @param len Pointer to length counter, will be updated
  * @param max_len Maximum length of buffer
  * @param key Key string
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_json_key(esp_cam_json_state_t *state, char *buf, 
                          size_t *len, size_t max_len, const char *key);
 
 /**
  * @brief Add a string value to a JSON object or array
  * 
  * @param state JSON builder state
  * @param buf Buffer to write to
  * @param len Pointer to length counter, will be updated
  * @param max_len Maximum length of buffer
  * @param value String value
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_json_string(esp_cam_json_state_t *state, char *buf, 
                             size_t *len, size_t max_len, const char *value);
 
 /**
  * @brief Add a number value to a JSON object or array
  * 
  * @param state JSON builder state
  * @param buf Buffer to write to
  * @param len Pointer to length counter, will be updated
  * @param max_len Maximum length of buffer
  * @param value Number value as string
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_json_number(esp_cam_json_state_t *state, char *buf, 
                             size_t *len, size_t max_len, const char *value);
 
 /**
  * @brief Add a boolean value to a JSON object or array
  * 
  * @param state JSON builder state
  * @param buf Buffer to write to
  * @param len Pointer to length counter, will be updated
  * @param max_len Maximum length of buffer
  * @param value Boolean value
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_json_bool(esp_cam_json_state_t *state, char *buf, 
                           size_t *len, size_t max_len, bool value);
 
 /**
  * @brief Add a null value to a JSON object or array
  * 
  * @param state JSON builder state
  * @param buf Buffer to write to
  * @param len Pointer to length counter, will be updated
  * @param max_len Maximum length of buffer
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_json_null(esp_cam_json_state_t *state, char *buf, 
                           size_t *len, size_t max_len);
 
 /**
  * @brief Add a key-string pair to a JSON object
  * 
  * @param state JSON builder state
  * @param buf Buffer to write to
  * @param len Pointer to length counter, will be updated
  * @param max_len Maximum length of buffer 
  * @param key Key string
  * @param value String value
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_json_key_string(esp_cam_json_state_t *state, char *buf, 
                                 size_t *len, size_t max_len, 
                                 const char *key, const char *value);
 
 /**
  * @brief Add a key-number pair to a JSON object
  * 
  * @param state JSON builder state
  * @param buf Buffer to write to
  * @param len Pointer to length counter, will be updated
  * @param max_len Maximum length of buffer
  * @param key Key string
  * @param value Number value as string
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_json_key_number(esp_cam_json_state_t *state, char *buf, 
                                 size_t *len, size_t max_len, 
                                 const char *key, const char *value);
 
 /**
  * @brief Add a key-boolean pair to a JSON object
  * 
  * @param state JSON builder state
  * @param buf Buffer to write to
  * @param len Pointer to length counter, will be updated
  * @param max_len Maximum length of buffer
  * @param key Key string
  * @param value Boolean value
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_json_key_bool(esp_cam_json_state_t *state, char *buf, 
                               size_t *len, size_t max_len, 
                               const char *key, bool value);
 
 /**
  * @brief Add a key-null pair to a JSON object
  * 
  * @param state JSON builder state
  * @param buf Buffer to write to
  * @param len Pointer to length counter, will be updated
  * @param max_len Maximum length of buffer
  * @param key Key string
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_json_key_null(esp_cam_json_state_t *state, char *buf, 
                               size_t *len, size_t max_len, const char *key);