#!/bin/sh
# This script should work both for Windows and Linux.

cd ../lib/$1
echo Building nested library...
make

echo Cleaning up temporary files...
# This step causes the library to be re-built every time the pre-build step runs.
# This is necessary because we can't clean up the library folder explicitly because
# Eclipse does not pass the "clean" target to the pre-build step if Project->Clean... is used.
rm *.o
