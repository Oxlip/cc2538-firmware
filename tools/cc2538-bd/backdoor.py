""" CC2538 has a ROM bootloader which is exposed through devkit's UART.
	This program provides interface to accessing the ROM to program the flash.

	Refer the following for details about ROM backdoor
	http://www.ti.com/lit/ug/swru333a/swru333a.pdf
    http://processors.wiki.ti.com/index.php/CC2538_Bootloader_Backdoor
"""

import serial
import array
import struct

# Commands supported by the bootloader
class CommandEnum:
	PING = 0x20
	DOWNLOAD = 0x21
	RUN = 0x22
	GET_STATUS = 0x23
	SEND_DATA = 0x24
	RESET = 0x25
	ERASE = 0x26
	CRC32 = 0x27
	GET_CHIP_ID = 0x28
	SET_XOSC = 0x29
	MEMORY_READ = 0x2A
	MEMORY_WRITE = 0x2B

# Command results
class CommandRetEnum:
	SUCCESS = 0x40
	UNKNOWN_CMD = 0x41
	INVALID_CMD = 0x42
	INVALID_ADR = 0x43
	FLASH_FAIL = 0x44

""" Checks and receives ACK from the bootloader
	Returns True to ACK was received
"""
def receive_ack(ser):
	ack = ser.read(2)
	ackBytes = array.array('B', ack)
	return ackBytes != None and len(ackBytes) == 2 and \
		   ackBytes[0] == 0 and ackBytes[1] == 0xCC

""" Sends ACK to the bootloader
"""
def send_ack(ser):
	ser.write(b'\x00\xCC')

""" Sends NACK to the bootloader
"""
def send_nack(ser):
	ser.write(b'\x00\x33')

""" Connects to the ROM bootloader through UART
"""
def connect(serialDevice, timeout):
	speed=115200
	ser = serial.Serial(serialDevice, speed, timeout=timeout)
	# Negotiate speed with the bootloader
	ser.write(b'\x55\x55')
	if receive_ack(ser):
		return ser
	ser.close()
	raise IOError('Not able to connect to CC2538 ROM backdoor')

""" Disconnects to the ROM bootloader through UART
"""
def disconnect(ser):
	ser.close()

""" Calculates checksum for the given array of bytes
"""
def calc_checksum(bytes):
	chksum = 0
	for b in bytes:
		chksum += b
	return chksum % 256

""" Sends the given bytes to ROM
"""
def send_bytes(ser, bytes):
	# length and checksum
	meta = bytearray(struct.pack('>B', len(bytes) + 2)) + \
		   bytearray(struct.pack('>B', calc_checksum(bytes)))
	send = ser.write(meta + bytes)
	# ROM should ACK our command
	if not receive_ack(ser):
		return False

	return True

""" Receive data from ROM
"""
def receive(ser):
	# read the total length, ignore if length is 0
	length = 0
	while length == 0:
		length = ser.read()
		if length == None:
			raise IOError('No data received')
		length = int(length.encode('hex'), 16)
		
	# Read the checksum
	chksum = ser.read()
	chksum = int(chksum.encode('hex'), 16)

	# read the actual bytes
	bytes = ser.read(length - 2)
	byteArray = array.array('B', bytes)

	# verify the checksum matches
	if chksum != calc_checksum(byteArray):
		raise IOError('Invalid checksum expected %d got %d' % (chksum, calc_checksum(byteArray)))

	# We should ACK to inform ROM that we received data
	send_ack(ser)

	return byteArray

""" Send a single byte command
"""
def send_command(ser, command):
	bs = bytearray(struct.pack('>B', command))
	return send_bytes(ser, bs)

""" Get last command's status
"""
def get_last_command_status(ser):
	bs = bytearray(struct.pack('>B', CommandEnum.GET_STATUS))
	if not send_bytes(ser, bs):
		return CommandRetEnum.UNKNOWN_CMD
	return receive(ser)[0]