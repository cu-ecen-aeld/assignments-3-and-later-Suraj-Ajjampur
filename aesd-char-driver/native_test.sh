#!/bin/sh
echo "Starting native unload, load and driver test script"
make clean

./aesdchar_unload

make

./aesdchar_load 

../assignment-autotest/test/assignment8/drivertest.sh
echo "End of native unload, load and driver test script run"