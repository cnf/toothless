#include "topic_router.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <algorithm>
#include <cstring>
#include <memory>

#include "funlog.h"

extern "C" {
#include <pubsub.h>
}

namespace toothless {
TopicRouter::TopicRouter() {
  // _subscription = ps_new_subscriber();
  _subscriptions = ps_new_subscriber(kSubscriptionQueueLength, PS_STRLIST("sensor", "router"));
}

TopicRouter::~TopicRouter() {
  if (_subscriptions) {
    ps_free_subscriber(_subscriptions);
    _subscriptions = nullptr;
  }
}

void TopicRouter::Init() {}

esp_err_t TopicRouter::StartTask() {
  auto inst = std::make_shared<TopicRouter>();  // ctor on task stack

  xTaskCreatePinnedToCore(
      [](void* ctx) {
        TopicRouter* router = static_cast<TopicRouter*>(ctx);
        while (true) {
          router->Loop();
          vTaskDelay(10 / portTICK_PERIOD_MS);
          taskYIELD();
        };
      },
      "TopicRouterTask", 4096, this, tskIDLE_PRIORITY + 1, nullptr, 1);
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

void TopicRouter::AddRoute(const char* from, const char* to) { _routes.push_back(Route{from, to}); }

void TopicRouter::RemoveRoute(const char* from) {
  _routes.erase(std::remove_if(_routes.begin(), _routes.end(), [from](const Route& r) { return r.from == from; }),
                _routes.end());
}

void TopicRouter::Start() {
  for (const auto& route : _routes) {
    // PS_SUB(route.from.c_str(), TopicRouter::OnMessage, this);
  }
}

}  // namespace toothless