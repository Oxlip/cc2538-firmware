#! /usr/bin/python


import socket
import serial
import os
from fcntl import ioctl
import select
import struct
import logging
import argparse
import binascii

TUNSETIFF = 0x400454ca
IFF_TUN   = 0x0001
IFF_TAP   = 0x0002

TUNMODE = IFF_TUN
MODE = 0

TUNDEV = "tun0"
IPV6PREFIX = 'aaaa::'
IFF_TUN    = 0x0001

SLIP_END = 0300
SLIP_ESC = 0333
SLIP_ESC_END = 0334
SLIP_ESC_ESC = 0335

'''
def creat_tun_if():
   tun = TunTapDevice(name='mytun')
   print tun.name
   '''

def create_tun():
    # create virtual interface
    f = os.open("/dev/net/tun", os.O_RDWR)
    ifs = ioctl(f, TUNSETIFF, struct.pack("16sH", "tun%d", IFF_TUN))
    ifname = ifs[:16].strip("\x00")

    # configure IPv6 address
    v = os.system('ifconfig ' + ifname + ' inet6 add ' + IPV6PREFIX + '/64')
    v = os.system('ifconfig ' + ifname + ' inet6 add fe80::1/64')
    v = os.system('ifconfig ' + ifname + ' up')

    # set route
    os.system('route -A inet6 add ' + IPV6PREFIX + '/64 dev ' + ifname)

    # enable IPv6 forwarding
    os.system('echo 1 > /proc/sys/net/ipv6/conf/all/forwarding')
    logging.info('\nCreated following virtual interface:')
    os.system('ifconfig ' + ifname)
    return f

def create_slip(serial_device):
    ser = serial.Serial(serial_device, 115200, timeout=5, bytesize=8, parity='N',
                        stopbits=1, xonxoff=False, rtscts=False)
    ser.write(serial.to_bytes([SLIP_END]))
    return ser

def slip_encode(byteList):
    slipBuf = []
    slipBuf.append(SLIP_END)

    for i in byteList:
        if i == SLIP_END:
            slipBuf.append(SLIP_ESC)
            slipBuf.append(SLIP_ESC_END)
        elif i == SLIP_ESC:
            slipBuf.append(SLIP_ESC)
            slipBuf.append(SLIP_ESC_ESC)
        else:
            slipBuf.append(i)

    slipBuf.append(SLIP_END)
    return bytearray(slipBuf)

def slip_decode(serial_fd):
    dataBuf = []
    while True:
        c = serial_fd.read(1)
        if c is None or len(c) <= 0:
            return dataBuf
        serialByte = ord(c)

        if serialByte == SLIP_END:
            if len(dataBuf) > 0:
                return dataBuf
        elif serialByte == SLIP_ESC:
            c = serial_fd.read(1)
            if c is None:
                return []
            serialByte = ord(c)
            if serialByte == SLIP_ESC_END:
                dataBuf.append(SLIP_END)
            elif serialByte == SLIP_ESC_ESC:
                dataBuf.append(SLIP_ESC)
            elif serialByte == DEBUG_MAKER:
                dataBuf.append(DEBUG_MAKER)
            else:
                logging.error("Protocol Error")
        else:
            dataBuf.append(serialByte)


def tun_to_serial(infd, outfd):
    data = os.read(infd, size)
    send_buf = ""
    if data:
        logging.debug('Packet from TUN of length %d -- write SLIP' % (len(data)))
        slipData = encodeToSlip(c)
        os.write(outfd, slipData)
    else:
        logging.error('Failed to read from TUN')

def serial_to_tun(infd, outfd):
    data = slip_decode(infd)
    if data is None or len(data) <= 0:
        return

    #logging.debug('SLIP read {0}'.format(data))

    string = str(bytearray(data))
    #print string

    if string == b'?P':
        """ Prefix info requested
        """
        raw_prefix = socket.inet_pton(socket.AF_INET6, IPV6PREFIX)
        prefix = slip_encode(bytearray('!P' + raw_prefix))
        logging.info('Sending IPv6 Prefix - ' + binascii.hexlify(prefix[3:-1]))
        infd.write(str(prefix))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Sets logging level to high.')
    parser.add_argument('-s', '--serial-device', default='/dev/ttyUSB0',
                       help='Serial device path - Eg: /dev/ttyUSB0')

    args = parser.parse_args()
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    else:
        logging.getLogger().setLevel(logging.INFO)


    ser  = create_slip(args.serial_device)
    tunfd = create_tun()

    while True:
        read_fds = [ser.fileno(), tunfd]
        write_fds = [ser.fileno()]

        write_ready, read_ready, _e = select.select(read_fds, write_fds,[])

        for fd in read_ready:
            if fd == ser.fileno():
                serial_to_tun(ser, tunfd)
            if fd == tunfd:
                tun_to_serial(tunfd, slipfd)

if __name__ == "__main__":
    main()
