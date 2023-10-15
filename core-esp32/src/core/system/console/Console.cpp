//
// Created by Ivan Kishchenko on 21/09/2023.
//

#include <esp_console.h>
#include <argtable3/argtable3.h>

#include "Console.h"

typedef struct {
    struct arg_str *subCmd;
    struct arg_end *end;
} wifi_args_t;

static wifi_args_t wifi_args;

static int wifiScan(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &wifi_args);

    if (nerrors != 0) {
        arg_print_errors(stderr, wifi_args.end, argv[0]);
        return 1;
    }

    if (wifi_args.subCmd->count == 1) {
        Command cmd;
        strcpy(cmd.cmd, "wifi");
        strncpy(cmd.params, wifi_args.subCmd->sval[0], 16);
        getDefaultEventBus().post(cmd);
    }

    return 0;
}

static void registerWifi() {
    wifi_args.subCmd = arg_str0(nullptr, nullptr, "<sub operation>", "scan, connect, ...");
    wifi_args.end = arg_end(1);
    const esp_console_cmd_t cmd = {
            .command = "wifi",
            .help = "Basic wifi operations",
            .hint = nullptr,
            .func = &wifiScan,
            .argtable = &wifi_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int restart(int argc, char **argv) {
    printf("restarting...");
    esp_restart();
}

static void registerRestart() {
    const esp_console_cmd_t cmd = {
            .command = "restart",
            .help = "Restart the program",
            .hint = nullptr,
            .func = &restart,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/** 'free' command prints available heap memory */

static int freeMem(int argc, char **argv) {
    printf("free heap:%" PRIu32 "\n", esp_get_free_heap_size());
    return 0;
}

static void registerFree(void) {
    const esp_console_cmd_t cmd = {
            .command = "free",
            .help = "Get the total size of heap memory available",
            .hint = nullptr,
            .func = &freeMem,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static void registerSystem() {
    esp_console_register_help_command();
    registerRestart();
    registerFree();
}

UartConsoleService::UartConsoleService(Registry &registry) : TService(registry) {}

void UartConsoleService::setup() {
    esp_console_repl_t *repl = nullptr;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

    repl_config.prompt = CONFIG_IDF_TARGET ">";
    repl_config.max_cmdline_length = 256;
    repl_config.history_save_path = "/storage/history.txt";

    registerSystem();
    registerWifi();

    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    esp_logi(uart, "console started");
}
