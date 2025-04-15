/**
 * @file main.cpp
 * @brief ESP32-CAM Gemini Vision example using our custom library
 */

 #include <Arduino.h>
 #include <WiFi.h>
 #include "soc/soc.h"           // Disable brownout problems
 #include "soc/rtc_cntl_reg.h"  // Disable brownout problems
 #include "esp_cam_utils.h"
 #include "credentials.h"       // Include your WiFi credentials and API key
 
 // Default camera resolution
 framesize_t currentResolution = FRAMESIZE_SVGA; // Default resolution
 
 // Resolution mapping based on numeric input (1-8)
 framesize_t resolutionMap[] = {
   FRAMESIZE_QQVGA,   // 1: 160x120
   FRAMESIZE_QVGA,    // 2: 320x240
   FRAMESIZE_CIF,     // 3: 400x296
   FRAMESIZE_VGA,     // 4: 640x480
   FRAMESIZE_SVGA,    // 5: 800x600
   FRAMESIZE_XGA,     // 6: 1024x768
   FRAMESIZE_SXGA,    // 7: 1280x1024
   FRAMESIZE_UXGA     // 8: 1600x1200
 };
 
 // Resolution names for diagnostic output
 const char* resolutionNames[] = {
   "QQVGA (160x120)",
   "QVGA (320x240)",
   "CIF (400x296)",
   "VGA (640x480)",
   "SVGA (800x600)",
   "XGA (1024x768)",
   "SXGA (1280x1024)",
   "UXGA (1600x1200)"
 };
 
 // Quality settings
 int currentQuality = 20; // Default quality (0-63, lower is better)
 
 // API settings
 const char* GEMINI_URL = "https://generativelanguage.googleapis.com/v1beta/models/gemini-pro-vision:generateContent";
 const char* GEMINI_MODEL = "gemini-pro-vision";
 const char* PROMPT = "I want a short answer for which trash type do you see in the image [cardboard, glass, metal, paper, plastic or other]";
 
 // Update camera resolution based on user input
 bool updateResolution(int resolutionIndex) {
   if (resolutionIndex < 1 || resolutionIndex > 8) {
     return false;
   }
   
   // Adjust for 0-based array index
   resolutionIndex--;
   
   // Only update if resolution changed
   if (currentResolution != resolutionMap[resolutionIndex]) {
     currentResolution = resolutionMap[resolutionIndex];
     
     // Reinitialize camera with new resolution
     esp_camera_deinit();
     if (ESP_CAM_Init(currentResolution, currentQuality) != ESP_OK) {
       Serial.println("Camera reinitialization failed!");
       return false;
     }
     
     Serial.print("Resolution changed to: ");
     Serial.println(resolutionNames[resolutionIndex]);
   }
   
   return true;
 }
 
 // Update camera quality based on user input
 bool updateQuality(int quality) {
   if (quality < 0 || quality > 63) {
     return false;
   }
   
   if (currentQuality != quality) {
     currentQuality = quality;
     
     // Reinitialize camera with new quality
     esp_camera_deinit();
     if (ESP_CAM_Init(currentResolution, currentQuality) != ESP_OK) {
       Serial.println("Camera reinitialization failed!");
       return false;
     }
     
     Serial.print("Quality changed to: ");
     Serial.println(currentQuality);
   }
   
   return true;
 }
 
 // Capture an image and get analysis from Gemini
 void captureAndAnalyze() {
   Serial.println("--- Starting detection ---");
   
   // Find the current resolution name
   for (int i = 0; i < 8; i++) {
     if (resolutionMap[i] == currentResolution) {
       Serial.print("Resolution: ");
       Serial.println(resolutionNames[i]);
       break;
     }
   }
   Serial.print("Quality: ");
   Serial.println(currentQuality);
   
   // Flush the camera buffer so the next capture is fresh
   ESP_CAM_FlushBuffer();
   
   Serial.println("Capturing image and sending to Gemini...");
   
   // Use our library's all-in-one function to capture, create payload, send to Gemini, and process response
   char* result = ESP_CAM_CaptureAndAnalyze(GEMINI_URL, GEMINI_API_KEY, GEMINI_MODEL, PROMPT);
   
   if (result) {
     Serial.print("Result: ");
     Serial.println(result);
     
     // Free the result memory
     ESP_CAM_FreePayload(result);
   } else {
     Serial.println("Analysis failed");
   }
   
   Serial.println("\nCommands:");
   Serial.println("r1-r8: Change resolution (e.g., r4 for VGA)");
   Serial.println("q0-q63: Change quality (e.g., q20 for quality 20)");
   Serial.println("c: Capture and analyze image");
 }
 
 void setup() {
   // Disable brownout detector
   WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
   
   // Initialize serial communication
   Serial.begin(115200);
   delay(1000);
   Serial.println("\nESP32-CAM Gemini Vision Integration");
   
   // Connect to WiFi
   Serial.print("Connecting to WiFi: ");
   Serial.println(WIFI_SSID);
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
   
   // Wait for WiFi connection with timeout
   int timeout = 0;
   while (WiFi.status() != WL_CONNECTED && timeout < 20) {
     delay(500);
     Serial.print(".");
     timeout++;
   }
   
   if (WiFi.status() == WL_CONNECTED) {
     Serial.println("\nWiFi connected!");
     Serial.print("IP Address: ");
     Serial.println(WiFi.localIP());
   } else {
     Serial.println("\nWiFi connection failed! Please reset the device.");
     while (1) delay(1000);
   }
   
   // Initialize camera
   if (ESP_CAM_Init(currentResolution, currentQuality) != ESP_OK) {
     Serial.println("Camera initialization failed!");
     while (1) delay(1000);
   }
   
   Serial.println("All systems ready");
   Serial.println("\nCommands:");
   Serial.println("r1-r8: Change resolution (e.g., r4 for VGA)");
   Serial.println("q0-q63: Change quality (e.g., q20 for quality 20)");
   Serial.println("c: Capture and analyze image");
 }
 
 void loop() {
   if (Serial.available() > 0) {
     String command = Serial.readStringUntil('\n');
     command.trim();
     
     if (command.length() > 0) {
       char firstChar = command.charAt(0);
       
       // Process resolution change
       if (firstChar == 'r' && command.length() >= 2) {
         int resolutionChoice = command.substring(1).toInt();
         if (updateResolution(resolutionChoice)) {
           Serial.println("Resolution updated");
         } else {
           Serial.println("Invalid resolution. Use r1-r8");
         }
       }
       // Process quality change
       else if (firstChar == 'q' && command.length() >= 2) {
         int qualityValue = command.substring(1).toInt();
         if (updateQuality(qualityValue)) {
           Serial.println("Quality updated");
         } else {
           Serial.println("Invalid quality. Use q0-q63");
         }
       }
       // Process capture command
       else if (firstChar == 'c') {
         captureAndAnalyze();
       }
       // Help command
       else if (firstChar == 'h') {
         Serial.println("\nCommands:");
         Serial.println("r1-r8: Change resolution (e.g., r4 for VGA)");
         Serial.println("q0-q63: Change quality (e.g., q20 for quality 20)");
         Serial.println("c: Capture and analyze image");
         Serial.println("h: Show this help");
       }
       else {
         Serial.println("Unknown command. Type 'h' for help.");
       }
     }
   }
   
   delay(10); // Small delay to prevent CPU hogging
 }