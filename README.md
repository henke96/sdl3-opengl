# SDL3 OpenGL
## Simple Build
`mkdir build`  
`cd build`  
`PARALLEL=$(nproc) CFLAGS=-Os LDFLAGS=-s ../src/build.sh`  
`./sdl3-opengl`  

## Cross Build
`. setup_toolchain.sh`  
`mkdir build-cross`  
`cd build-cross`  
`PARALLEL=$(nproc) CFLAGS=-Os LDFLAGS=-s ../src/build.sh`  

## Configure a Makefile
`. setup_toolchain.sh`  
`mkdir build-dev`  
`cd build-dev`  
`PARALLEL=$(nproc) CONFIGURE=1 CFLAGS="-g -Wall -Wextra -Wconversion -Wno-sign-conversion -fsanitize=undefined" LDFLAGS="-fsanitize=undefined" ../src/build.sh`  
`make`  
