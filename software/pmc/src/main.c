/*
 * Copyright (c) 2016 ARM Ltd.
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 * Copyright (c) 2023 FTP Technologies
 * Copyright (c) 2023 Daniel DeGrasse <daniel@degrasse.com>
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#define LINUX_NAME "PolarFire SoM"

#define MIN_TEMPERATURE_THRESHOLD 10.0
#define MIN_VOLTAGE_THRESHOLD 2000

#define LINUX_TRIG_PIN 0

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(green_led), gpios);
static const struct device *linux_trigger_dev = DEVICE_DT_GET(DT_NODELABEL(gpioa));

static const struct device *const uart_volt = DEVICE_DT_GET(DT_NODELABEL(usart3));
static const struct device *const uart_temp = DEVICE_DT_GET(DT_NODELABEL(usart2));

#define MSG_SIZE 32
#define DATA_SIZE (MSG_SIZE - 1)

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

/* receive buffer used in UART ISR callback */
struct uart_buf {
	int pos;
	char buf[MSG_SIZE];
};

#define VOLT_MSG_ID 1
#define TEMP_MSG_ID 2

static struct uart_buf uart_volt_buf;
static struct uart_buf uart_temp_buf;

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *uart_dev, void *user_data)
{
	struct uart_buf *uart_dev_buf = (struct uart_buf*)user_data;
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		if ((c == '\n' || c == '\r') && uart_dev_buf->pos > 1) {
			/* terminate string */
			uart_dev_buf->buf[uart_dev_buf->pos] = '\0';

			/* if queue is full, message is silently dropped */
			k_msgq_put(&uart_msgq, &uart_dev_buf->buf, K_NO_WAIT);

			/* reset the buffer (it was copied to the msgq) */
			uart_dev_buf->pos = 1;
		} else if (uart_dev_buf->pos < (MSG_SIZE - 1)) {
			uart_dev_buf->buf[uart_dev_buf->pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

int init_uart(const struct device *const uart_dev, struct uart_buf *buf)
{
	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return -1;
	}

	buf->pos = 1;

	/* configure interrupt and callback to receive data */
	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, buf);

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
	uart_irq_rx_enable(uart_dev);

	return 0;
}

int main(void)
{
	char tx_buf[DATA_SIZE];
	int ret;

 	// configure linux trigger pin

	if (!device_is_ready(linux_trigger_dev)) {
		printf(LINUX_NAME " trigger pin is not ready\n");
		return 0;
	}

	ret = gpio_pin_set(linux_trigger_dev, LINUX_TRIG_PIN, 0);
	if (ret < 0) {
		printf("Failed to configure " LINUX_NAME " trigger pin\n");
		return 0;
	}

	// configure LED pin

	if (!gpio_is_ready_dt(&led)) {
		printf("LED is not ready\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printf("Failed to configure LED\n");
		return 0;
	}

	// configure voltage and temperature UART data channels

	uart_volt_buf.buf[0] = VOLT_MSG_ID;
	ret = init_uart(uart_volt, &uart_volt_buf);
	if (ret < 0) {
		printf("Failed to configure voltage data channel\n");
		return 0;
	}

	uart_temp_buf.buf[0] = TEMP_MSG_ID;
	ret = init_uart(uart_temp, &uart_temp_buf);
	if (ret < 0) {
		printf("Failed to configure temperature data channel\n");
		return 0;
	}

	// wait for trigger condition

	bool linux_started = false;
	double temperature = 0.0;
	long voltage = 0;

	/* indefinitely wait for input from the user */
	while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0) {
		switch(tx_buf[0])
		{
			case VOLT_MSG_ID:
				voltage = strtol(tx_buf + 1, NULL, 10);
				printf("voltage: %ldmV\n", voltage);
				break;
			case TEMP_MSG_ID:
				temperature = strtod(tx_buf + 1, NULL);
				printf("temperature: %0.1lf°C\n", temperature);
				break;
			default:
				printf("Internal message handling error\n");
				return 0;
		}

		bool startup_cond = temperature >= MIN_TEMPERATURE_THRESHOLD && voltage >= MIN_VOLTAGE_THRESHOLD;

		if(!linux_started && startup_cond)
		{
			// startup linux

			ret = gpio_pin_set(linux_trigger_dev, LINUX_TRIG_PIN, 1);
			if (ret < 0) {
				printf("Failed to toggle " LINUX_NAME " trigger pin\n");
			} else {
				printf(LINUX_NAME " started\n");
			}
			linux_started = true;
		}

		// set activity LED

		ret = gpio_pin_set_dt(&led, !startup_cond);
		if (ret < 0) {
			printf("Failed to set LED state\n");
		}
	}

	return 0;
}
