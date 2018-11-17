#!/bin/bash -e
echo "Compiling"
gcc vm.c -o vm
echo "Running"
./vm BACKING_STORE.bin addresses.txt > out.txt
echo "Comparing"
diff out.txt correct.txt
