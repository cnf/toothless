#pragma once

#include <string>
#include <vector>

extern "C" {
#include <pubsub.h>
}

namespace toothless {
class TopicRouter {
 public:
  void AddRoute(const char* from, const char* to);
  void RemoveRoute(const char* from);
  // void LoadFromNvs();
  // void SaveToNvs();
  void Start();  // Subscribes to all source topics

 private:
  struct Route {
    std::string from;
    std::string to;
  };
  std::vector<Route> _routes;
  ps_subscriber_t* _sub;

  static void OnMessage(const char* topic, ps_msg_t* msg, void* ctx);
};
}  // namespace toothless