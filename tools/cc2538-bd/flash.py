#!/usr/bin/env python

""" CC2538 has a ROM bootloader which is exposed through devkit's UART.
   This program provides interface to accessing the ROM to program the flash.

   Refer the following for details about ROM backdoor
   http://www.ti.com/lit/ug/swru333a/swru333a.pdf
    http://processors.wiki.ti.com/index.php/CC2538_Bootloader_Backdoor

    This program allows writing a BIN file to CC2538's flash through UART.
    Connect CC2538's P0 and P1 to UART's TX and RX.
"""
import os
from backdoor import *
import struct
from optparse import OptionParser
from progressbar import Bar, ETA, Percentage, ProgressBar
import hexdump

KB = 1024

# CC2538 has 512K flash and each page size is 2K
flash_page_size = 2 * KB


""" Read 32bit memory from the device
"""
def read_memory_32bit(ser, address):
    byteString = struct.pack(
        '>BLB', CommandEnum.MEMORY_READ, address, 4)

    bytes = bytearray(byteString)
    meta = bytearray(struct.pack('>BB', len(bytes) + 2, calc_checksum(bytes)))
    result = send_bytes(ser, bytearray(byteString))
    if not result:
        raise IOError('Failed to send READ_MEMORY command')
    result = receive(ser)
    status = get_last_command_status(ser)

    if status != CommandRetEnum.SUCCESS:
        raise IOError('Unhandled command status after READ_MEMORY')
    return result


""" Read memory at the given address of given size
"""
def read_memory(ser, start_address, size):
    result = bytearray()
    for address in xrange(start_address, start_address + size, 4):
        result += read_memory_32bit(ser, address)
    return result


""" Erase a single flash page
"""
def erase_flash_page(ser, start_address):
    byteString = struct.pack(
        '>BLL', CommandEnum.ERASE, start_address, flash_page_size)
    result = send_bytes(ser, bytearray(byteString))
    if not result:
        raise IOError('Failed to send ERASE command')
    status = get_last_command_status(ser)
    if status == CommandRetEnum.SUCCESS:
        return
    if status == CommandRetEnum.FLASH_FAIL:
        raise IOError('Error returned - FLASH_FAIL')
    raise IOError('Unhandled command status after ERASE')


""" Erase all the flash pages
"""
def erase_flash(ser, start_address, size):
    print 'Erasing flash @ {0:#x} size {1}KB'.format(start_address, size / KB)
    widgets = [Bar('#'), ' ', ETA()]
    pbar = ProgressBar(widgets=widgets, maxval=size).start()
    for address in xrange(start_address, start_address + size, flash_page_size):
        erase_flash_page(ser, address)
        pbar.update(address - start_address)
    pbar.finish()


""" Send DOWNLOAD command to ROM to initiate flash write process
"""
def send_download_command(ser, start_address, size=flash_page_size):
    byteString = struct.pack(
        '>BLL', CommandEnum.DOWNLOAD, start_address, size)
    result = send_bytes(ser, bytearray(byteString))
    if not result:
        raise IOError('Failed to send DOWNLOAD command')
    status = get_last_command_status(ser)
    if status != CommandRetEnum.SUCCESS:
        print status
        raise IOError('DOWNLOAD command returned error')


""" Write 128bytes of data to flash.

    At time a maximum of 252 bytes can be written.
    Because the backdoor communication limits the databits to 252bytes.
    So to fill a flash page(2048 bytes) we need 128x16 writes.
"""
def write_128bytes(ser, content, flash_address):
    byteString = struct.pack('>B', CommandEnum.SEND_DATA) + content
    result = send_bytes(ser, bytearray(byteString))
    if not result:
        raise IOError('Failed to send SEND_DATA command')
    status = get_last_command_status(ser)
    if status != CommandRetEnum.SUCCESS:
        raise IOError('Failed to SEND_DATA command failed')


""" Fill a flash page with given command.

    This function expects the page is already erased.
"""
def program_flash_page(ser, content, flash_address):
    write_size = 128
    # start DOWNLOAD process
    send_download_command(ser, flash_address)
    for position in xrange(0, len(content), write_size):
        write_128bytes(
            ser, content[position: position + write_size], flash_address)


""" Write the given BIN file to CC2538's flash
"""
def program_flash(ser, bin_file, start_address, flash_size):
    data = open(bin_file, "rb").read()
    file_size = len(data)
    if file_size > flash_size:
        print 'File({0} = {1}KB) is bigger than flash({2}KB)'.format(bin_file, file_size / KB, flash_size / KB)

    print 'Programming {0} ({1}KB)'.format(bin_file, file_size / KB)
    widgets = [Bar('#'), ' ', ETA()]
    pbar = ProgressBar(widgets=widgets, maxval=file_size).start()
    for position in xrange(0, file_size, flash_page_size):
        flash_address = start_address + position
        program_flash_page(
            ser, data[position: position + flash_page_size], flash_address)
        pbar.update(position)
    pbar.finish()


""" Writes CCA at the end of flash
"""
def write_cca(ser, flash_start, flash_size):
    lock_bits = '\xFF' * 32
    cca = struct.pack('<LLL', 0xFFFFFFFF, 0, flash_start) + lock_bits
    cca_addr = flash_start + flash_size - 44
    print 'Writing cca at ', hex(cca_addr), len(cca)
    erase_flash_page(ser, flash_start + flash_size - flash_page_size)
    send_download_command(ser, cca_addr, len(cca))
    write_128bytes(ser, cca, cca_addr)


""" Parse command line arguments
"""
def parse_command_line():
    parser = OptionParser()
    parser.add_option('-w', '--write-file', dest='filename', metavar='FILE',
                      help='Write the given firmware file(in binary format) to flash.\n'
                           '(Eg: examples/cc2538dk/cc2538-demo.bin)')
    parser.add_option('-r', '--read', nargs=2, type=int,
                      help='Read data from given address and length.\n'
                           '(Eg: -r 0x200000 128)')
    parser.add_option('-u', '--uart', dest='uart', metavar='FILE',
                      help='UART device which is connected to CC2538.\n'
                           '(Eg: /dev/ttyUSB1)')
    parser.add_option('-s', '--flash-size', type=int, default=512 * KB,
                      help='Size of the flash in the CC2538.\n'
                           '(Default : 512KB)')
    parser.add_option('-f', '--flash-start', type=int, default=0x200000,
                      help='Starting address of the firmware where to begin writing the firmware.\n'
                           '(Default: 0x200000)')
    parser.add_option('-c', '--write-cca', action='store_true',
                      help='Writes CCA at the end of the flash')
    (options, args) = parser.parse_args()
    return options


def main():
    options = parse_command_line()
    ser = connect(serialDevice=options.uart, timeout=5)
    if ser == None:
        print 'Not able to connect to {0}'.format(serialDevice)
        return

    #Handle read operation first
    if options.read:
        bytes = read_memory(ser, options.read[0], options.read[1])
        hexdump.hexdump(str(bytes))
        return

    if options.filename is None:
        file_size = options.flash_size
    else:
        file_size = os.path.getsize(options.filename)
        if file_size > options.flash_size:
            print 'File({0} = {1}KB) is bigger than flash({2}KB)'.format(options.filename, file_size / KB, options.flash_size / KB)
            return


    send_command(ser, CommandEnum.GET_CHIP_ID)
    chip_id = receive(ser)
    print 'Chip ID : 0x{0:X}{1:X}'.format(chip_id[2], chip_id[3])

    # Erash flash pages
    erase_flash(ser, start_address=options.flash_start, size=file_size)
    if options.filename:
        # Write the program
        program_flash(ser, options.filename, start_address=options.flash_start, flash_size=options.flash_size)
        # write CCA if needed.
        if options.write_cca:
            write_cca(ser, flash_start=options.flash_start, flash_size=options.flash_size)

    # Reset the chip so that new program will start
    send_command(ser, CommandEnum.RESET)
    disconnect(ser)

main()
