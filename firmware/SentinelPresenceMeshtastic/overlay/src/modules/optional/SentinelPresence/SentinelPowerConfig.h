// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#ifndef SENTINEL_POWER_ENABLED
#define SENTINEL_POWER_ENABLED 0
#endif
// XIAO ESP32S3 D0/D1 are ADC1; keep motion D2/D3 and radio wiring intact.
#define SENTINEL_POWER_CURRENT_GPIO 1
#define SENTINEL_POWER_VOLTAGE_GPIO 2
#define SENTINEL_POWER_CHANNEL 0
#define SENTINEL_POWER_DC false
#define SENTINEL_POWER_MEASURE_VOLTAGE false
// Calibrate against reference instruments. Zero gains keep acquisition disabled.
#define SENTINEL_POWER_AMPS_PER_ADC_VOLT 0.0
#define SENTINEL_POWER_CURRENT_ZERO_V 0.0
#define SENTINEL_POWER_VOLTS_PER_ADC_VOLT 0.0
#define SENTINEL_POWER_NOMINAL_AC_VOLTS 0.0
#define SENTINEL_POWER_INTERVAL_MS 60000UL
