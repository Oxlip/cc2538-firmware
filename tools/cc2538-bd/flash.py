#!/usr/bin/env python

""" CC2538 has a ROM bootloader which is exposed through devkit's UART.
	This program provides interface to accessing the ROM to program the flash.

	Refer the following for details about ROM backdoor
	http://www.ti.com/lit/ug/swru333a/swru333a.pdf
    http://processors.wiki.ti.com/index.php/CC2538_Bootloader_Backdoor

    This program allows writing a BIN file to CC2538's flash through UART.
    Connect CC2538's P0 and P1 to UART's TX and RX.
"""
from backdoor import *
import struct
from optparse import OptionParser

# CC2538 has 512K flash and each page size is 2K
flash_start_address = 0x00200000
flash_size = 512 * 1024
flash_page_size = 2048

""" Erase a single flash page
"""
def erase_flash_page(ser, start_address):
	print 'Erasing {0:#x} - {1:#x}'.format(start_address, start_address + flash_page_size)
	byteString = struct.pack('>B', CommandEnum.ERASE) + \
				 struct.pack('>L', start_address) + \
				 struct.pack('>L', flash_page_size)
	result = send_bytes(ser, bytearray(byteString))
	if not result:
		raise IOError('Failed to send ERASE command')
		return False
	status = get_last_command_status(ser)
	if status == CommandRetEnum.SUCCESS:
		return True
	if status == CommandRetEnum.FLASH_FAIL:
		return False
	raise IOError('Unhandled command status after ERASE')

""" Erase all the flash pages
"""
def erase_flash(ser, start_address=flash_start_address, size=flash_size):
	for address in xrange(start_address, start_address + size, flash_page_size):
		erase_flash_page(ser, address)

""" Send DOWNLOAD command to ROM to initiate flash write process
"""
def send_download_command(ser, flash_address):
	byteString = struct.pack('>B', CommandEnum.DOWNLOAD) + \
				 struct.pack('>L', flash_address) + \
				 struct.pack('>L', flash_page_size)
	result = send_bytes(ser, bytearray(byteString))
	if not result:
		raise IOError('Failed to send DOWNLOAD command')
		return False
	status = get_last_command_status(ser)
	if status != CommandRetEnum.SUCCESS:
		return False

""" Write 128bytes of data to flash.

    At time a maximum of 252 bytes can be written.
    Because the backdoor communication limits the databits to 252bytes.
    So to fill a flash page(2048 bytes) we need 128x16 writes.
"""
def write_128bytes(ser, content, flash_address):
	#assert len(content) == 128

	byteString = struct.pack('>B', CommandEnum.SEND_DATA) + content
	result = send_bytes(ser, bytearray(byteString))
	if not result:
		raise IOError('Failed to send SEND_DATA command')
		return False
	status = get_last_command_status(ser)
	if status != CommandRetEnum.SUCCESS:
		raise IOError('Failed to SEND_DATA command failed')
		return False

""" Fill a flash page with given command.

	This function expects the page is already erased.
"""
def program_flash_page(ser, content, flash_address):
	#print len(content), flash_page_size
	#assert len(content) == flash_page_size
	print 'Writing {0:#x} - {1:#x}'.format(flash_address, flash_address + flash_page_size)

	#start DOWNLOAD process	
	send_download_command(ser, flash_address)
	for position in xrange(0, len(content), 128):
		write_128bytes(ser, content[position:position+128], flash_address)
	return True

""" Write the given BIN file to CC2538's flash
"""
def program_flash(ser, bin_file, start_address=flash_start_address):
	data = open(bin_file, "rb").read()
	file_size = len(data)
	if file_size > flash_size:
		print 'File({0} = {1}KB) is bigger than flash({2}KB)'.format(bin_file, file_size / 1024, flash_size / 1024)
		return False
	print 'Programming {0} ({1}KB)'.format(bin_file, file_size / 1024)
	for position in xrange(0, file_size, flash_page_size):
		flash_address = start_address + position
		program_flash_page(ser, data[position:position+flash_page_size], flash_address)

	return True


def parse_command_line():
	parser = OptionParser()
	parser.add_option("-f", "--file", dest="filename",
                  	  help="Binary file (Eg examples/cc2538dk/cc2538-demo.bin)", metavar="FILE")
	parser.add_option("-u", "--uart", dest="uart",
                  	  help="UART (Eg /dev/ttyUSB1)", metavar="FILE")

	(options, args) = parser.parse_args()
	return options.filename, options.uart

def main():
	filename, uart = parse_command_line()
	ser = connect(serialDevice=uart, timeout=5)
	if ser == None:
		print 'Not able to connect to {0}'.format(serialDevice)
		return
	
	send_command(ser, CommandEnum.GET_CHIP_ID)
	chip_id = receive(ser)
	print 'Chip ID : 0x{0:X}{1:X}'.format(chip_id[2], chip_id[3])

	# Erash all the pages
	erase_flash(ser)
	# Write the program
	program_flash(ser, filename)
	# Reset the chip so that new program will start
	send_command(ser, CommandEnum.RESET)

	disconnect(ser)

main()