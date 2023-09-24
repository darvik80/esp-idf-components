//
// Created by Ivan Kishchenko on 07.11.2021.
//

#pragma once

#include <utility>
#include <vector>

#include "Properties.h"
#include "Core.h"
#include "EventBus.h"

typedef uint16_t ServiceId;
typedef uint8_t ServiceSubId;
typedef uint8_t SystemId;

class Registry;

class Service {
public:
    typedef std::shared_ptr<Service> Ptr;

    [[nodiscard]] virtual ServiceId getServiceId() const = 0;

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
    template<typename C, typename... T>
    C &create(T &&... all) {
        auto service = std::make_shared<C>(*this, std::forward<T>(all)...);
        return static_cast<C&>(*_services.emplace_back(service).get());
//
//        return *(service.get());
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

template<typename T, ServiceSubId Id, SystemId systemId = Sys_User>
class TService : public Service, public std::enable_shared_from_this<T>{
    Registry &_registry;
public:
    explicit TService(Registry &registry) : _registry(registry) {}

    enum {
        ID = Id | (((uint16_t) systemId) << 8)
    };

    [[nodiscard]] ServiceId getServiceId() const override {
        return Id | (((uint16_t) systemId) << 8);
    }

    Registry &getRegistry() override {
        return _registry;
    }

    DefaultEventBus& getBus() {
        return getDefaultEventBus();
    }

    void setup() override {
        if constexpr (std::is_base_of<EventSubscriber, T>::value) {
            esp_logi(tmpl, "subscribe service: %d", ID);
            getBus().subscribe(TService::shared_from_this());
        } else {
            esp_logi(tmpl, "not subscribed service: %d", ID);
        }
    }
};
