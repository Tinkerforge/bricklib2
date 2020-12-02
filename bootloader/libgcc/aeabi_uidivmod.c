#include "bricklib2/bootloader/bootloader.h"

extern BootloaderFunctions bootloader_functions;
uint64_t __aeabi_uidivmod(unsigned int a, unsigned int b) {
#ifdef BOOTLOADER_DIV_USED_IN_IRQ
	__disable_irq();
#endif

	const uint64_t res = bootloader_functions.__aeabi_uidivmod(a, b);

#ifdef BOOTLOADER_DIV_USED_IN_IRQ
	__enable_irq();
#endif

	return res;
}
