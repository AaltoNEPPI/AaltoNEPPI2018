
#include "thread.h"
#include "periph/adc.h"

#define MESSAGE_POWER_BATTERY_EMPTY  2000
#define MESSAGE_POWER_BATTERY_LOW    2001
#define MESSAGE_POWER_BATTERY_FULL   2002
#define MESSAGE_POWER_BATTERY_VALUE  2003

void neppi_power_init(kernel_pid_t pid, adc_t line);
void neppi_power_start(void);

NORETURN void neppi_power_shutdown(void); 

