# ESP32-CAM Gemini Integration Library

## Overview

This library provides a streamlined, memory-efficient integration between ESP32-CAM (with OV2640 sensor) and Google's Gemini API. It handles the entire pipeline from image capture to API communication with minimal memory usage, eliminating the need for SD card storage and minimizing buffer copies.

## Key Features

- **OV2640-Optimized**: Stripped down to support only the OV2640 camera for maximum efficiency
- **Memory-Efficient**: Processes image data in streams to minimize memory usage
- **Direct Gemini Integration**: Handles image capture, base64 encoding, JSON construction, and API communication
- **Simple API**: Clean, easy-to-use functions that handle complex operations
- **No External Dependencies**: Custom implementations of base64 encoding and JSON building
- **PlatformIO Compatible**: Easily integratable into PlatformIO projects

## File Structure

```
esp32-camera/
├── CMakeLists.txt
├── library.json
├── Kconfig
├── driver/              # Core camera driver files
├── include/
│   └── esp_cam_utils.h  # Main public API
├── sensors/
│   └── ov2640.c         # OV2640-specific code
└── utils/
    ├── esp_cam_base64.c # Optimized base64 encoder
    ├── esp_cam_json.c   # Minimal JSON builder
    └── gemini_client.c  # HTTP client for Gemini API
```

## Memory Optimization Strategy

1. **No Intermediate Storage**: Direct processing from camera buffer to API payload
2. **Streaming Base64 Encoding**: Process image data in chunks without full buffer copies
3. **Progressive JSON Building**: Construct JSON payload incrementally
4. **Early Resource Release**: Free buffers as soon as they're no longer needed
5. **No SD Card Usage**: Everything is processed in memory

## Usage

### Basic Usage Pattern

```cpp
#include "esp_cam_utils.h"

void app_main() {
    // 1. Initialize WiFi (your code here)
    
    // 2. Initialize camera with desired resolution and quality
    ESP_CAM_Init(FRAMESIZE_SVGA, 20); // SVGA resolution, quality 20 (range 0-63, lower = better)
    
    // 3. Capture image and get Gemini API payload
    char* payload = ESP_CAM_CaptureAndGetGeminiPayload(
        "https://generativelanguage.googleapis.com/v1beta/models/gemini-pro-vision:generateContent",
        "YOUR_API_KEY", 
        "Describe what you see in this image"
    );
    
    // 4. Send request to Gemini API
    char* response = ESP_CAM_SendGeminiRequest(
        "https://generativelanguage.googleapis.com/v1beta/models/gemini-pro-vision:generateContent",
        "YOUR_API_KEY",
        payload
    );
    
    // 5. Free payload as it's no longer needed
    ESP_CAM_FreePayload(payload);
    
    // 6. Extract text from Gemini JSON response
    char* text = ESP_CAM_ExtractGeminiResponse(response);
    
    // 7. Process the text (your code here)
    printf("Gemini says: %s\n", text);
    
    // 8. Free resources
    ESP_CAM_FreePayload(response);
    ESP_CAM_FreePayload(text);
}
```

### Complete Example With Error Handling

```cpp
#include "esp_cam_utils.h"
#include "esp_wifi.h"
#include "esp_log.h"

#define WIFI_SSID "your_ssid"
#define WIFI_PASS "your_password"
#define GEMINI_API_KEY "your_api_key"
#define GEMINI_URL "https://generativelanguage.googleapis.com/v1beta/models/gemini-pro-vision:generateContent"

static const char* TAG = "ESP32_CAM_GEMINI";

void app_main() {
    // Initialize WiFi
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    // Wait for WiFi connection
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    
    // Initialize camera
    ret = ESP_CAM_Init(FRAMESIZE_SVGA, 20);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Camera initialization failed with error 0x%x", ret);
        return;
    }
    
    // Flush the camera buffer to avoid stale frames
    ESP_CAM_FlushBuffer();
    
    // Capture image and create Gemini API payload
    char* payload = ESP_CAM_CaptureAndGetGeminiPayload(
        GEMINI_URL,
        GEMINI_API_KEY,
        "What kind of trash do you see in this image? Options: cardboard, glass, metal, paper, plastic, or other."
    );
    
    if (!payload) {
        ESP_LOGE(TAG, "Failed to capture image or create payload");
        return;
    }
    
    // Send request to Gemini API
    char* response = ESP_CAM_SendGeminiRequest(GEMINI_URL, GEMINI_API_KEY, payload);
    
    // Free payload as it's no longer needed
    ESP_CAM_FreePayload(payload);
    
    if (!response) {
        ESP_LOGE(TAG, "Failed to get response from Gemini API");
        return;
    }
    
    // Extract text from Gemini JSON response
    char* text = ESP_CAM_ExtractGeminiResponse(response);
    
    if (!text) {
        ESP_LOGE(TAG, "Failed to extract text from response");
        ESP_LOGI(TAG, "Raw response: %s", response);
        ESP_CAM_FreePayload(response);
        return;
    }
    
    // Display the result
    ESP_LOGI(TAG, "Gemini classified the trash as: %s", text);
    
    // Free resources
    ESP_CAM_FreePayload(response);
    ESP_CAM_FreePayload(text);
}
```

## API Reference

### Camera Initialization

```c
esp_err_t ESP_CAM_Init(framesize_t frame_size, int jpeg_quality);
```

Initializes the ESP32 camera with specified settings.

Parameters:
- `frame_size`: Resolution of the camera (e.g., FRAMESIZE_SVGA)
- `jpeg_quality`: JPEG compression quality (0-63, lower = better quality)

Returns:
- `ESP_OK` on success
- Error code on failure

### Image Capture and Payload Creation

```c
char* ESP_CAM_CaptureAndGetGeminiPayload(const char* gemini_url, const char* api_key, const char* prompt);
```

Captures an image and creates a JSON payload for the Gemini API.

Parameters:
- `gemini_url`: Gemini API endpoint URL
- `api_key`: Your Gemini API key
- `prompt`: Text prompt to send with the image

Returns:
- Pointer to a string containing the JSON payload (must be freed with `ESP_CAM_FreePayload`)
- NULL on failure

### API Request Handling

```c
char* ESP_CAM_SendGeminiRequest(const char* gemini_url, const char* api_key, const char* payload);
```

Sends a request to the Gemini API and gets the response.

Parameters:
- `gemini_url`: Gemini API endpoint URL
- `api_key`: Your Gemini API key
- `payload`: JSON payload (from `ESP_CAM_CaptureAndGetGeminiPayload`)

Returns:
- Pointer to a string containing the JSON response (must be freed with `ESP_CAM_FreePayload`)
- NULL on failure

### Response Processing

```c
char* ESP_CAM_ExtractGeminiResponse(const char* json_response);
```

Extracts the text response from Gemini API JSON.

Parameters:
- `json_response`: JSON response from Gemini API

Returns:
- Pointer to a string containing the extracted text (must be freed with `ESP_CAM_FreePayload`)
- NULL if text couldn't be extracted

### Memory Management

```c
void ESP_CAM_FreePayload(char* payload);
```

Frees memory allocated for payload or response.

Parameters:
- `payload`: Memory allocated by any of the ESP_CAM functions

### Camera Buffer Management

```c
void ESP_CAM_FlushBuffer(void);
```

Flushes the camera buffer to avoid stale frames.

## Configuration Options

### Resolution Options

The library supports all standard resolutions for the OV2640:

| Resolution | Enum Value | Dimensions | Memory Usage |
|------------|------------|------------|--------------|
| QQVGA      | FRAMESIZE_QQVGA | 160x120 | Lowest |
| QVGA       | FRAMESIZE_QVGA  | 320x240 | Low |
| CIF        | FRAMESIZE_CIF   | 400x296 | Low-Medium |
| VGA        | FRAMESIZE_VGA   | 640x480 | Medium |
| SVGA       | FRAMESIZE_SVGA  | 800x600 | Medium-High (Recommended) |
| XGA        | FRAMESIZE_XGA   | 1024x768 | High |
| SXGA       | FRAMESIZE_SXGA  | 1280x1024 | Very High |
| UXGA       | FRAMESIZE_UXGA  | 1600x1200 | Maximum |

### Quality Settings

The JPEG quality parameter ranges from 0 to 63, where lower values mean higher quality:
- 10-20: High quality (larger file size)
- 20-30: Medium quality (recommended)
- 30-40: Low quality (smaller file size)

### Gemini API Models

This library can work with several Gemini vision models:

| Model | API Endpoint Path |
|-------|-------------------|
| Gemini Pro Vision | `gemini-pro-vision:generateContent` |
| Gemini 1.5 Pro | `gemini-1.5-pro:generateContent` |
| Gemini 2.0 Flash | `gemini-2.0-flash:generateContent` |

## Error Handling

The library provides detailed error information:

1. **Camera Errors**: Returned as ESP-IDF error codes
2. **Memory Errors**: Functions return NULL when memory allocation fails
3. **API Errors**: Error messages are embedded in the response JSON
4. **Parsing Errors**: Functions return NULL when parsing fails

## Memory Usage Considerations

For optimal performance with minimal memory usage:

1. Use a moderate resolution (FRAMESIZE_SVGA is recommended)
2. Use a moderate quality setting (20-30 is a good balance)
3. Keep prompts concise
4. Free resources as soon as they're no longer needed

## Implementation Details

1. **Base64 Encoding**: Implemented with chunked processing to minimize memory usage
2. **JSON Building**: Constructed progressively to avoid large buffer allocations
3. **HTTP Client**: Uses ESP-IDF's HTTP client with minimal buffer sizes
4. **Camera Driver**: Stripped down to essential components for OV2640 support

## License

Apache License 2.0