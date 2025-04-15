#include <stdio.h>
#include "sensor.h"

// Simplified camera sensor table with only OV2640
const camera_sensor_info_t camera_sensor[CAMERA_MODEL_MAX] = {
    // The sequence must be consistent with camera_model_t
    {CAMERA_OV2640, "OV2640", OV2640_SCCB_ADDR, OV2640_PID, FRAMESIZE_UXGA, true},
    // Keep these empty entries to maintain the array structure
    {CAMERA_NONE, "", 0, 0, 0, false},
    {CAMERA_NONE, "", 0, 0, 0, false},
    {CAMERA_NONE, "", 0, 0, 0, false},
    {CAMERA_NONE, "", 0, 0, 0, false},
    {CAMERA_NONE, "", 0, 0, 0, false},
    {CAMERA_NONE, "", 0, 0, 0, false},
    {CAMERA_NONE, "", 0, 0, 0, false},
    {CAMERA_NONE, "", 0, 0, 0, false},
    {CAMERA_NONE, "", 0, 0, 0, false},
    {CAMERA_NONE, "", 0, 0, 0, false},
    {CAMERA_NONE, "", 0, 0, 0, false},
    {CAMERA_NONE, "", 0, 0, 0, false},
    {CAMERA_NONE, "", 0, 0, 0, false}
};

// Simplified resolution table
const resolution_info_t resolution[FRAMESIZE_INVALID] = {
    {   96,   96, ASPECT_RATIO_1X1   }, /* 96x96 */
    {  160,  120, ASPECT_RATIO_4X3   }, /* QQVGA */
    {  128,  128, ASPECT_RATIO_1X1   }, /* 128x128 */
    {  176,  144, ASPECT_RATIO_5X4   }, /* QCIF  */
    {  240,  176, ASPECT_RATIO_4X3   }, /* HQVGA */
    {  240,  240, ASPECT_RATIO_1X1   }, /* 240x240 */
    {  320,  240, ASPECT_RATIO_4X3   }, /* QVGA  */
    {  320,  320, ASPECT_RATIO_1X1   }, /* 320x320 */
    {  400,  296, ASPECT_RATIO_4X3   }, /* CIF   */
    {  480,  320, ASPECT_RATIO_3X2   }, /* HVGA  */
    {  640,  480, ASPECT_RATIO_4X3   }, /* VGA   */
    {  800,  600, ASPECT_RATIO_4X3   }, /* SVGA  */
    { 1024,  768, ASPECT_RATIO_4X3   }, /* XGA   */
    { 1280,  720, ASPECT_RATIO_16X9  }, /* HD    */
    { 1280, 1024, ASPECT_RATIO_5X4   }, /* SXGA  */
    { 1600, 1200, ASPECT_RATIO_4X3   }, /* UXGA  */
    // Reduce the additional entries that were causing warnings
    { 1920, 1080, ASPECT_RATIO_16X9  }, /* FHD   */
    {  720, 1280, ASPECT_RATIO_9X16  }, /* Portrait HD   */
    {  864, 1536, ASPECT_RATIO_9X16  }, /* Portrait 3MP   */
    { 2048, 1536, ASPECT_RATIO_4X3   } /* QXGA  */
};

camera_sensor_info_t *esp_camera_sensor_get_info(sensor_id_t *id)
{
    for (int i = 0; i < CAMERA_MODEL_MAX; i++) {
        if (id->PID == camera_sensor[i].pid) {
            return (camera_sensor_info_t *)&camera_sensor[i];
        }
    }
    return NULL;
}