#!/bin/sh
set -e
cd -- "${0%/*}/"

CTAGS="${CTAGS:-ctags}"

"$CTAGS" --extras=Ff -R src
