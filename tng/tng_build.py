#!/usr/bin/env python3

# This program is used by the makefiles of TNG modules to prepend CRC
# and length of firmware file.
# the checksum at the end of the firmware

import binascii
import sys
import struct

firmware = open(sys.argv[1],'rb').read()
crc = struct.pack('<I', binascii.crc32(firmware) & 0xFFFFFFFF)
length = struct.pack('<I', len(firmware))
sys.stdout.buffer.write(crc)
sys.stdout.buffer.write(length)
sys.stdout.buffer.write(firmware[8:])
