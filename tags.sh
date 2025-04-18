#!/bin/sh
set -e
cd -- "${0%/*}/"

"${CTAGS:-ctags}" -R src
