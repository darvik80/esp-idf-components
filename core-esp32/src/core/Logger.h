//
// Created by Ivan Kishchenko on 07/08/2023.
//

#pragma once

#ifndef APP_LOG_LEVEL
#define APP_LOG_LEVEL 5
#endif

#define CONFIG_LOG_COLORS 1

#ifndef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL 5
#endif

#include <esp_log.h>

#define log_format(letter, format)  LOG_COLOR_ ## letter "[" #letter "] " LOG_RESET_COLOR "[%08" PRIu32 "] \033[0;34m[%6s]" LOG_RESET_COLOR ": " format LOG_RESET_COLOR "\n"

#define esp_log_level(level, tag, format, ...) do {                     \
        if (level==ESP_LOG_ERROR )          { esp_log_write(ESP_LOG_ERROR,      tag, log_format(E, format), esp_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==ESP_LOG_WARN )      { esp_log_write(ESP_LOG_WARN,       tag, log_format(W, format), esp_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==ESP_LOG_DEBUG )     { esp_log_write(ESP_LOG_DEBUG,      tag, log_format(D, format), esp_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==ESP_LOG_VERBOSE )   { esp_log_write(ESP_LOG_VERBOSE,    tag, log_format(V, format), esp_log_timestamp(), tag, ##__VA_ARGS__); } \
        else                                { esp_log_write(ESP_LOG_INFO,       tag, log_format(I, format), esp_log_timestamp(), tag, ##__VA_ARGS__); } \
    } while(0)

#define esp_log_level_local(level, tag, format, ...) do {               \
        if ( LOG_LOCAL_LEVEL >= level ) esp_log_level(level, tag, format, ##__VA_ARGS__); \
    } while(0)

#if APP_LOG_LEVEL >= 1
#define esp_loge(tag, format, ...) esp_log_level_local(ESP_LOG_ERROR,   #tag, format, ##__VA_ARGS__)
#else
#define esp_loge(tag, format, ...)
#endif

#if APP_LOG_LEVEL >= 2
#define esp_logw(tag, format, ...) esp_log_level_local(ESP_LOG_WARN,    #tag, format, ##__VA_ARGS__)
#else
#define esp_logw(tag, format, ...)
#endif


#if APP_LOG_LEVEL >= 3
#define esp_logi(tag, format, ...) esp_log_level_local(ESP_LOG_INFO,    #tag, format, ##__VA_ARGS__)
#else
#define esp_logi(tag, format, ...)
#endif

#if APP_LOG_LEVEL >= 4
#define esp_logd(tag, format, ...) esp_log_level_local(ESP_LOG_DEBUG,   #tag, format, ##__VA_ARGS__)
#else
#define esp_logd(tag, format, ...)
#endif

#if APP_LOG_LEVEL >= 5
#define esp_logv(tag, format, ...) esp_log_level_local(ESP_LOG_VERBOSE, #tag, format, ##__VA_ARGS__)
#else
#define esp_logv(tag, format, ...)
#endif

#include <fmt/core.h>

#define log_format_fmt(letter, format)  LOG_COLOR_ ## letter "[" #letter "] " LOG_RESET_COLOR "[{:#08}] \033[0;34m[{:>6}]" LOG_RESET_COLOR ": " format LOG_RESET_COLOR "\n"

#define esp_log_level_fmt(level, tag, strf, ...) do {                     \
        if (level==ESP_LOG_ERROR )          { esp_log_writev(ESP_LOG_ERROR,      tag, fmt::format(log_format_fmt(E, strf), esp_log_timestamp(), tag, ##__VA_ARGS__).c_str(), va_list{}); } \
        else if (level==ESP_LOG_WARN )      { esp_log_writev(ESP_LOG_WARN,       tag, fmt::format(log_format_fmt(W, strf), esp_log_timestamp(), tag, ##__VA_ARGS__).c_str(), va_list{}); } \
        else if (level==ESP_LOG_DEBUG )     { esp_log_writev(ESP_LOG_DEBUG,      tag, fmt::format(log_format_fmt(D, strf), esp_log_timestamp(), tag, ##__VA_ARGS__).c_str(), va_list{}); } \
        else if (level==ESP_LOG_VERBOSE )   { esp_log_writev(ESP_LOG_VERBOSE,    tag, fmt::format(log_format_fmt(V, strf), esp_log_timestamp(), tag, ##__VA_ARGS__).c_str(), va_list{}); } \
        else                                { esp_log_writev(ESP_LOG_INFO,       tag, fmt::format(log_format_fmt(I, strf), esp_log_timestamp(), tag, ##__VA_ARGS__).c_str(), va_list{}); } \
    } while(0)

#define esp_log_level_local_fmt(level, tag, format, ...) do {               \
        if ( LOG_LOCAL_LEVEL >= level ) esp_log_level_fmt(level, tag, format, ##__VA_ARGS__); \
    } while(0)

#if APP_LOG_LEVEL >= 1
#define core_loge(tag, format, ...) esp_log_level_local_fmt(ESP_LOG_ERROR,   #tag, format, ##__VA_ARGS__)
#else
#define esp_loge(tag, format, ...)
#endif

#if APP_LOG_LEVEL >= 2
#define core_logw(tag, format, ...) esp_log_level_local_fmt(ESP_LOG_WARN,    #tag, format, ##__VA_ARGS__)
#else
#define esp_logw(tag, format, ...)
#endif


#if APP_LOG_LEVEL >= 3
#define core_logi(tag, format, ...) esp_log_level_local_fmt(ESP_LOG_INFO,    #tag, format, ##__VA_ARGS__)
#else
#define esp_logi(tag, format, ...)
#endif

#if APP_LOG_LEVEL >= 4
#define core_logd(tag, format, ...) esp_log_level_local_fmt(ESP_LOG_DEBUG,   #tag, format, ##__VA_ARGS__)
#else
#define esp_logd(tag, format, ...)
#endif

#if APP_LOG_LEVEL >= 5
#define core_logv(tag, format, ...) esp_log_level_local_fmt(ESP_LOG_VERBOSE, #tag, format, ##__VA_ARGS__)
#else
#define esp_logv(tag, format, ...)
#endif
