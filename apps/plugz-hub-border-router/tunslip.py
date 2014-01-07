#! /usr/bin/python


import socket, sys
import serial 
import os
from socket import *
from fcntl import ioctl
from select import select
import getopt, struct
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
    slipfd = os.open(SLIPDEV, os.O_RDWR | os.O_NONBLOCK)
    inslip = os.fdopen(slipfd, 'r')

#    ser = serial.Serial(SLIPDEV, 115200)

def main():
    #[inslip, slipfd] = create_slip()
    tunfd = create_tun()

if __name__ == "__main__":
    main()




