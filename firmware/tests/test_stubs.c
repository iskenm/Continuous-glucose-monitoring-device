/**
 * @file test_stubs.c
 * @brief Hardware abstraction stubs implementation
 */

#include "test_stubs.h"

uint16_t test_stub_adc_value = 2048;    /* Mid-range ADC */
float    test_stub_temperature = 37.0f; /* Normal body temp */
uint32_t test_stub_impedance = 10000;   /* 10k ohm (normal) */
uint32_t test_stub_uptime_ms = 0;
uint16_t test_stub_battery_mv = 3000;   /* Full battery */

void test_stubs_reset(void)
{
    test_stub_adc_value = 2048;
    test_stub_temperature = 37.0f;
    test_stub_impedance = 10000;
    test_stub_uptime_ms = 0;
    test_stub_battery_mv = 3000;
}
