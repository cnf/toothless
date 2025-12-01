#include "topic_router.hpp"

#include <algorithm>

namespace toothless {
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