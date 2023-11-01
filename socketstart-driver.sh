#!/bin/sh
cd aesd-char-driver/
make clean
cd ..
#Perform clean operation

sudo ./aesd-char-driver/aesdchar_unload
# Unload the driver

cd aesd-char-driver/
make
cd ..
#Build the kernel object file
sudo ./aesd-char-driver/aesdchar_load
#Loaded the char device module
cat /proc/devices | grep "aesdchar"
#Check if aesdchar driver is in the current list of allocated drivers
#./assignment-autotest/test/assignment8/drivertest.sh

cd server/
make clean && make
./aesdsocket
#./assignment-autotest/test/assignment5/sockettest.sh
#strace -o /tmp/strace-aesdsocket.txt -f ./aesd-char-driver/aesdchar.ko