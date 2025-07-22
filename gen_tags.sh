#!/bin/sh
set -e
cd -- "${0%/*}/"

"${CTAGS:-ctags}" src/*.c src/*.h
