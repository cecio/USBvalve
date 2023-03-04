#
# Create an empty file
#     dd if=/dev/zero of=fat.fs bs=1024 count=1024
# Format fs file
#     sudo mkfs.fat fat.fs -g 1/1 -f 1 -s 1 -r 16 -n "USBVALVE"
#
# Mount the fs
#     sudo mount fat.fs /mnt
#
# Now you can create a minimal set of files (AUTORUN.INF and README.TXT).
# Place them (by manually fixing the ROOT_DIR and FATTABLE) out of the caching
# sectors (> 100 ?)
#

import sys
import re

def all_zero(buff):
	ret = True
	for x in buff:
		if x != 0:
			ret = False
			break
	return ret

if len(sys.argv) < 2:
	print('Please specify input file')
	sys.exit(1)

f = open(sys.argv[1], "rb")

print('uint8_t msc_disk[DISK_BLOCK_NUM][DISK_BLOCK_SIZE] = {')

block = 0

while True:
	data = f.read(512)
	if not data:
		break
	print('{ ')
	print('//------------- Block %d: -------------//' % (block))
	block += 1
	if all_zero(data):
		print('0x00')
	else:
		dump = ' '.join("0x{:02x},".format(c) for c in data)
		dump = re.sub("(.{96})", "\\1\n", dump, 0, re.DOTALL)
		dump = dump[:-1]     # Remove last comma
		print(dump)
	print('},')

print('};')

f.close()
sys.exit(0)