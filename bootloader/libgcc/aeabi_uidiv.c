#include "bricklib2/bootloader/bootloader.h"

extern BootloaderFunctions bootloader_functions;
unsigned int __aeabi_uidiv(unsigned int a, unsigned int b) {
	return bootloader_functions.__aeabi_uidiv(a, b);
}
