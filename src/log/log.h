#ifndef _LOG_H_
#define _LOG_H_

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "spdlog/common.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

using namespace std::chrono;

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO

// SPDLOG_TRACE(...)
// SPDLOG_DEBUG(...)
// SPDLOG_INFO(...)
// SPDLOG_WARN(...)
// SPDLOG_ERROR(...)
// SPDLOG_CRITICAL(...)

#ifdef SIGNAL_LOGGER
constexpr auto LOGGER_NAME = "siganl_server";
#else
constexpr auto LOGGER_NAME = "remote_desk";
#endif

#define LOG_INFO(...)                                                         \
  if (nullptr == spdlog::get(LOGGER_NAME)) {                                  \
    auto now = std::chrono::system_clock::now() + std::chrono::hours(8);      \
    auto timet = std::chrono::system_clock::to_time_t(now);                   \
    auto localTime = *std::gmtime(&timet);                                    \
    std::stringstream ss;                                                     \
    std::string filename;                                                     \
    ss << LOGGER_NAME;                                                        \
    ss << std::put_time(&localTime, "-%Y%m%d-%H%M%S.log");                    \
    ss >> filename;                                                           \
    std::string path = "logs/" + filename;                                    \
    std::vector<spdlog::sink_ptr> sinks;                                      \
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>()); \
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(   \
        path, 1048576 * 5, 3));                                               \
    auto combined_logger = std::make_shared<spdlog::logger>(                  \
        LOGGER_NAME, begin(sinks), end(sinks));                               \
    combined_logger->flush_on(spdlog::level::info);                           \
    spdlog::register_logger(combined_logger);                                 \
    SPDLOG_LOGGER_INFO(combined_logger, __VA_ARGS__);                         \
  } else {                                                                    \
    SPDLOG_LOGGER_INFO(spdlog::get(LOGGER_NAME), __VA_ARGS__);                \
  }

#define LOG_WARN(...)                                                         \
  if (nullptr == spdlog::get(LOGGER_NAME)) {                                  \
    auto now = std::chrono::system_clock::now() + std::chrono::hours(8);      \
    auto timet = std::chrono::system_clock::to_time_t(now);                   \
    auto localTime = *std::gmtime(&timet);                                    \
    std::stringstream ss;                                                     \
    std::string filename;                                                     \
    ss << LOGGER_NAME;                                                        \
    ss << std::put_time(&localTime, "-%Y%m%d-%H%M%S.log");                    \
    ss >> filename;                                                           \
    std::string path = "logs/" + filename;                                    \
    std::vector<spdlog::sink_ptr> sinks;                                      \
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>()); \
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(   \
        path, 1048576 * 5, 3));                                               \
    auto combined_logger = std::make_shared<spdlog::logger>(                  \
        LOGGER_NAME, begin(sinks), end(sinks));                               \
    spdlog::register_logger(combined_logger);                                 \
    SPDLOG_LOGGER_WARN(combined_logger, __VA_ARGS__);                         \
  } else {                                                                    \
    SPDLOG_LOGGER_WARN(spdlog::get(LOGGER_NAME), __VA_ARGS__);                \
  }

#define LOG_ERROR(...)                                                        \
  if (nullptr == spdlog::get(LOGGER_NAME)) {                                  \
    auto now = std::chrono::system_clock::now() + std::chrono::hours(8);      \
    auto timet = std::chrono::system_clock::to_time_t(now);                   \
    auto localTime = *std::gmtime(&timet);                                    \
    std::stringstream ss;                                                     \
    std::string filename;                                                     \
    ss << LOGGER_NAME;                                                        \
    ss << std::put_time(&localTime, "-%Y%m%d-%H%M%S.log");                    \
    ss >> filename;                                                           \
    std::string path = "logs/" + filename;                                    \
    std::vector<spdlog::sink_ptr> sinks;                                      \
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>()); \
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(   \
        path, 1048576 * 5, 3));                                               \
    auto combined_logger = std::make_shared<spdlog::logger>(                  \
        LOGGER_NAME, begin(sinks), end(sinks));                               \
    spdlog::register_logger(combined_logger);                                 \
    SPDLOG_LOGGER_ERROR(combined_logger, __VA_ARGS__);                        \
  } else {                                                                    \
    SPDLOG_LOGGER_ERROR(spdlog::get(LOGGER_NAME), __VA_ARGS__);               \
  }

#define LOG_FATAL(...)                                                        \
  if (nullptr == spdlog::get(LOGGER_NAME)) {                                  \
    auto now = std::chrono::system_clock::now() + std::chrono::hours(8);      \
    auto timet = std::chrono::system_clock::to_time_t(now);                   \
    auto localTime = *std::gmtime(&timet);                                    \
    std::stringstream ss;                                                     \
    std::string filename;                                                     \
    ss << LOGGER_NAME;                                                        \
    ss << std::put_time(&localTime, "-%Y%m%d-%H%M%S.log");                    \
    ss >> filename;                                                           \
    std::string path = "logs/" + filename;                                    \
    std::vector<spdlog::sink_ptr> sinks;                                      \
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>()); \
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(   \
        path, 1048576 * 5, 3));                                               \
    auto combined_logger = std::make_shared<spdlog::logger>(                  \
        LOGGER_NAME, begin(sinks), end(sinks));                               \
    spdlog::register_logger(combined_logger);                                 \
    SPDLOG_LOGGER_CRITICAL(combined_logger, __VA_ARGS__);                     \
  } else {                                                                    \
    SPDLOG_LOGGER_CRITICAL(spdlog::get(LOGGER_NAME), __VA_ARGS__);            \
  }

#endif
