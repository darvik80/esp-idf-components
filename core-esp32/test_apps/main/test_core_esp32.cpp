/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 *
 * This test code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */

extern "C" {
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include "unity_fixture.h"
#include "unity_fixture_extras.h"
#include "test_utils.h"
#include "memory_checks.h"
}

#include <freertos/event_groups.h>
#include "core/EventBus.h"


#define BIT_RECEIVED	0x0001

/* Declare a variable to hold the handle of the created event group. */
EventGroupHandle_t testEventHandle = xEventGroupCreate();
static FreeRTOSEventBus<32> eventBus({.stackSize = 2048, .name="debug"});

struct TestMessage : TMessage<1, 1> {
    uint32_t status{};
};

class TestSubscriber : public TMessageSubscriber<TestSubscriber, TestMessage> {
public:
    void handle(const TestMessage &msg) {
        esp_logi(bus_test, "handle: %.04X:%d", msg.ID, msg.status);
        TEST_ASSERT_EQUAL(msg.status, 1);
        xEventGroupSetBits(testEventHandle, BIT_RECEIVED);
    }
};

TEST_GROUP(core_esp32);

TEST_SETUP(core_esp32) {
    auto sub = std::make_shared<TestSubscriber>();
    eventBus.subscribe(sub);
}

TEST_TEAR_DOWN(core_esp32) {
}

TEST(core_esp32, post_simple) {
    eventBus.post(TestMessage{.status=1});
    if (BIT_RECEIVED != xEventGroupWaitBits(testEventHandle, BIT_RECEIVED, true, true, pdMS_TO_TICKS(1000))) {
        TEST_FAIL();
    };

    xEventGroupClearBits(testEventHandle, BIT_RECEIVED);
}

TEST_GROUP_RUNNER(core_esp32) {
    RUN_TEST_CASE(core_esp32, post_simple);
}

extern "C" void app_main(void) {
    UNITY_MAIN(core_esp32);
}
