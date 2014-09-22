#!/bin/sh

SDKDRIVER=ftdi_sio
SDKUSB_HWID=0403
SDKUSB_PID=a6d1


CC2538REPO=`pwd`/..
UHUBDIR=$CC2538REPO/apps/plugz-hub
USENSEDIR=$CC2538REPO/apps/plugz-sense
UHUBREPO=$CC2538REPO/../uhub
PYTHON=python

LOGFILE=`pwd`/test.log
FLASHTOOL=$CC2538REPO/tools/cc2538-bd/flash.py

SENSOR_IP=aaaa::f4:ac6a:12:4b00
#NOFLASH=1



if python --version | grep 'Python 3\.' > /dev/null; then
    echo "Using python2"
    PYTHON=python2
fi


# Checking if module is loaded

if [ `lsmod | grep $SDKDRIVER | wc -l` -eq 0 ]; then
    KERNEL_VER_MAJOR=`uname -r | cut -d . -f 1`
    KERNEL_VER_MINOR=`uname -r | cut -d . -f 2`
    echo "Try to load $SDKDRIVER module"
    if [ $KERNEL_VER_MAJOR -lt 3 ] || [ $KERNEL_VER_MINOR -lt 12 ]; then
	sudo modprobe $SDKDRIVER vendor=0x$SDKUSB_HWID product=0x$SDKUSB_PID
    else
	sudo modprobe $SDKDRIVER
	echo $SDKUSB_HWID $SDKUSB_PID | \
	    sudo tee /sys/bus/usb-serial/drivers/$SDKDRIVER/new_id	
    fi
fi


# Flashing the boards
if [ -z "$NOFLASH" ]; then

    SDKUSB_TTY=`ls -1 /sys/bus/usb-serial/drivers/ftdi_sio/ | grep ttyUSB | tail -n 1`
    if [ -z "$SDKUSB_TTY" ]; then
	echo "The sdk board don't seem to be plugged."
	exit 1
    fi

    echo "Using $SDKUSB_TTY as tty for sdk board"


    C2538USB_TTY=`ls -1 /sys/bus/usb-serial/drivers/pl2303/ | grep ttyUSB | tail -n 1`
    if [ -z "$C2538USB_TTY" ]; then
	echo "The second CC2538 don't seem to be plugged"
	exit 1
    fi

    echo "Using $C2538USB_TTY as tty for cc2538"

    cd $USENSEDIR
    make >> $LOGFILE

    echo "Type enter when the sdk board is ready to be flashed"
    read

    sudo $PYTHON $FLASHTOOL  -w plugz-sense.p2.bin -c -f 0x240000 -s 0x40000 -u /dev/$SDKUSB_TTY

    cd $UHUBDIR
    make >> $LOGFILE

    echo "Type enter when the cc2538 board is ready to be flashed"
    read
    sudo $PYTHON $FLASHTOOL  -w plugz-hub.p2.bin -c -f 0x240000 -s 0x40000 -u /dev/$C2538USB_TTY
fi



echo "Running uhub python script"

sudo $PYTHON $UHUBREPO/main.py --verbose 0 >> $LOGFILE 2>&1 &
UHUB_PID=$!


# Waiting routes to be created
sleep 2

echo "switching on leds"

CMD="$PYTHON $UHUBREPO/coap_request.py --ip $SENSOR_IP --node dev/led --method post"

$CMD --data 'green on'
$CMD --data 'red on'
$CMD --data 'orange on'

sleep 5

echo "switching off leds"
$CMD --data 'green off'
$CMD --data 'red off'
$CMD --data 'orange off'

echo "Type enter to finish"
read

sudo kill -9 $UHUB_PID


