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

#include <core/Application.h>
#include <core/system/System.h>

extern "C" {
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include "unity_fixture.h"
#include "unity_fixture_extras.h"
#include "test_utils.h"
#include "memory_checks.h"
}

#define APP_BIT_RECEIVED	0x0001

EventGroupHandle_t testAppEventHandle = xEventGroupCreate();

class TestApp : public Application<TestApp>, public TMessageSubscriber<TestApp, TimerEvent<1>> {
    FreeRTOSTimer _timer;
public:
    void userSetup() override {
        getRegistry().getEventBus().subscribe(shared_from_this());
        _timer.fire<1>(1000, false);
    }

    void handle(const TimerEvent<1>& event) {
        xEventGroupSetBits(testAppEventHandle, APP_BIT_RECEIVED);
    }
};

TEST_GROUP(core_esp32);

TEST_SETUP(core_esp32) {
}

TEST_TEAR_DOWN(core_esp32) {
}

TEST(core_esp32, simple) {
    auto app = std::make_shared<TestApp>();
    app->setup();
    if (APP_BIT_RECEIVED != xEventGroupWaitBits(testAppEventHandle, APP_BIT_RECEIVED, true, true, pdMS_TO_TICKS(10000))) {
        TEST_FAIL();
    };
}

TEST_GROUP_RUNNER(core_esp32) {
    RUN_TEST_CASE(core_esp32, simple);
}

static void RunAllTests() {
    RUN_TEST_GROUP(core_esp32);
    RUN_TEST_GROUP(event_bus);
}

extern "C" void app_main(void) {
    UNITY_MAIN_FUNC(RunAllTests);
}
