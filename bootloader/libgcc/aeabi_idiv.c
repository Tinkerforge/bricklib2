#include "bricklib2/bootloader/bootloader.h"

extern BootloaderFunctions bootloader_functions;
int __aeabi_idiv(int a, int b) {
	return bootloader_functions.__aeabi_idiv(a, b);
}
