# SDL3 OpenGL
## Simple Build
`mkdir build`  
`cd build`  
`PARALLEL=8 CFLAGS=-Os LDFLAGS=-s ../src/build.sh`  
`./sdl3-opengl`  

## Cross Build
`. ./setup_toolchain.sh`  
`mkdir build-cross`  
`cd build-cross`  
`PARALLEL=8 CFLAGS=-Os LDFLAGS=-s ../src/build.sh`  

## Configure a Makefile
`mkdir build-dev`  
`cd build-dev`  
`CONFIGURE=1 PARALLEL=8 CFLAGS="-g -Wall -Wextra -Wconversion -Wno-sign-conversion -fsanitize=undefined,address" LDFLAGS="-fsanitize=undefined,address" ../src/build.sh`  
`make`  
