#pragma once

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "config_mgr.hpp"
#include "esp_err.h"
#include "peripheral.hpp"
#include "topics.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

inline constexpr size_t kSubscriptionQueueLength = 10;

enum TopicVerbs { kVerbGet, kVerbSet, kVerbErase };

using TopicParts = struct {
  bool incoming;
  std::string target;  // "sensor".
  std::string type;    // sensor type, e.g., "temperature"
  std::string bus;     // bus type, e.g., "i2c"
  // TopicVerbs verb;
  // size_t Topic(char *buf) {
  //   char topic[kMaxTopicLength];
  //   snprintf(topic, kMaxTopicLength, "%s.%s", module.c_str(), kTopicConfig);
  //   return strlen(topic);
  // };
  // size_t TopicGet(char* buf) {
  //   // char topic[kMaxTopicLength];
  //   snprintf(buf, kMaxTopicLength, "%s.%s", module.c_str(), kTopicConfigGet);
  //   return strlen(buf);
  // };

  // size_t TopicSet(char* buf) {
  //   // char topic[kMaxTopicLength];
  //   snprintf(buf, kMaxTopicLength, "%s.%s", module.c_str(), kTopicConfigSet);
  //   return strlen(buf);
  // };
};

class TopicRouter {
 public:
  TopicRouter();
  ~TopicRouter();
  void Init();
  static esp_err_t StartTask();
  esp_err_t Loop();
  void ApplySettings(std::shared_ptr<SettingsMap> config);
  void AddRoute(const std::string from, const std::string to);
  void RemoveRoute(std::string from);
  void RemoveTarget(std::string to);
  // uint32_t ParseTopic(const char* topic, TopicParts* parts);

 private:
  ps_subscriber_t* _subscriptions = nullptr;
  struct Route {
    std::string from;
    std::string to;
  };
  std::vector<Route> _routes;
  std::shared_ptr<SettingsMap> _config = nullptr;

  static void OnMessage(const char* topic, ps_msg_t* msg, void* ctx);
};
}  // namespace toothless