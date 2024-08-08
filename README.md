# CubeSat Zephyr software samples builder

Copyright (c) 2024 [Antmicro](https://www.antmicro.com)

This repository contains sources For Zephyr-based applications for three components of the
demonstration [CubeSat Edge Computing System](https://designer.antmicro.com/projects/cubesat_edge_computing_system)
as presented in Antmicro's [Interactive System Designer](https://designer.antmicro.com/welcome).

This software targets three components of the CubeSat system:

- [LEON3-based coprocessor](https://designer.antmicro.com/hardware/devices/cecs_leon3_coprocessor), responsible for managing solar panels
- [STM32G474-based OBC (On-Board Computer)](https://designer.antmicro.com/hardware/devices/cecs_on-board_computer) equipped with an UHF radio, a battery voltage monitor and a temperature sensor
- [STM32G474-based Processing Module Controller](https://designer.antmicro.com/hardware/devices/cecs_processing_module), responsible for waking up the [PolarFire SoM-based Processing Module](https://designer.antmicro.com/hardware/devices/polarfire-som) when meeting predefined environmental conditions

