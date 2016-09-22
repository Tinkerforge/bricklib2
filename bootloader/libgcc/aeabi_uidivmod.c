#include "bricklib2/bootloader/bootloader.h"

extern BootloaderFunctions bootloader_functions;
uint64_t __aeabi_uidivmod(unsigned int a, unsigned int b) {
	return bootloader_functions.__aeabi_uidivmod(a, b);
}
