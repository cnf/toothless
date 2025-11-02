#include "config.h"

extern "C" {
#include <pubsub.h>
}

#include "funlog.h"
#include "heater/heater.hpp"
#include "max6675.h"
#include "sensors/sensors.hpp"
#include "ui/user_interface.hpp"
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_lib_utils.h>
#include <esp_timer.h>
#include <memory>

#include <QDispatch.h>

DynamicContextPool context_pool;
TaskDispatcher main_dispatcher(&context_pool);
// TaskDispatcher prio_dispatcher(&context_pool);

/// @brief Set the log levels for various components
void SetLogLevels() {
#if defined(CONFIG_FLOG_LEVEL)
  esp_log_level_set("*", (esp_log_level_t)CONFIG_FLOG_LEVEL);
#else
  esp_log_level_set("*", ESP_LOG_WARN);
#endif
  FLOG_INFO("current esp log level is %i", (int)esp_log_level_get(NULL));
  FLOG_INFO("max log level is %i", CONFIG_LOG_MAXIMUM_LEVEL);
  // funlog_set_net_fun_log(&NetFunLog); // Set the custom log function for pubsub messages
  // esp_log_set_vprintf(_ps_vprintf);
  // Set the log levels for various components

  esp_log_level_set("wifi", ESP_LOG_ERROR);
  esp_log_level_set("wifi_init", ESP_LOG_WARN);

  // esp_log_level_set("nvs", ESP_LOG_NONE);
  // esp_log_level_set("tmc2208", ESP_LOG_ERROR);
  // esp_log_level_set("efuse", ESP_LOG_ERROR);
  // esp_log_level_set("gpio", ESP_LOG_ERROR);
  esp_log_level_set("heater.cpp", ESP_LOG_INFO);
}

// using namespace esp_panel::drivers;
// using namespace esp_panel::board;
using namespace toothless;

extern "C" void app_main(void) {
  SetLogLevels();
  // TODO: wait()
  usleep(1000 * 500);

  FLOG_INFO("ESP-IDF version is: %s", esp_get_idf_version());

  FLOG_INFO("Initializing pubsub msg bus");
  ps_init();

  FLOG_INFO("Initialize SPI bus");
  spi_bus_config_t buscfg = {
      .mosi_io_num = BSP_SD_SPI_MOSI,
      .miso_io_num = BSP_SD_SPI_MISO,
      .sclk_io_num = BSP_SD_SPI_SCLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = CONFIG_TL_DISPLAY_HRES * 80 * sizeof(uint16_t),
  };
  ESP_ERROR_CHECK(spi_bus_initialize(BSP_SD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
  main_dispatcher.schedulingPolicy = TaskDispatcher::TIMING;

  FLOG_INFO("Initializing User Interface");
  UserInterface ui;
  ui.Init();
  main_dispatcher.callEvery(100, &UserInterface::Loop, &ui);

  FLOG_INFO("Initializing sensors");
  Sensors sensors;
  sensors.Init();
  main_dispatcher.callEvery(100, &Sensors::Loop, &sensors);

  FLOG_INFO("Initializing heater");
  Heater heater(GPIO_NUM_12);
  heater.Init();
  main_dispatcher.callEvery(200, &Heater::Loop,
                            &heater); // TODO: Heater will be run on its own core, focusing on UI first

  FLOG_INFO("Init done");

  // uint64_t timer = esp_timer_get_time();

  // gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);
  // gpio_set_level(GPIO_NUM_13, 1);
  // bool lvl = 1;

  ps_subscriber_t *sub = ps_new_subscriber(10, PS_STRLIST("sensor.chamber.temperature"));

  FLOG_INFO("Starting main thread");
  while (true) {
    main_dispatcher.run();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    taskYIELD();
  }
}
