/**
 * jpeg_encoder.h - Lightweight JPEG to Base64 encoder for ESP32-CAM
 * 
 * This file provides functions for capturing JPEG images and
 * converting them to Base64 encoded strings with minimal memory usage.
 * 
 * Copyright (c) 2025 
 * Licensed under MIT License
 */

 #ifndef JPEG_ENCODER_H
 #define JPEG_ENCODER_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stdlib.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * State structure for chunked Base64 encoding
  */
 typedef struct {
     uint32_t buffer;     // Bit buffer for carrying bits between chunks
     uint8_t buffer_size; // Number of valid bits in buffer
     size_t total_bytes;  // Total bytes processed (for padding)
 } base64_state_t;
 
 /**
  * Calculate required size for Base64 encoding of input data
  */
 size_t calculate_base64_length(size_t input_length);
 
 /**
  * Encode a binary buffer to Base64 string
  * 
  * @param input Input binary data
  * @param input_length Length of input data
  * @param output Pre-allocated output buffer for Base64 string
  * @param output_size Size of the output buffer
  * @return Size of the Base64 string (excluding null terminator) or 0 if error
  */
 size_t encode_base64(const uint8_t* input, size_t input_length, 
                     char* output, size_t output_size);
 
 /**
  * Capture JPEG image and return Base64 encoded data
  * 
  * @param use_flash Whether to use flash for capture
  * @param output_buffer Pre-allocated output buffer for Base64 string
  * @param buffer_size Size of the output buffer
  * @param jpeg_size Optional parameter to receive original JPEG size
  * @return Length of the Base64 string (excluding null terminator) or 0 if error
  */
 size_t capture_jpeg_as_base64(bool use_flash, char* output_buffer, 
                              size_t buffer_size, size_t* jpeg_size);
 
 /**
  * Capture JPEG image and allocate a new buffer for Base64 encoded data
  * CAUTION: Caller must free the returned buffer with free() when done
  * 
  * @param use_flash Whether to use flash for capture
  * @param encoded_size Optional parameter to receive the encoded data size
  * @param jpeg_size Optional parameter to receive original JPEG size
  * @return Pointer to newly allocated Base64 string or NULL if error
  */
 char* capture_jpeg_as_base64_alloc(bool use_flash, size_t* encoded_size, size_t* jpeg_size);
 
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
                           base64_state_t* state);
 
 /**
  * Finalize Base64 stream encoding and add padding if needed
  * 
  * @param output Pre-allocated output buffer for Base64 string
  * @param output_size Size of the output buffer
  * @param state Pointer to a base64_state structure to track encoding state
  * @return Number of bytes written to output buffer or 0 if error
  */
 size_t encode_base64_finalize(char* output, size_t output_size, base64_state_t* state);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* JPEG_ENCODER_H */