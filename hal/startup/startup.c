#include "startup.h"

#include "configs/config.h"

#include "sysclk.h"
#include "wdt.h"
#include "ioport.h"
#include "bricklib2/logging/logging.h"


void startup_init(void) {
	// Initialize System clocks
	sysclk_init();

	// Initialize watchdog
	// Mode: Enable reset, halt watchdug on debug and idle
	uint32_t wdt_mode = WDT_MR_WDRSTEN | WDT_MR_WDDBGHLT  | WDT_MR_WDIDLEHLT;
	uint32_t timeout_value = wdt_get_timeout_value(WATCHDOG_PERIOD * 1000, BOARD_FREQ_SLCK_XTAL);
	wdt_init(WDT, wdt_mode, timeout_value, timeout_value);

	// Initialize interrupt vector table support.
	irq_initialize_vectors();

	// Enable interrupts
	cpu_irq_enable();

	// Initialize logging
	logging_init();

	// Initialize IO
	ioport_init();
}
