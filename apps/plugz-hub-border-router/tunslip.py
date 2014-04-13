#! /usr/bin/python
""" SLIP using TUN interface.
"""
import os
from fcntl import ioctl
import socket
import select
import serial
import struct
import logging
import argparse
import binascii
import string
import hexdump

TUNSETIFF = 0x400454ca
IFF_TUN   = 0x0001
IFF_TAP   = 0x0002
IFF_NO_PI = 0x1000

IPV6PREFIX = 'aaaa::'

SLIP_END = 0300
SLIP_ESC = 0333
SLIP_ESC_END = 0334
SLIP_ESC_ESC = 0335
DEBUG_MSG_START = 0x0D
DEBUG_MSG_END = 0x0A

def create_tun():
    """ Creates tunnel interface and sets up route entries.
    """

    # create virtual interface
    f = os.open("/dev/net/tun", os.O_RDWR)
    ifs = ioctl(f, TUNSETIFF, struct.pack("16sH", "tun%d", IFF_TUN | IFF_NO_PI))
    ifname = ifs[:16].strip("\x00")

    # configure IPv6 address
    os.system('ifconfig tun0 inet `hostname` up')
    os.system('ifconfig ' + ifname + ' inet add ' + IPV6PREFIX + '/64')
    os.system('ifconfig ' + ifname + ' inet6 add fe80::0:0:0:0/64')
    #v = os.system('ifconfig ' + ifname + ' up')

    # set route
    #os.system('route -A inet6 add ' + IPV6PREFIX + '/64 dev ' + ifname)

    # enable IPv6 forwarding
    os.system('echo 1 > /proc/sys/net/ipv6/conf/all/forwarding')
    logging.info('\nCreated following virtual interface:')
    os.system('ifconfig ' + ifname)
    return f

def slip_encode(byteList):
    """ Encodes the given IP packet as per SLIP protocol.
    """
    slipBuf = [SLIP_END]

    for i in byteList:
        if i == SLIP_END:
            slipBuf += [SLIP_ESC, SLIP_ESC_END]
        elif i == SLIP_ESC:
            slipBuf += [SLIP_ESC, SLIP_ESC_ESC]
        else:
            slipBuf.append(i)

    slipBuf.append(SLIP_END)
    return bytearray(slipBuf)

def slip_decode(serial_fd):
    """ Decodes the given SLIP packet into IP packet.
    """
    debug_msg = []
    dataBuf = []
    while True:
        c = serial_fd.read(1)
        if c is None:
            break
        serialByte = ord(c)

        if serialByte == SLIP_END:
            break
        elif serialByte == SLIP_ESC:
            c = serial_fd.read(1)
            if c is None:
                return []
            serialByte = ord(c)
            if serialByte == SLIP_ESC_END:
                dataBuf.append(SLIP_END)
            elif serialByte == SLIP_ESC_ESC:
                dataBuf.append(SLIP_ESC)
            elif serialByte == DEBUG_MSG_START:
                dataBuf.append(DEBUG_MSG_START)
            elif serialByte == DEBUG_MSG_END:
                dataBuf.append(DEBUG_MSG_END)
            else:
                logging.error("Protocol Error")
        elif serialByte == DEBUG_MSG_START and len(dataBuf) == 0:
            while True:
                c = serial_fd.read(1)
                if c is None or ord(c) == DEBUG_MSG_END:
                    break
                debug_msg.append(ord(c))
        else:
            dataBuf.append(serialByte)

    return dataBuf, debug_msg

def tun_to_serial(infd, outfd):
    """ Processes packets from tunnel and sends them over serial.
    """
    data = os.read(infd, 4096)
    if data:
        print 'tun_to_serial : '
        hexdump.hexdump(data)
        encoded = str(slip_encode(data))
        outfd.write(encoded)
    else:
        logging.error('Failed to read from TUN')

def serial_to_tun(infd, outfd):
    """ Processes packets from serial port and sends them over tunnel.
    """
    data, debug_msg = slip_decode(infd)

    if debug_msg is not None and len(debug_msg) > 0:
        for line in string.replace(str(bytearray(debug_msg)), '\r', '').split('\n'):
            if line != '' and line != '\r':
                logging.debug('serial>   {0}'.format(line))

    if data is None or len(data) <= 0:
        return

    if bytearray(data) == b'?P':
        """ Prefix info requested
        """
        raw_prefix = socket.inet_pton(socket.AF_INET6, IPV6PREFIX)
        prefix = slip_encode(bytearray('!P' + raw_prefix))
        logging.info('Sending IPv6 Prefix - ' + binascii.hexlify(prefix[3:-1]))
        infd.write(str(prefix))
    else:
        print 'serial_to_tun : '
        hexdump.hexdump(str(bytearray(data)))

        os.write(outfd, bytearray(data))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Sets logging level to high.')
    parser.add_argument('-s', '--serial-device', default='/dev/ttyUSB0',
                       help='Serial device path - Eg: /dev/ttyUSB0')

    args = parser.parse_args()
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    ser = serial.Serial(args.serial_device, 115200, timeout=5, bytesize=8, parity='N',
                        stopbits=1, xonxoff=False, rtscts=False)
    ser.write(serial.to_bytes([SLIP_END]))

    tunfd = create_tun()

    while True:
        read_fds = [ser.fileno(), tunfd]
        read_ready, write_ready, _e = select.select(read_fds, [], [])

        for fd in read_ready:
            if fd == ser.fileno():
                serial_to_tun(ser, tunfd)
            if fd == tunfd:
                tun_to_serial(tunfd, ser)

if __name__ == "__main__":
    main()
