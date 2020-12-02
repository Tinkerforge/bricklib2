#include "bricklib2/bootloader/bootloader.h"

extern BootloaderFunctions bootloader_functions;
int __aeabi_idiv(int a, int b) {
#ifdef BOOTLOADER_DIV_USED_IN_IRQ
	__disable_irq();
#endif

	const int res = bootloader_functions.__aeabi_idiv(a, b);

#ifdef BOOTLOADER_DIV_USED_IN_IRQ
	__enable_irq();
#endif

	return res;
}
