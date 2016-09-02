#!/usr/bin/env python

# This program is used by the makefiles of co-processor Bricklets to append
# the checksum at the end of the firmware

import binascii
import sys
import struct

sys.stdout.write(struct.pack('<I', binascii.crc32(open(sys.argv[1],'rb').read()) & 0xFFFFFFFF))
