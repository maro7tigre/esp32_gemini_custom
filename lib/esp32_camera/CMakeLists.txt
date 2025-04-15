# get IDF version for comparison
set(idf_version "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}")

# Set core files
set(srcs
  driver/esp_camera.c
  driver/cam_hal.c
  driver/sensor.c
  sensors/ov2640.c
  driver/sccb.c
  target/xclk.c
  target/esp32/ll_cam.c
  utils/esp_cam_base64.c
  utils/esp_cam_json.c
  utils/gemini_client.c
  )

set(include_dirs
  driver/include
  include
  utils
  )

set(priv_include_dirs
  driver/private_include
  sensors/private_include
  target/private_include
  )

set(COMPONENT_REQUIRES driver)
set(priv_requires freertos nvs_flash esp_http_client esp_timer)

if (idf_version VERSION_GREATER_EQUAL "4.2")
  list(APPEND priv_requires esp_timer)
endif()

idf_component_register(
  SRCS ${srcs}
  INCLUDE_DIRS ${include_dirs}
  PRIV_INCLUDE_DIRS ${priv_include_dirs}
  REQUIRES driver
  PRIV_REQUIRES ${priv_requires}
)