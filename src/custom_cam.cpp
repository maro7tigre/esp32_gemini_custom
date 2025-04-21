#include "custom_cam.h"
#include <Arduino.h>
#include "esp_camera.h"
#include "driver/gpio.h"
#include <WiFiClientSecure.h>

// Base64 encoding table
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Gemini API host
static const char* GEMINI_HOST = "generativelanguage.googleapis.com";
static const int GEMINI_PORT = 443;

// ESP32-CAM pin definitions (hardcoded for AI-Thinker ESP32-CAM)
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0      5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

// Flash LED pin
#define FLASH_GPIO_PIN  4

bool initCamera(void) {
    // Set up flash LED
    pinMode(FLASH_GPIO_PIN, OUTPUT);
    digitalWrite(FLASH_GPIO_PIN, LOW);
    
    // Camera configuration
    camera_config_t camera_config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,
        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_VGA,  // 640x480, less memory than UXGA
        .jpeg_quality = 10,           // Good quality (0-63, lower is better)
        .fb_count = 2,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_LATEST
    };
    
    // Initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        return false;
    }
    
    // Fine-tune camera settings
    sensor_t* sensor = esp_camera_sensor_get();
    if (sensor) {
        sensor->set_brightness(sensor, 0);     
        sensor->set_contrast(sensor, 0);       
        sensor->set_saturation(sensor, 0);     
        sensor->set_sharpness(sensor, 0);      
        sensor->set_denoise(sensor, 1);        
        sensor->set_whitebal(sensor, 1);       
        sensor->set_exposure_ctrl(sensor, 1);  
    }
    
    return true;
}

// Calculate base64 encoded length
static size_t calculateBase64Length(size_t input_length) {
    return ((input_length + 2) / 3 * 4) + 1;  // +1 for null terminator
}

// Encode image to JSON format for Gemini API
static size_t encodeToGeminiJson(
    const uint8_t* input_data, 
    size_t input_length, 
    char* output_buffer, 
    size_t output_buffer_size, 
    const char* prompt,
    const char* gemini_key) {
    
    if (!input_data || !output_buffer || input_length == 0) {
        return 0;
    }
    
    // JSON format template parts
    const char* json_prefix = "{\n"
        "  \"contents\":[\n"
        "    {\n"
        "      \"parts\":[\n"
        "        {\"text\":\"";
    
    const char* prompt_suffix = "\"},\n"
        "        {\"inline_data\":{\n"
        "          \"mime_type\":\"image/jpeg\",\n"
        "          \"data\":\"";
    
    const char* json_suffix = "\"\n"
        "        }}\n"
        "      ]\n"
        "    }\n"
        "  ],\n"
        "  \"generationConfig\":{\n"
        "    \"maxOutputTokens\":5,\n"
        "    \"temperature\":1\n"
        "  }\n"
        "}";
    
    // Calculate total size needed
    size_t base64_length = calculateBase64Length(input_length) - 1;
    size_t total_size = 
        strlen(json_prefix) + 
        strlen(prompt) + 
        strlen(prompt_suffix) + 
        base64_length + 
        strlen(json_suffix) + 
        1; // +1 for null terminator
    
    if (total_size > output_buffer_size) {
        return 0;
    }
    
    // Build JSON output
    char* pos = output_buffer;
    
    // Copy JSON prefix
    strcpy(pos, json_prefix);
    pos += strlen(json_prefix);
    
    // Copy prompt (escaping quotes)
    for (const char* p = prompt; *p; p++) {
        if (*p == '"' || *p == '\\') {
            *pos++ = '\\';
        }
        *pos++ = *p;
    }
    
    // Copy prompt suffix
    strcpy(pos, prompt_suffix);
    pos += strlen(prompt_suffix);
    
    // Encode image data to Base64
    size_t i;
    uint32_t a, b, c;
    
    for (i = 0; i < input_length; i += 3) {
        a = i < input_length ? input_data[i] : 0;
        b = (i + 1) < input_length ? input_data[i + 1] : 0;
        c = (i + 2) < input_length ? input_data[i + 2] : 0;
        
        uint32_t triple = (a << 16) | (b << 8) | c;
        
        *pos++ = base64_table[(triple >> 18) & 0x3F];
        *pos++ = base64_table[(triple >> 12) & 0x3F];
        *pos++ = (i + 1) < input_length ? base64_table[(triple >> 6) & 0x3F] : '=';
        *pos++ = (i + 2) < input_length ? base64_table[triple & 0x3F] : '=';
    }
    
    // Copy JSON suffix
    strcpy(pos, json_suffix);
    pos += strlen(json_suffix);
    
    return pos - output_buffer;
}

char* captureImageAsGeminiJson(const char* prompt, size_t* encoded_size, const char* gemini_key) {
    if (!prompt || !gemini_key) {
        return NULL;
    }
    
    // Turn on flash
    digitalWrite(FLASH_GPIO_PIN, HIGH);
    delay(75);  // Wait for flash to stabilize
    
    // Capture frame
    camera_fb_t* fb = esp_camera_fb_get();
    
    // Turn off flash immediately
    digitalWrite(FLASH_GPIO_PIN, LOW);
    
    if (!fb || fb->format != PIXFORMAT_JPEG) {
        if (fb) esp_camera_fb_return(fb);
        return NULL;
    }
    
    // Calculate output size needed
    size_t base64_len = calculateBase64Length(fb->len);
    size_t json_overhead = 500; 
    size_t prompt_len = strlen(prompt) * 2;
    size_t buffer_size = base64_len + json_overhead + prompt_len;
    
    // Allocate buffer for JSON output
    char* json_buffer = (char*)malloc(buffer_size);
    if (!json_buffer) {
        esp_camera_fb_return(fb);
        return NULL;
    }
    
    // Encode to JSON
    size_t json_len = encodeToGeminiJson(fb->buf, fb->len, json_buffer, buffer_size, prompt, gemini_key);
    
    // Free the camera frame buffer
    esp_camera_fb_return(fb);
    
    if (json_len == 0) {
        free(json_buffer);
        return NULL;
    }
    
    // Set encoded size if requested
    if (encoded_size) {
        *encoded_size = json_len;
    }
    
    return json_buffer;
}

char* sendToGeminiAPI(const char* json_payload, const char* gemini_key) {
    if (!json_payload || !gemini_key) {
        return NULL;
    }
    
    // Create secure client
    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation
    
    // Connect to Gemini API
    if (!client.connect(GEMINI_HOST, GEMINI_PORT)) {
        return NULL;
    }
    
    // Build API URL
    char url[256];
    snprintf(url, sizeof(url), 
             "/v1beta/models/gemini-2.0-flash-lite:generateContent?key=%s", 
             gemini_key);
    
    // Calculate payload length
    size_t payload_len = strlen(json_payload);
    
    // Build HTTP request
    client.printf("POST %s HTTP/1.1\r\n", url);
    client.printf("Host: %s\r\n", GEMINI_HOST);
    client.println("Content-Type: application/json");
    client.printf("Content-Length: %u\r\n", payload_len);
    client.println("Connection: close");
    client.println();
    
    // Send payload in chunks
    const size_t CHUNK_SIZE = 1024;
    const char* pos = json_payload;
    size_t remaining = payload_len;
    
    while (remaining > 0) {
        size_t chunk = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;
        size_t sent = client.write((const uint8_t*)pos, chunk);
        if (sent == 0) {
            client.stop();
            return NULL;
        }
        
        pos += sent;
        remaining -= sent;
        delay(1);
    }
    
    // Wait for response with timeout
    unsigned long timeout = millis();
    while (client.connected() && !client.available()) {
        if (millis() - timeout > 10000) {
            client.stop();
            return NULL;
        }
        delay(100);
    }
    
    // Skip HTTP headers
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            break;
        }
    }
    
    // Read response body
    String response = client.readString();
    client.stop();
    
    // Allocate buffer for response
    char* response_buffer = (char*)malloc(response.length() + 1);
    if (!response_buffer) {
        return NULL;
    }
    
    // Copy response
    strcpy(response_buffer, response.c_str());
    return response_buffer;
}