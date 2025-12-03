#pragma once

/// @file network_topics.hpp
/// @brief PubSub topics for network events
/// @details Subscribe to these topics to receive network state changes.

namespace networking {

/// WiFi state changes (int: WiFiState enum)
static constexpr const char* kTopicNetWiFiState = "net.wifi.state";

/// WiFi SSID (string)
static constexpr const char* kTopicNetWiFiSSID = "net.wifi.ssid";

/// WiFi IP address (string)
static constexpr const char* kTopicNetWiFiIP = "net.wifi.ip";

/// WiFi signal strength (int: RSSI dBm)
static constexpr const char* kTopicNetWiFiRSSI = "net.wifi.rssi";

/// OTA state changes (int: OTAState enum)
static constexpr const char* kTopicNetOTAState = "net.ota.state";

/// OTA progress (int: percent 0-100)
static constexpr const char* kTopicNetOTAProgress = "net.ota.progress";

/// Request WiFi connect (payload: WiFiConfig*)
static constexpr const char* kTopicNetWiFiConnect = "net.wifi.connect";

/// Request WiFi disconnect
static constexpr const char* kTopicNetWiFiDisconnect = "net.wifi.disconnect";

/// Request OTA start (payload: URL string or nullptr for default)
static constexpr const char* kTopicNetOTAStart = "net.ota.start";

/// Request OTA abort
static constexpr const char* kTopicNetOTAAbort = "net.ota.abort";

/// HTTP server state changes (int: HttpServerState enum)
static constexpr const char* kTopicNetHttpState = "net.http.state";

/// Request HTTP server start (payload: port as int, or 0 for default 80)
static constexpr const char* kTopicNetHttpStart = "net.http.start";

/// Request HTTP server stop
static constexpr const char* kTopicNetHttpStop = "net.http.stop";

}  // namespace networking
