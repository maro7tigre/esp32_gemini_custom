/**
 * jpeg_encoder.c - Lightweight JPEG to Base64 encoder for ESP32-CAM
 * 
 * This file provides functions for capturing JPEG images and
 * converting them to Base64 encoded strings with minimal memory usage.
 * 
 * Copyright (c) 2025 
 * Licensed under MIT License
 */

 #include "encoder.h"
 #include "custom_cam.h"
 #include "esp_log.h"
 #include <string.h>
 #include <stdlib.h>
 
 static const char* TAG = "jpeg_encoder";
 
 // Base64 encoding table
 static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 
 /**
  * Calculate required size for Base64 encoding of input data
  */
 size_t calculate_base64_length(size_t input_length) {
     // Base64 encoding: 4 output chars for every 3 input bytes
     size_t output_length = (input_length + 2) / 3 * 4;
     
     // Add space for null terminator
     return output_length + 1;
 }
 
 /**
  * Encode a binary buffer to Base64 string
  * 
  * @param input Input binary data
  * @param input_length Length of input data
  * @param output Pre-allocated output buffer for Base64 string
  * @param output_size Size of the output buffer
  * @return Size of the Base64 string (excluding null terminator) or 0 if error
  */
 size_t encode_base64(const uint8_t* input, size_t input_length, char* output, size_t output_size) {
     if (!input || !output || input_length == 0) {
         return 0;
     }
     
     size_t required_size = calculate_base64_length(input_length);
     if (output_size < required_size) {
         ESP_LOGE(TAG, "Output buffer too small for Base64 encoding");
         return 0;
     }
     
     size_t i, j;
     uint32_t a, b, c;
     
     for (i = 0, j = 0; i < input_length; i += 3, j += 4) {
         // Get next three bytes (24 bits)
         a = i < input_length ? input[i] : 0;
         b = (i + 1) < input_length ? input[i + 1] : 0;
         c = (i + 2) < input_length ? input[i + 2] : 0;
         
         // Pack into 24-bit integer
         uint32_t triple = (a << 16) | (b << 8) | c;
         
         // Encode to 4 Base64 characters (6 bits each)
         output[j] = base64_table[(triple >> 18) & 0x3F];
         output[j + 1] = base64_table[(triple >> 12) & 0x3F];
         output[j + 2] = (i + 1) < input_length ? base64_table[(triple >> 6) & 0x3F] : '=';
         output[j + 3] = (i + 2) < input_length ? base64_table[triple & 0x3F] : '=';
     }
     
     // Null-terminate the string
     output[j] = '\0';
     
     return j;
 }
 
 /**
  * Capture JPEG image and return Base64 encoded data
  * 
  * @param use_flash Whether to use flash for capture
  * @param output_buffer Pre-allocated output buffer for Base64 string
  * @param buffer_size Size of the output buffer
  * @param jpeg_size Optional parameter to receive original JPEG size
  * @return Length of the Base64 string (excluding null terminator) or 0 if error
  */
 size_t capture_jpeg_as_base64(bool use_flash, char* output_buffer, size_t buffer_size, size_t* jpeg_size) {
     if (!output_buffer || buffer_size == 0) {
         ESP_LOGE(TAG, "Invalid output buffer");
         return 0;
     }
     
     // Take picture with flash if requested
     camera_fb_t* fb = custom_cam_take_picture(use_flash, 50);
     if (!fb) {
         ESP_LOGE(TAG, "Camera capture failed");
         return 0;
     }
     
     // Store original JPEG size if requested
     if (jpeg_size) {
         *jpeg_size = fb->len;
     }
     
     ESP_LOGI(TAG, "Image captured: %dx%d, size: %u bytes", fb->width, fb->height, fb->len);
     
     // Check if output buffer is large enough
     size_t required_size = calculate_base64_length(fb->len);
     if (buffer_size < required_size) {
         ESP_LOGE(TAG, "Output buffer too small (%u bytes), need %u bytes", 
                  buffer_size, required_size);
         custom_cam_return_fb(fb);
         return 0;
     }
     
     // Encode to Base64
     size_t encoded_len = encode_base64(fb->buf, fb->len, output_buffer, buffer_size);
     
     // Free the camera frame buffer
     custom_cam_return_fb(fb);
     
     ESP_LOGI(TAG, "JPEG encoded to Base64: %u bytes", encoded_len);
     return encoded_len;
 }
 
 /**
  * Capture JPEG image and allocate a new buffer for Base64 encoded data
  * CAUTION: Caller must free the returned buffer with free() when done
  * 
  * @param use_flash Whether to use flash for capture
  * @param encoded_size Optional parameter to receive the encoded data size
  * @param jpeg_size Optional parameter to receive original JPEG size
  * @return Pointer to newly allocated Base64 string or NULL if error
  */
 char* capture_jpeg_as_base64_alloc(bool use_flash, size_t* encoded_size, size_t* jpeg_size) {
     // Take picture with flash if requested
     camera_fb_t* fb = custom_cam_take_picture(use_flash, 50);
     if (!fb) {
         ESP_LOGE(TAG, "Camera capture failed");
         return NULL;
     }
     
     // Store original JPEG size if requested
     if (jpeg_size) {
         *jpeg_size = fb->len;
     }
     
     ESP_LOGI(TAG, "Image captured: %dx%d, size: %u bytes", fb->width, fb->height, fb->len);
     
     // Calculate required buffer size and allocate
     size_t required_size = calculate_base64_length(fb->len);
     char* output_buffer = (char*)malloc(required_size);
     if (!output_buffer) {
         ESP_LOGE(TAG, "Failed to allocate memory for Base64 encoding");
         custom_cam_return_fb(fb);
         return NULL;
     }
     
     // Encode to Base64
     size_t encoded_len = encode_base64(fb->buf, fb->len, output_buffer, required_size);
     
     // Free the camera frame buffer
     custom_cam_return_fb(fb);
     
     // Store encoded size if requested
     if (encoded_size) {
         *encoded_size = encoded_len;
     }
     
     ESP_LOGI(TAG, "JPEG encoded to Base64: %u bytes", encoded_len);
     return output_buffer;
 }
 
 /**
  * Stream-friendly version of Base64 encoding that processes data in chunks
  * to minimize memory usage.
  * 
  * @param input Input binary data chunk
  * @param input_length Length of input data chunk
  * @param output Pre-allocated output buffer for Base64 string
  * @param output_size Size of the output buffer
  * @param state Pointer to a base64_state structure to track encoding state
  * @return Number of bytes written to output buffer or 0 if error
  */
 size_t encode_base64_chunk(const uint8_t* input, size_t input_length,
                           char* output, size_t output_size,
                           base64_state_t* state) {
     if (!input || !output || !state || input_length == 0) {
         return 0;
     }
     
     size_t i = 0;  // Input index
     size_t j = 0;  // Output index
     uint32_t buffer = state->buffer;
     uint8_t buffer_size = state->buffer_size;
     
     // Process each byte from input
     while (i < input_length) {
         // Add next byte to buffer
         buffer = (buffer << 8) | input[i++];
         buffer_size += 8;
         
         // Process complete 6-bit groups
         while (buffer_size >= 6 && j < output_size) {
             uint8_t index = (buffer >> (buffer_size - 6)) & 0x3F;
             output[j++] = base64_table[index];
             buffer_size -= 6;
         }
     }
     
     // Update state
     state->buffer = buffer;
     state->buffer_size = buffer_size;
     
     return j;
 }
 
 /**
  * Finalize Base64 stream encoding and add padding if needed
  * 
  * @param output Pre-allocated output buffer for Base64 string
  * @param output_size Size of the output buffer
  * @param state Pointer to a base64_state structure to track encoding state
  * @return Number of bytes written to output buffer or 0 if error
  */
 size_t encode_base64_finalize(char* output, size_t output_size, base64_state_t* state) {
     if (!output || !state) {
         return 0;
     }
     
     size_t j = 0;
     uint32_t buffer = state->buffer;
     uint8_t buffer_size = state->buffer_size;
     
     // Process any remaining bits
     if (buffer_size > 0) {
         if (j < output_size) {
             // Shift bits to align with 6-bit boundary
             buffer <<= (6 - buffer_size);
             uint8_t index = buffer & 0x3F;
             output[j++] = base64_table[index];
         }
     }
     
     // Add padding '=' characters
     int padding = (3 - ((state->total_bytes) % 3)) % 3;
     for (int i = 0; i < padding && j < output_size; i++) {
         output[j++] = '=';
     }
     
     // Null-terminate if there's space
     if (j < output_size) {
         output[j] = '\0';
     }
     
     // Reset state
     state->buffer = 0;
     state->buffer_size = 0;
     
     return j;
 }