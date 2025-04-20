# SDL3 OpenGL
## Simple Build
`mkdir build`  
`cd build`  
`CFLAGS=-Os LDFLAGS=-s ../src/build.sh`  
`./sdl3-opengl`  

## Cross Build
`. setup_toolchain.sh`  
`mkdir build-cross`  
`cd build-cross`  
`CFLAGS=-Os LDFLAGS=-s ../src/build.sh`  

## Configure a Makefile
`. setup_toolchain.sh`  
`mkdir build-dev`  
`cd build-dev`  
`CONFIGURE=1 CFLAGS=-g ../src/build.sh`  
`make`  
