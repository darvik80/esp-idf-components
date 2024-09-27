//
// Created by Ivan Kishchenko on 29/8/24.
//

#pragma once

#include <sdkconfig.h>
#ifdef CONFIG_EXCHANGE_BUS_SPI

#include <hal/spi_types.h>
#include <soc/gpio_num.h>

constexpr gpio_num_t pinHandshake{static_cast<gpio_num_t>(CONFIG_EXCHANGE_BUS_SPI_HANDSHAKE_PIN)};
constexpr gpio_num_t pinDataReady{static_cast<gpio_num_t>(CONFIG_EXCHANGE_BUS_SPI_DATA_READY_PIN)};
constexpr gpio_num_t pinCS{static_cast<gpio_num_t>(CONFIG_EXCHANGE_BUS_SPI_CS_PIN)};
constexpr gpio_num_t pinMOSI{static_cast<gpio_num_t>(CONFIG_EXCHANGE_BUS_SPI_MOSI_PIN)};
constexpr gpio_num_t pinCLK{static_cast<gpio_num_t>(CONFIG_EXCHANGE_BUS_SPI_CLK_PIN)};

constexpr spi_host_device_t spi = static_cast<spi_host_device_t>(CONFIG_EXCHANGE_BUS_SPI_CHANNEL);

#define SPI_TASK_DEFAULT_STACK_SIZE	 4096
#define SPI_TASK_DEFAULT_PRIO        22
#define SPI_BITS_PER_WORD			8

#endif
