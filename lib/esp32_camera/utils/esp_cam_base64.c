/**
 * @file esp_cam_base64.c
 * @brief Memory-efficient Base64 encoder optimized for ESP32-CAM
 */

 #include "esp_cam_base64.h"
 #include <string.h>
 #include "esp_log.h"
 
 static const char *TAG = "esp_cam_base64";
 
 static const char base64_chars[] = 
     "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 
 size_t esp_cam_base64_encode_len(size_t input_len) {
     // Calculate encoded length: 4 bytes for every 3 bytes of input, rounded up
     // Add 1 for null termination
     return (((input_len + 2) / 3) * 4) + 1;
 }
 
 esp_err_t esp_cam_base64_encode(const uint8_t *input, size_t input_len, char *output) {
     if (!input || !output) {
         return ESP_ERR_INVALID_ARG;
     }
 
     uint8_t state = 0;
     size_t output_len;
     return esp_cam_base64_encode_stream(input, input_len, output, &state, &output_len);
 }
 
 esp_err_t esp_cam_base64_encode_stream(const uint8_t *input, size_t input_len, 
                                      char *output, uint8_t *state, 
                                      size_t *output_len) {
     if (!input || !output || !state || !output_len) {
         return ESP_ERR_INVALID_ARG;
     }
 
     size_t i = 0;
     size_t o = 0;
     uint8_t leftover = *state & 0x03;  // Extract leftover data count (0-2)
     uint32_t bits = (*state >> 2) & 0x3FFFF;  // Extract saved bits
 
     // Process leftover bytes from previous call first
     while (leftover > 0 && i < input_len) {
         bits = (bits << 8) | input[i++];
         leftover--;
         
         if (leftover == 0) {
             // We have 3 bytes (24 bits), encode to 4 base64 chars
             output[o++] = base64_chars[(bits >> 18) & 0x3F];
             output[o++] = base64_chars[(bits >> 12) & 0x3F];
             output[o++] = base64_chars[(bits >> 6) & 0x3F];
             output[o++] = base64_chars[bits & 0x3F];
             bits = 0;
         }
     }
 
     // Main encoding loop - process input in groups of 3 bytes
     while (i + 3 <= input_len) {
         uint32_t val = ((uint32_t)input[i] << 16) | 
                        ((uint32_t)input[i+1] << 8) | 
                        input[i+2];
         
         output[o++] = base64_chars[(val >> 18) & 0x3F];
         output[o++] = base64_chars[(val >> 12) & 0x3F];
         output[o++] = base64_chars[(val >> 6) & 0x3F];
         output[o++] = base64_chars[val & 0x3F];
         
         i += 3;
     }
 
     // Handle remaining 1 or 2 bytes
     leftover = input_len - i;  // 0, 1, or 2
     
     if (leftover > 0) {
         bits = 0;
         
         // Collect remaining bytes
         while (i < input_len) {
             bits = (bits << 8) | input[i++];
         }
         
         // Shift to correct position based on number of bytes
         bits <<= 8 * (3 - leftover);
         
         // Output first character(s) of the group
         output[o++] = base64_chars[(bits >> 18) & 0x3F];
         
         if (leftover >= 1) {
             output[o++] = base64_chars[(bits >> 12) & 0x3F];
         }
         
         if (leftover == 2) {
             output[o++] = base64_chars[(bits >> 6) & 0x3F];
         }
     }
 
     // Save state for next call if we're in the middle of processing
     if (leftover > 0) {
         // Store leftover count and bits
         *state = (leftover & 0x03) | ((bits & 0x3FFFF) << 2);
     } else {
         *state = 0;
     }
 
     // Null-terminate if this is the end
     if (leftover == 0) {
         output[o] = '\0';
     }
     
     *output_len = o;
     return ESP_OK;
 }