/*
 * Copyright (c) 2016 ARM Ltd.
 * Copyright (c) 2023 FTP Technologies
 * Copyright (c) 2023 Daniel DeGrasse <daniel@degrasse.com>
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>

#define MAX_TEMPERATURE_THRESHOLD 40.0

#define TX_SIZE 32

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(green_led), gpios);

static const struct device *const uart_temp = DEVICE_DT_GET(DT_NODELABEL(usart3));
static const struct device *const temp_sensor = DEVICE_DT_GET(DT_NODELABEL(temp));

void report_sample(double temp)
{
	int i = 0;
	char buf[TX_SIZE];
	snprintf(buf, TX_SIZE, "%lf\n", temp);

	while(buf[i] != '\0'){
		uart_poll_out(uart_temp, buf[i++]);
	};
}

int read_temperature(const struct device *dev, struct sensor_value *val)
{
	int ret;

	ret = sensor_sample_fetch_chan(dev, SENSOR_CHAN_AMBIENT_TEMP);
	if (ret < 0) {
		printf("Could not fetch temperature: %d\n", ret);
		return ret;
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, val);
	if (ret < 0) {
		printf("Could not get temperature: %d\n", ret);
	}
	return ret;
}

int main(void)
{
	int ret;
	struct sensor_value value;

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

	// configure temperature UART data channel

	if (!device_is_ready(uart_temp)) {
		printf("UART is not ready\n");
		return 0;
	}

	// configure temperature sensor

	if (!device_is_ready(temp_sensor)) {
		printf("Device %s is not ready\n", temp_sensor->name);
		return 0;
	}

	// sampling loop

	bool led_state = false;
	double temperature = 0.0;

	for (int i = 0;; ++i) {
		ret = read_temperature(temp_sensor, &value);
		if (ret != 0) {
			printf("Failed to read temperature: %d\n", ret);
			break;
		}
		temperature = sensor_value_to_double(&value);
		report_sample(temperature);
		printf("%s: %0.1lf°C\n", temp_sensor->name, temperature);

		led_state = !led_state;
		ret = gpio_pin_set_dt(&led, led_state || temperature >= MAX_TEMPERATURE_THRESHOLD);
		if (ret < 0) {
			printf("Failed to set LED state\n");
		}

		k_sleep(K_MSEC(1000));
	}

	return 0;
}
