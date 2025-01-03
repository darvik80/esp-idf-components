//
// Created by Ivan Kishchenko on 15/02/2024.
//

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>
#include <string_view>
#include <esp_err.h>
#include "Logger.h"

struct TaskOptions {
    uint32_t stackSize = configMINIMAL_STACK_SIZE;
    size_t priority{tskIDLE_PRIORITY};
    std::string name;
};

typedef std::function<void()> TaskEntry;

class FreeRTOSTask {
    struct Context {
        TaskEntry entry;
    };

    TaskHandle_t _task{};
private:
    explicit FreeRTOSTask(TaskHandle_t task) : _task(task) {
    }

    static void task(void *arg) {
        auto task = static_cast<Context *>(arg);
        task->entry();
        delete task;
        vTaskDelete(nullptr);
    }

    static TaskHandle_t create(
            const TaskEntry &entry,
            std::string_view name,
            uint32_t stackDepth,
            UBaseType_t priority,
            BaseType_t core
    ) {
        TaskHandle_t handle{nullptr};

        ESP_ERROR_CHECK(xTaskCreatePinnedToCore(
                task, name.data(), stackDepth,
                new Context{.entry = entry }, priority, &handle, core) ? ESP_OK : ESP_FAIL
        );

        return handle;
    }
public:
    FreeRTOSTask() = default;

    FreeRTOSTask(FreeRTOSTask &) = delete;

    FreeRTOSTask(FreeRTOSTask &&other) {
        _task = other._task;
        other._task = nullptr;
    }

    FreeRTOSTask &operator=(FreeRTOSTask &&other) {
        if (this != &other) {
            _task = other._task;
            other._task = nullptr;
        }
        return *this;
    }

    [[maybe_unused]] static FreeRTOSTask submit(
            const TaskEntry &entry,
            std::string_view name = "task-submit",
            uint32_t stackDepth = configMINIMAL_STACK_SIZE,
            UBaseType_t priority = tskIDLE_PRIORITY,
            BaseType_t core = tskNO_AFFINITY
    ) {
        return FreeRTOSTask{create(entry, name, stackDepth, priority, core)};
    }

    static void execute(
            const TaskEntry &entry,
            std::string_view name = "task-execute",
            uint32_t stackDepth = configMINIMAL_STACK_SIZE,
            UBaseType_t priority = tskIDLE_PRIORITY,
            BaseType_t core = tskNO_AFFINITY
    ) {
        create(entry, name, stackDepth, priority, core);
    }

    void shutdown() {
        if (_task) {
            vTaskDelete(_task);
            _task = nullptr;
        }
    }

    ~FreeRTOSTask() {
        shutdown();
    }
};
