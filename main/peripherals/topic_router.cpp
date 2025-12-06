#include "topic_router.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <algorithm>
#include <cstring>
#include <memory>

#include "config_mgr.hpp"
#include "funlog.h"
#include "peripheral_registry.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {
TopicRouter::TopicRouter() {
  _subscriptions = ps_new_subscriber(kSubscriptionQueueLength, PS_STRLIST("peripheral", "sensor", "router"));
  _config = std::make_shared<SettingsMap>();
}

TopicRouter::~TopicRouter() {
  if (_subscriptions) {
    ps_free_subscriber(_subscriptions);
    _subscriptions = nullptr;
  }
  FLOG_ERROR("TopicRouter destroyed");
}

void TopicRouter::Init() {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);

  GetSettings(_config, "peripheral");
  ApplySettings(_config);
}

esp_err_t TopicRouter::StartTask() {
  // BUG: memory leak here, how do i do this better?
  static TopicRouter* self = new TopicRouter();
  // auto inst = std::make_shared<TopicRouter>();  // ctor on task stack

  xTaskCreatePinnedToCore(
      [](void* ctx) {
        TopicRouter* router = static_cast<TopicRouter*>(ctx);
        FLOG_INFO("TopicRouter task started");
        router->Init();
        while (true) {
          router->Loop();
          vTaskDelay(10 / portTICK_PERIOD_MS);
          taskYIELD();
        };
        FLOG_ERROR("TopicRouter task exiting");
        delete router;
      },
      "TopicRouterTask", 4096, self, tskIDLE_PRIORITY + 1, nullptr, 1);
  return ESP_OK;
}

esp_err_t TopicRouter::Loop() {
  ps_msg_t* msg = NULL;
  // TopicParts parts;
  for ((msg = ps_get(_subscriptions, 0)); msg != NULL; (msg = ps_get(_subscriptions, 0))) {
    if (ps_has_topic_prefix(msg, "router")) {
      FLOG_DEBUG("Router message received on topic %s, ignoring to prevent loops", msg->topic);
      ps_unref_msg(msg);
      continue;
    } else if (ps_has_topic_prefix(msg, "peripheral.config.get")) {
      FLOG_INFO("Peripheral config get message received");
      // std::shared_ptr<SettingsMap> config = std::make_shared<PeripheralConfig>();
      // std::shared_ptr<SettingsMap> config = std::make_shared<SettingsMap>();
      GetSettings(_config, "peripheral");
      ApplySettings(_config);
      ps_unref_msg(msg);
      continue;
    } else if (ps_has_topic_prefix(msg, "sensor")) {
      // ParseTopic(msg->topic, &parts);
      for (const auto& route : _routes) {
        if (route.from == msg->topic) {
          FLOG_DEBUG("Routing message from %s to %s", route.from.c_str(), route.to.c_str());
          switch (msg->flags & PS_MSK_TYP) {
            case PS_STR_TYP:
              FLOG_DEBUG("Message is string: %s", msg->str_val);
              PS_PUB_STR(route.to.c_str(), msg->str_val);
              break;
            case PS_INT_TYP:
              FLOG_DEBUG("Message is int: %lld", msg->int_val);
              PS_PUB_INT(route.to.c_str(), msg->int_val);
              break;
            case PS_BOOL_TYP:
              FLOG_DEBUG("Message is bool: %s", msg->bool_val ? "true" : "false");
              PS_PUB_BOOL(route.to.c_str(), msg->bool_val);
              break;
            case PS_PTR_TYP:
              FLOG_DEBUG("Message is ptr: %p", msg->ptr_val);
              PS_PUB_PTR(route.to.c_str(), msg->ptr_val);
              break;
            case PS_NIL_TYP:
              FLOG_DEBUG("Message is nil");
              PS_PUB_NIL(route.to.c_str());
              break;
            default:
              FLOG_DEBUG("Message is of unknown type: 0x%X", msg->flags & PS_MSK_TYP);
              break;
          }
          PS_PUB_PTR(route.to.c_str(), msg);
        }
      }
      ps_unref_msg(msg);
      continue;
    } else {
      FLOG_WARN("Unknown topic prefix for topic %s", msg->topic);
    }
  }
  ps_unref_msg(msg);
  return ESP_OK;
}

void TopicRouter::ApplySettings(std::shared_ptr<SettingsMap> config) {
  _routes.clear();
  if (config->contains("zone_temp")) {
    FLOG_DEBUG("Applying zone_temp setting");
    std::string zone_temp = std::get<std::string>(config->at("zone_temp"));
    std::string ztemp_topic = PeripheralRegistry::GetPeripheralTopic(zone_temp.c_str());
    if (!ztemp_topic.empty()) {
      FLOG_DEBUG("Zone temperature sensor topic: %s", ztemp_topic.c_str());
      // PS_PUB_STR("peripheral.zone.temperature_sensor.set", ztemp_topic.c_str());
      // RemoveRoute("sensor.temperature.zone");
      AddRoute(ztemp_topic.c_str(), "sensor.temperature.zone");
    } else {
      FLOG_WARN("Zone temperature sensor '%s' not found in registry", zone_temp.c_str());
    }
    // PeripheralRegistry::GetInstance().SetZoneTemperatureSensor(zone_temp.c_str());
  }
  if (config->contains("zone_heater")) {
    FLOG_DEBUG("Zone_heater not managed here");
    // std::string zone_heater = std::get<std::string>(config->at("zone_heater"));
    // PeripheralRegistry::GetPeripheralTopic(zone_heater.c_str());

    // std::string zone_heater = std::get<std::string>((*config)["zone_heater"]);
    // PeripheralRegistry::GetInstance().SetZoneHeater(zone_heater.c_str());
  }
  // PeripheralRegistry& registry = PeripheralRegistry::GetInstance();
  // for (const auto& [key, value] : *config) {
  //   FLOG_DEBUG("Applying setting %s", key.c_str());
  //   if (key == "routes" && std::holds_alternative<std::vector<uint8_t>>(value)) {
  //     const auto& buf = std::get<std::vector<uint8_t>>(value);
  //     std::string serialized(buf.begin(), buf.end());
  //     // Deserialize routes
  //   }
  // }
}

void TopicRouter::AddRoute(const std::string from, const std::string to) {
  FLOG_DEBUG("Adding route from %s to %s", from.c_str(), to.c_str());
  _routes.push_back(Route{from, to});
}

void TopicRouter::RemoveRoute(std::string route) {
  FLOG_DEBUG("Removing routes from %s", route);
  _routes.erase(std::remove_if(_routes.begin(), _routes.end(),
                               [route](const Route& r) { return r.from.compare(route) || r.to.compare(route); }),
                _routes.end());
}

void TopicRouter::RemoveTarget(std::string to) {
  FLOG_DEBUG("Removing route  %s", to.c_str());
  _routes.erase(std::remove_if(_routes.begin(), _routes.end(), [to](const Route& r) { return r.to.compare(to); }),
                _routes.end());
}

}  // namespace toothless