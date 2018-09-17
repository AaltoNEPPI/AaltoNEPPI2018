
#include <stdio.h>

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "periph/pm.h"
#include "xtimer.h"
#include "neppi_power.h"

#include "nrf_soc.h"

/*
 * ADC values:
 *   At 14 bits resolution:
 *       > 14000 battery full, ~3.85V
 *       ~ 13400 battery almost empty, ~3.7V
 *       < 13000 battery empty, ~3.6V
 */
#define POWER_BATTERY_FULL_ADC  14000
#define POWER_BATTERY_LOW_ADC   13400
#define POWER_BATTERY_EMPTY_ADC 13000

/**
 * Hysteresis in reporting state changes, in ADC units
 */
#define HYSTERESIS 10

/**
 * Sliding averaging factor
 */
#define FACTOR 10

/**
 * Time to sleep between samples
 */
#define SLEEP_TIME 2

static kernel_pid_t reportee_pid;
static adc_t adc_line;

NORETURN static void *power_thread(void *arg)
{
    int prev_average;
    int sliding_average = adc_sample(adc_line, ADC_RES_14BIT);

    for (;;) {

#if 1
	extern int main_watchdog;
	if (main_watchdog++ > 100) {
	    DEBUG("Main thread blocked.  Resetting.\n");
	    pm_reboot();
	}
#endif

	const int adc = adc_sample(adc_line, ADC_RES_14BIT);
	sliding_average = (sliding_average * (FACTOR-1) + adc) / FACTOR;

	if (   sliding_average < prev_average - HYSTERESIS
	    || sliding_average > prev_average + HYSTERESIS) {

	    int type;
	    if (sliding_average < POWER_BATTERY_EMPTY_ADC) {
		type = MESSAGE_POWER_BATTERY_EMPTY;
	    } else if (sliding_average < POWER_BATTERY_LOW_ADC) {
		type = MESSAGE_POWER_BATTERY_LOW;
	    } else if (sliding_average > POWER_BATTERY_FULL_ADC) {
		type = MESSAGE_POWER_BATTERY_FULL;
	    } else {
		type = MESSAGE_POWER_BATTERY_VALUE;
	    }

	    msg_t m = {
		.type = type,
		.content = { .value = sliding_average },
	    };
	    msg_send(&m, reportee_pid);
	    prev_average = sliding_average;
	}
	xtimer_sleep(SLEEP_TIME);
    }
}

void neppi_power_init(kernel_pid_t pid, adc_t line) {
    reportee_pid = pid;
    adc_line = line;

    adc_init(adc_line);
}

void neppi_power_start(void)
{
    static char thread_stack[THREAD_STACKSIZE_DEFAULT*2/*XXX for now, due to SD*/];
    thread_create(thread_stack, sizeof(thread_stack),
                  THREAD_PRIORITY_MAIN + 2, THREAD_CREATE_WOUT_YIELD,
                  power_thread, NULL, "PWR");
    DEBUG("neppi_power: thread created\n");
}


NORETURN void neppi_power_shutdown(void) {
    sd_power_system_off();
    // If the SD power off didn't work, use the RIOT one.
    pm_off();
    /* Last resort */
    for (;;) {
	__DSB();
	__WFI();
	__NOP();
    }
}
