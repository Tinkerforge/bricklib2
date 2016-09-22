#include "bricklib2/bootloader/bootloader.h"

extern BootloaderFunctions bootloader_functions;
uint64_t __aeabi_idivmod(int a, int b) {
	return bootloader_functions.__aeabi_idivmod(a, b);
}
