/*
 * Copyright (c) 2016 ARM Ltd.
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 * Copyright (c) 2023 FTP Technologies
 * Copyright (c) 2023 Daniel DeGrasse <daniel@degrasse.com>
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#define MAX_TEMPERATURE_THRESHOLD 40.0

#define SENSOR_NAME "Voltage Monitor"

#define TX_SIZE 32

static const struct device *const uart_in = DEVICE_DT_GET(DT_NODELABEL(uart1));
static const struct device *const uart_out = DEVICE_DT_GET(DT_NODELABEL(uart2));

volatile long voltage;
static long voltage_buf = 0;
static int voltage_pos = 0;

void report_sample(long sample)
{
	int i = 0;
	char buf[TX_SIZE];
	snprintf(buf, TX_SIZE, "%ld\n", sample);

	while(buf[i] != '\0'){
		uart_poll_out(uart_out, buf[i++]);
	}
}

void sensor_cb(const struct device *uart_dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		voltage_buf <<= 8;
		voltage_buf |= c;
		if (++voltage_pos >= sizeof(voltage)) {
			voltage = voltage_buf;
			voltage_pos = 0;
		}
	}
}


int main(void)
{
	int ret;

	// configure temperature UART data channel

	if (!device_is_ready(uart_out)) {
		printf("UART is not ready\n");
		return 0;
	}

	// configure voltage monitor

	if (!device_is_ready(uart_in)) {
		printf(SENSOR_NAME " is not ready\n");
		return 0;
	}

	/* configure interrupt and callback to receive data */
	ret = uart_irq_callback_user_data_set(uart_in, sensor_cb, NULL);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk("UART device does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
		return -1;
	}
	uart_irq_rx_enable(uart_in);

	// transfer loop

	for (int i = 0;; ++i) {
		long sample = voltage;
		report_sample(sample);
		printf(SENSOR_NAME ": %ldmV\n", sample);

		k_sleep(K_MSEC(500));
	}

	return 0;
}
