//
// Created by Ivan Kishchenko on 04/09/2023.
//

#include "EventBus.h"

namespace {
    class bus_category : public std::error_category {
    public:
        [[nodiscard]] const char *name() const noexcept override {
            return "bus";
        }

        [[nodiscard]] std::string message(int ev) const override {
            switch ((bus_error) ev) {
                case bus_error::ok:
                    return "ok";
                case bus_error::fail:
                    return "fail";
                case bus_error::invalid_state:
                    return "invalid state";
                case bus_error::invalid_arguments:
                    return "invalid arguments";
                case bus_error::out_of_memory:
                    return "not enough memory";
                case bus_error::timeout:
                    return "operation timeout";
                default:
                    return "(unrecognized error)";
            }
        }

        [[nodiscard]] std::error_condition default_error_condition(int err_value) const noexcept override {
            bus_condition result;
            switch (static_cast<bus_error>(err_value)) {
                case bus_error::ok:
                    result = bus_condition::success;
                    break;
                default:
                    result = bus_condition::fail;

            }
            return result;
        }

        [[nodiscard]] bool equivalent(const std::error_code &err_code, int err_value) const noexcept override {
            return *this == err_code.category() &&
                   static_cast<int>(default_error_condition(err_code.value()).value()) == err_value;
        }

        [[nodiscard]] bool equivalent(int err_value, const std::error_condition &err_cond) const noexcept override {
            return default_error_condition(err_value) == err_cond;
        }
    };

    const bus_category bus_err_category{};
}

std::error_code make_error_code(bus_error e) {
    return {static_cast<int>(e), bus_err_category};
}

std::error_code make_error_code(esp_err_t e) {
    bus_error err;
    switch (e) {
        case ESP_OK:
            err = bus_error::ok;
            break;
        case ESP_ERR_INVALID_STATE:
            err = bus_error::invalid_state;
            break;
        case ESP_ERR_INVALID_ARG:
            err = bus_error::invalid_arguments;
            break;
        case ESP_ERR_NO_MEM:
            err = bus_error::out_of_memory;
            break;
        case ESP_ERR_TIMEOUT:
            err = bus_error::timeout;
            break;
        default:
            err = bus_error::fail;
            break;
    }

    return make_error_code(err);
}

inline std::error_condition make_error_condition(bus_condition cond) noexcept {
    return {static_cast<int>(cond), bus_err_category};
}

ESP_EVENT_DEFINE_BASE(CORE_EVENT);

DefaultEventBus &getDefaultEventBus() {
    static DefaultEventBus bus{
#ifdef CONFIG_BUS_FREE_RTOS_ENABLED
#ifdef CONFIG_BUS_FREE_RTOS_QUEUE_SIZE
            withQueueSize(CONFIG_BUS_FREE_RTOS_QUEUE_SIZE),
#endif
#ifdef CONFIG_BUS_FREE_RTOS_TASK_STACK_SIZE
            withStackSize(CONFIG_BUS_FREE_RTOS_TASK_STACK_SIZE),
#endif
#endif
#ifdef CONFIG_BUS_ESP_EVENT_LOOP_ENABLED
#ifdef CONFIG_ESP_EVENT_QUEUE_SIZE
            withQueueSize(CONFIG_ESP_EVENT_QUEUE_SIZE),
#endif
#ifdef CONFIG_ESP_EVENT_TASK_STACK_SIZE
            withStackSize(CONFIG_ESP_EVENT_TASK_STACK_SIZE),
#endif
#endif
            withName("main")
    };


    return bus;
}

BusOption withSystemQueue(bool useSystemQueue) {
    return [useSystemQueue](BusOptions &options) {
        options.name = "sys_evt";
        options.useSystemQueue = useSystemQueue;
    };
}

BusOption withQueueSize(int32_t queueSize) {
    return [queueSize](BusOptions &options) {
        options.queueSize = queueSize;
    };
}

BusOption withStackSize(size_t stackSize) {
    return [stackSize](BusOptions &options) {
        options.stackSize = stackSize;
    };
}

BusOption withPrioritySize(size_t priority) {
    return [priority](BusOptions &options) {
        options.priority = priority;
    };
}

BusOption withName(std::string_view name) {
    return [name](BusOptions &options) {
        if (!options.useSystemQueue) {
            options.name = name;
        } // else ignore name
    };
}

