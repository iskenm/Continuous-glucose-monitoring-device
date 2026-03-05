/**
 * @file test_stubs.h
 * @brief Hardware abstraction stubs for unit testing
 *
 * Provides controllable mock values for hardware peripherals so that
 * firmware logic can be tested on the host without target hardware.
 */

#ifndef TEST_STUBS_H
#define TEST_STUBS_H

#include <stdint.h>

/* Controllable test values */
extern uint16_t test_stub_adc_value;
extern float    test_stub_temperature;
extern uint32_t test_stub_impedance;
extern uint32_t test_stub_uptime_ms;
extern uint16_t test_stub_battery_mv;

/* Reset all stubs to default values */
void test_stubs_reset(void);

#endif /* TEST_STUBS_H */
