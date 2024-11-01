//
// Created by Ivan Kishchenko on 07.11.2021.
//

#pragma once

#include <utility>
#include <vector>
#include <type_traits>

#include "EventBus.h"
#include "Properties.h"
#include "detail/NonCopyable.h"

typedef uint16_t ServiceId;
typedef uint8_t ServiceSubId;
typedef uint8_t SystemId;

class Registry;

class Service {
public:
    typedef std::shared_ptr<Service> Ptr;

    [[nodiscard]] virtual ServiceId getServiceId() const = 0;

    [[nodiscard]] virtual std::string_view getServiceName() const = 0;

    virtual Registry &getRegistry() = 0;

    virtual void setup() {}

    virtual void loop() {}

    virtual void destroy() {}

    virtual ~Service() = default;
};

typedef std::vector<Service::Ptr> ServiceArray;

class Registry {
    ServiceArray _services;
    PropertiesLoader _propsLoader;
private:
    Service *doGetService(ServiceId serviceId) {
        for (auto &service: getServices()) {
            if (service->getServiceId() == serviceId) {
                return service.get();
            }
        }

        return nullptr;
    }

public:
    static Registry& getInstance() {
        static Registry instance;
        return instance;
    }

    template<typename C, typename... T>
    C &create(T &&... all) {
        static_assert(std::is_base_of_v<Service, C>, "C must be derived from Service");
        static_assert(!std::is_default_constructible_v<C>, "C must not have default constructor");
        static_assert(!std::is_copy_constructible_v<C>, "C must not have copy constructor");

        auto service = std::make_shared<C>(*this, std::forward<T>(all)...);
        if constexpr (std::is_base_of<MessageSubscriber, C>::value) {
            esp_logi(tmpl, "subscribe service: 0x%04x:%s", C::ID, service->getServiceName().data());
            getEventBus().subscribe(service->shared_from_this());
        } else {
            esp_logi(tmpl, "not subscribed service: 0x%04x:%s", C::ID, service->getServiceName().data());
        }

        return static_cast<C &>(*_services.emplace_back(service).get());
    }

    template<typename C>
    C *getService() {
        return static_cast<C *>(doGetService(C::ID));
    }

    ServiceArray &getServices() {
        return _services;
    }

    PropertiesLoader &getPropsLoader() {
        return _propsLoader;
    }

    DefaultEventBus &getEventBus() {
        return getDefaultEventBus();
    }
};

template<typename T, ServiceSubId Id, SystemId systemId>
class TService : public Service, public std::enable_shared_from_this<T>, public detail::NonCopyable {
    Registry &_registry;
public:
    explicit TService(Registry &registry) : _registry(registry) {}

    enum {
        ID = Id | (((uint16_t) systemId) << 8)
    };

    [[nodiscard]] ServiceId getServiceId() const override {
        return ID;
    }

    [[nodiscard]] std::string_view getServiceName() const override {
        return "unknown";
    }

    Registry &getRegistry() override {
        return _registry;
    }

    DefaultEventBus &getBus() {
        return getDefaultEventBus();
    }
};
