# RS
## Simple Build
`mkdir build`  
`cd build`  
`PARALLEL=8 CFLAGS=-Os LDFLAGS=-s ../client244/build.sh`  
`./client244.com 10 0 highmem members 0`  

## Generate a Makefile
`mkdir build-dev`  
`cd build-dev`  
`MAKEFILE=1 PARALLEL=8 CFLAGS="-g -Wall -Wextra -Wconversion -Wno-sign-conversion -fsanitize=undefined,address" LDFLAGS="-fsanitize=undefined,address" ../client244/build.sh`  
`make`  
`./client244.com 10 0 highmem members 0`  

## Cross Build
`. ./setup_toolchain.sh`  
`mkdir build-cross`  
`cd build-cross`  
`PARALLEL=8 CFLAGS=-Os LDFLAGS=-s ../client244/build.sh`  
