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
`mkdir build-dev`  
`cd build-dev`  
`echo 'CONFIGURE=1 PARALLEL=$(nproc) CFLAGS="-g -Wall -Wextra -Wconversion -Wno-sign-conversion -fsanitize=undefined,address" LDFLAGS="-fsanitize=undefined,address" ../src/build.sh' > configure.sh`  
`sh configure.sh`  
`make`  
