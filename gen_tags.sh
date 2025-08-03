#!/bin/sh
set -e
cd -- "${0%/*}/"

"${CTAGS:-ctags}" client/*.c client/*.h
