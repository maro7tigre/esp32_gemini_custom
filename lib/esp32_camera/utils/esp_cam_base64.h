/**
 * @file esp_cam_base64.h
 * @brief Memory-efficient Base64 encoder optimized for ESP32-CAM
 */

 #pragma once

 #include <stddef.h>
 #include <stdint.h>
 #include "esp_err.h"
 
 /**
  * @brief Calculate the base64 encoded length for a given input length
  * 
  * @param input_len Length of the input data in bytes
  * @return Size of the encoded data in bytes (including null terminator)
  */
 size_t esp_cam_base64_encode_len(size_t input_len);
 
 /**
  * @brief Encode binary data to base64
  * 
  * @param input Input binary data
  * @param input_len Length of input data in bytes
  * @param output Output buffer for base64 data (must be large enough)
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_base64_encode(const uint8_t *input, size_t input_len, char *output);
 
 /**
  * @brief Encode binary data to base64 with streaming support
  * 
  * @param input Input binary data
  * @param input_len Length of input data in bytes
  * @param output Output buffer for base64 data
  * @param state State to maintain between calls (must be zero on first call)
  * @param output_len Pointer to store the length of output written
  * @return ESP_OK on success
  */
 esp_err_t esp_cam_base64_encode_stream(const uint8_t *input, size_t input_len, 
                                      char *output, uint8_t *state, 
                                      size_t *output_len);