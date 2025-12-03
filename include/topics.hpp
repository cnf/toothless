#pragma once

#include <array>
#include <iostream>
#include <string_view>

static constexpr const int kMaxTopicLength = 64;
static constexpr const size_t kMaxPartLength = 15;

// Message Bus Topics
#define TOPIC_INC_PREFIX "inc"
#define TOPIC_SEND_PREFIX "send"
#define TOPIC_DOT "."  // Do NOT change this, the pubsub-c code expects a .
#define TOPIC_SET "set"
#define TOPIC_GET "get"
#define TOPIC_ERASE "erase"

static constexpr const char kTopicDelimiter[] = ".";
static constexpr const char kTopicVerbSet[] = "set";
static constexpr const char kTopicVerbGet[] = "get";
static constexpr const char kTopicVerbErase[] = "erase";
static constexpr const char kTopicVerbAdd[] = "add";
static constexpr const char kTopicVerbRemove[] = "remove";
static constexpr const char kTopicVerbShow[] = "show";
static constexpr const char kTopicVerbHide[] = "hide";
static constexpr const char kTopicVerbToggle[] = "toggle";
static constexpr const char kTopicVerbHome[] = "home";
static constexpr const char kTopicTransportStatus[] = "transport.status";
static constexpr const char kTopicTo[] = "to";
static constexpr const char kTopicErrors[] = "errors";
static constexpr const char kTopicReport[] = "report";
static constexpr const char kTopicMetrics[] = "metrics";
static constexpr const char kTopicEvents[] = "events";
static constexpr const char kTopicConfig[] = "config";
static constexpr const char kTopicConfigGet[] = "config.get";
static constexpr const char kTopicConfigSet[] = "config.set";
static constexpr const char kTopicIncConfig[] = "inc.config";
static constexpr const char kTopicRegister[] = "register";
static constexpr const char kTopicReset[] = "reset";
static constexpr const char kTopicProtocolRxRaw[] = "#.protocol.rxraw";
static constexpr const char kTopicProtocolTxRaw[] = "#.protocol.txraw";
static constexpr const char kTopicDisplay[] = "inc.display";
static constexpr const char kTopicDisplay_Space[] = "inc.display ";

// // Message Bus CTS Topics
// #include "compile_time_strings.hpp"
// static constexpr const char kTopicDot = "."_cts;
// static constexpr const char kTopicInc = "inc"_cts;
// static constexpr const char kTopicSend = "send"_cts;
// static constexpr const char kTopicSet = "set"_cts;
// static constexpr const char kTopicGet = "get"_cts;
// static constexpr const char kTopicsError = "error"_cts;
// static constexpr const char kTopicsReport = "report"_cts;
// static constexpr const char kTopicsMetrics = "metrics"_cts;
// static constexpr const char kTopicsEvents = "events"_cts;
// static constexpr const char kTopicsConfig = "config"_cts;
// static constexpr const char kTopicsRegister = "register"_cts;
// static constexpr const char kTopicsReset = "reset"_cts;
// // Compound Topics
// static constexpr char kTopicIncConfig = kTopicInc + kTopicDot + kTopicsConfig;
