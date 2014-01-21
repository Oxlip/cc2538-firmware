#! /usr/bin/python


import socket, sys
import serial 
import os
from socket import *
from fcntl import ioctl
import select
import getopt, struct
import termios, tty
#from pytun import TunTapDevice


TUNSETIFF = 0x400454ca
IFF_TUN   = 0x0001
IFF_TAP   = 0x0002

TUNMODE = IFF_TUN
MODE = 0
DEBUG = 0

TUNDEV = "tun0"
SLIPDEV = "/dev/ttyUSB0"
IPV6PREFIX = 'aaaa'
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
    v = os.system('ifconfig ' + ifname + ' inet6 add ' + IPV6PREFIX + '::1/64')
    v = os.system('ifconfig ' + ifname + ' inet6 add fe80::1/64')
    v = os.system('ifconfig ' + ifname + ' up')

    # set route
    os.system('route -A inet6 add ' + IPV6PREFIX + '::/64 dev ' + ifname)

    # enable IPv6 forwarding
    os.system('echo 1 > /proc/sys/net/ipv6/conf/all/forwarding')
    print('\ncreated following virtual interface:')
    os.system('ifconfig ' + ifname)
    return f

def create_slip():
    ser = serial.Serial(SLIPDEV, 115200, timeout=0)
    #slipfd = os.open(SLIPDEV, os.O_RDWR | os.O_NONBLOCK)
    #inslip = os.fdopen(slipfd, 'r')

    #termios.tcflush(slipfd, termios.TCIOFLUSH)
    #iflag, oflag, cflag, lflag, ispeed, ospeed, cc = termios.tcgetattr(slipfd)
    #ispeed = 115200
    #ospeed = 115200
    #ttyattr[6][6] = ttyattr[6][6] & ~termios.ECHO
    #ttyattr[6][6] = 1
    #print ttyattr
    #termios.tcsetattr(slipfd, termios.TCSANOW, [iflag, oflag, cflag, lflag, ispeed, ospeed, cc])
    #tty.setraw(slipfd)
    #return [ser, slipfd, inslip]
    return ser

def getSerialByte(serialFD):       
    newByte = ord(serialFD.read())  
    return newByte  

def slipEncode(byteList):  
    slipBuf = []  
    slipBuf.append(SLIP_END)  
    for i in byteList:  
        print "%c" %(i)
        if i == SLIP_END:  
            slipBuf.append(SLIP_ESC)  
            slipBuf.append(SLIP_ESC_END)  
        elif i == SLIP_ESC:  
            slipBuf.append(SLIP_ESC)  
            slipBuf.append(SLIP_ESC_ESC)  
        else:  
            slipBuf.append(i)  
            slipBuf.append(SLIP_END)  
            return slipBuf

def slipDecode(serial_fd):  
    dataBuf = []  
    while True:  
        serialByte = getSerialByte(serial_fd)  
        print "Serial byte : "  + serialByte
        if serialByte is None:  
            return -1  
        elif serialByte == SLIP_END:  
            if len(dataBuf) > 0:  
                return dataBuf  
            elif serialByte == SLIP_ESC:  
                serialByte = getSerialByte(serial_fd)  
                if serialByte is None:  
                    return -1  
                elif serialByte == SLIP_ESC_END:  
                    dataBuf.append(SLIP_END)  
                elif serialByte == SLIP_ESC_ESC:  
                    dataBuf.append(SLIP_ESC)  
                elif serialByte == DEBUG_MAKER:  
                    dataBuf.append(DEBUG_MAKER)  
                else:  
                    print("Protocol Error")  
            else:  
                dataBuf.append(serialByte)  
                return 


def tun_to_serial(infd, outfd):
    data = os.read(infd, size) 
    send_buf = ""
    if data: 
        print "Packet from TUN of length %d -- write SLIP" %(len(data))
        slipData = encodeToSlip(c)
        os.write(outfd, slipData)
    else: 
        print "Failed to read"

def serial_to_tun(infd, outfd):
    data = slipDecode(infd)
    outfd.write(data)



def main():
    #ser, slipfd, inslip = create_slip()
    ser  = create_slip()
    tunfd = create_tun()

    while True:
        read_fds = [ser.fileno(), tunfd]
        write_fds = [ser.fileno()]

        write_ready, read_ready, _e = select.select(read_fds, write_fds,[])

        for fd in read_ready:
            if fd == ser.fileno():
                print "read event on slipfd "
                serial_to_tun(ser, tunfd)
            if fd == tunfd:
                print "read event on tunfd "
                tun_to_serial(tunfd, slipfd)

        for fd in write_ready:
            print  "Write fd ready"




if __name__ == "__main__":
    main()




