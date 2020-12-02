#include "bricklib2/bootloader/bootloader.h"

extern BootloaderFunctions bootloader_functions;
unsigned int __aeabi_uidiv(unsigned int a, unsigned int b) {
#ifdef BOOTLOADER_DIV_USED_IN_IRQ
	__disable_irq();
#endif

	const unsigned int res = bootloader_functions.__aeabi_uidiv(a, b);

#ifdef BOOTLOADER_DIV_USED_IN_IRQ
	__enable_irq();
#endif

	return res;
}
