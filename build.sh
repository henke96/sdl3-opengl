#!/bin/sh
set -e
SCRIPT_DIR="$(cd -- "${0%/*}/" && pwd)"

set -x
CC="${CC:-cc}"
PKG_CONFIG="${PKG_CONFIG:-pkg-config}"

SDL3_CFLAGS="${SDL3_CFLAGS:-"$("$PKG_CONFIG" --cflags sdl3)"}"
SDL3_LIBS="${SDL3_LIBS:-"$("$PKG_CONFIG" --libs sdl3)"}"

"$CC" -o sdl3-opengl -I"$SCRIPT_DIR/toolchain/egl/usr/include" $SDL3_CFLAGS $CFLAGS $LDFLAGS "$SCRIPT_DIR/src/sdl3-opengl.c" $SDL3_LIBS $LIBS
