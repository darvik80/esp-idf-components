//
// Created by Ivan Kishchenko on 21/02/2024.
//

#include "test_event_bus.h"

extern "C" {
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
}

#include <core/system/SystemService.h>
#include "core/EventBus.h"

#define BIT_RECEIVED	0x0001
#define BIT_DESTRUCTOR	0x0002

enum TestMessageId {
    MsgId_Trivial,
    MsgId_NonTrivial,
};

/* Declare a variable to hold the handle of the created event group. */
EventGroupHandle_t testEventHandle = xEventGroupCreate();
static FreeRTOSEventBus<32> eventBus({.stackSize = 4096, .name="debug"});

struct TrivialMessage : TMessage<MsgId_Trivial, Sys_User> {
    uint32_t status{};
};

struct NotTrivialMessage : CMessage<MsgId_NonTrivial, Sys_User> {
    std::string message{"Hello, world!"};
    ~ NotTrivialMessage() override {
        esp_logi(bus_test, "~NotTrivialMessage");
        xEventGroupSetBits(testEventHandle, BIT_DESTRUCTOR);
    }
};

class TestSubscriber : public TMessageSubscriber<TestSubscriber, TrivialMessage, NotTrivialMessage> {
public:
    void handle(const TrivialMessage &msg) {
        xEventGroupSetBits(testEventHandle, BIT_RECEIVED);
        TEST_ASSERT_EQUAL(msg.status, 1);
    }

    void handle(const NotTrivialMessage& msg) {
        xEventGroupSetBits(testEventHandle, BIT_RECEIVED);
        TEST_ASSERT_EQUAL_STRING(msg.message.c_str(), "Hello, world!");
    }
};

TEST_SETUP(event_bus) {
    auto sub = std::make_shared<TestSubscriber>();
    eventBus.subscribe(sub);
}

TEST_TEAR_DOWN(event_bus) {
}

TEST(event_bus, post_trivial) {
    xEventGroupClearBits(testEventHandle, BIT_RECEIVED);
    eventBus.post(TrivialMessage{.status=1});
    if (BIT_RECEIVED != xEventGroupWaitBits(testEventHandle, BIT_RECEIVED, true, true, pdMS_TO_TICKS(1000))) {
        TEST_FAIL();
    };
}

TEST(event_bus, post_non_trivial) {
    xEventGroupClearBits(testEventHandle, BIT_RECEIVED | BIT_DESTRUCTOR);
    eventBus.post(NotTrivialMessage{});
    if ((BIT_RECEIVED | BIT_DESTRUCTOR) !=
        xEventGroupWaitBits(testEventHandle, BIT_RECEIVED | BIT_DESTRUCTOR, true, true, pdMS_TO_TICKS(10000))) {
        TEST_FAIL();
    };

}

TEST_GROUP_RUNNER(event_bus) {
    RUN_TEST_CASE(event_bus, post_trivial);
    RUN_TEST_CASE(event_bus, post_non_trivial);
}
