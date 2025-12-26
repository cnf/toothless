#include "config.h"

extern "C" {
#include <pubsub.h>
}

#include <QDispatch.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_lib_utils.h>
#include <esp_timer.h>

#include <memory>

#include "funlog.h"
#include "heater/heater.hpp"
#include "i2c_manager.hpp"
#include "networking.hpp"
#include "peripherals/peripheral_registry.hpp"
#include "ui/display/display.hpp"
#include "ui/user_interface.hpp"

DynamicContextPool context_pool;
TaskDispatcher main_dispatcher(&context_pool);
TaskDispatcher prio_dispatcher(&context_pool);

// Declarations
extern "C" void app_main();  // Function prototype
void SetLogLevels();

/// @brief Set the log levels for various components
void SetLogLevels() {
#if defined(CONFIG_FLOG_LEVEL)
  esp_log_level_set("*", (esp_log_level_t)CONFIG_FLOG_LEVEL);
#else
  esp_log_level_set("*", ESP_LOG_WARN);
#endif
  FLOG_INFO("current esp log level is %i", (int)esp_log_level_get(NULL));
  FLOG_INFO("max log level is %i", CONFIG_LOG_MAXIMUM_LEVEL);
  // esp_log_set_vprintf(_ps_vprintf);
  // Set the log levels for various components

  esp_log_level_set("wifi", ESP_LOG_ERROR);
  esp_log_level_set("wifi_init", ESP_LOG_WARN);
  // esp_log_level_set("lcd_panel.io.i2c", ESP_LOG_ERROR);
  // esp_log_level_set("i2c", ESP_LOG_ERROR);
  // esp_log_level_set("i2c.master", ESP_LOG_NONE);
  // esp_log_level_set("i2c_manager.cpp", ESP_LOG_DEBUG);

  // esp_log_level_set("nvs", ESP_LOG_NONE);
  // esp_log_level_set("tmc2208", ESP_LOG_ERROR);
  // esp_log_level_set("efuse", ESP_LOG_ERROR);
  // esp_log_level_set("gpio", ESP_LOG_ERROR);
}

using namespace toothless;

extern "C" void app_main(void) {
  FLOG_INFO("================= Starting Toothless =================");
  SetLogLevels();
  usleep(1000 * 100);

  FLOG_INFO("Initializing pubsub msg bus");
  ps_init();
  usleep(1000 * 100);

  FLOG_INFO("Initializing configuration manager");
  ConfigManager::Start();

#if defined(CONFIG_IOM_I2C_SDA_PIN) && defined(CONFIG_IOM_I2C_SCL_PIN)
  FLOG_INFO("Initializing I2C");
  I2cManager::GetInstance()->Init();
#if defined(CONFIG_IOM_EXTERNAL_ENABLE)
  I2cManager::GetExternalInstance()->Init();
#endif
#endif

  Display::Init();

  // Dispatcher
  main_dispatcher.schedulingPolicy = TaskDispatcher::TIMING;

  // Networking
  FLOG_INFO("Initializing networking");
  static networking::NetworkManager network_mgr;
  network_mgr.Init();
  main_dispatcher.callEvery(500, &networking::NetworkManager::Loop, &network_mgr);

  FLOG_INFO("Initializing peripherals");
  auto& registry = PeripheralRegistry::Instance();
  registry.Init();
  main_dispatcher.callEvery(50, &PeripheralRegistry::Loop, &registry);

  // TopicRouter::StartTask();

  FLOG_INFO("Initializing heater");
  static Heater heater;
  heater.Init();
  prio_dispatcher.callEvery(200, &Heater::Loop, &heater);

  FLOG_INFO("Initializing User Interface");
  UserInterface::Start();

  xTaskCreatePinnedToCore(
      [](void* arg) {
        FLOG_INFO("Starting high priority dispatcher thread");
        while (true) {
          prio_dispatcher.run();
          vTaskDelay(10 / portTICK_PERIOD_MS);
          taskYIELD();
        }
      },
      "HighPrioDispatcher", 4096, NULL, 20, NULL, 1);
  FLOG_INFO("Init done");

  FLOG_INFO("Starting main thread");
  while (true) {
    main_dispatcher.run();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    taskYIELD();
  }
}
