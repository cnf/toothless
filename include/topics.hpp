#pragma once

#include <array>
#include <iostream>
#include <string_view>

static constexpr const int kMaxTopicLength = 64;
static constexpr const size_t kMaxPartLength = 15;

// Message Bus Topics
#define TOPIC_DOT "."  // Do NOT change this, the pubsub-c code expects a .
#define TOPIC_VERB_SET "set"
#define TOPIC_VERB_GET "get"
#define TOPIC_VERB_ERASE "erase"
#define TOPIC_VERB_ADD "add"
#define TOPIC_VERB_REMOVE "remove"
#define TOPIC_VERB_SHOW "show"
#define TOPIC_VERB_HIDE "hide"
#define TOPIC_VERB_TOGGLE "toggle"
#define TOPIC_VERB_RESET "reset"

// static constexpr const char kTopicDelimiter[] = ".";
// static constexpr const char kTopicVerbSet[] = "set";
// static constexpr const char kTopicVerbGet[] = "get";
// static constexpr const char kTopicVerbErase[] = "erase";
// static constexpr const char kTopicVerbAdd[] = "add";
// static constexpr const char kTopicVerbRemove[] = "remove";
// static constexpr const char kTopicVerbShow[] = "show";
// static constexpr const char kTopicVerbHide[] = "hide";
// static constexpr const char kTopicVerbToggle[] = "toggle";

static constexpr const char kTopicReport[] = "report";

static constexpr const char kTopicStatus[] = "status";
static constexpr const char kTopicStatusWarning[] = "status.warn";
static constexpr const char kTopicStatusInfo[] = "status.info";
static constexpr const char kTopicStatusError[] = "status.error";

static constexpr const char kTopicConfig[] = "config";
static constexpr const char kTopicConfigGet[] = "config" TOPIC_DOT TOPIC_VERB_GET;
static constexpr const char kTopicConfigSet[] = "config" TOPIC_DOT TOPIC_VERB_SET;
static constexpr const char kTopicRegister[] = "register";
static constexpr const char kTopicReset[] = "reset";
