#!/bin/sh
set -e
cd -- "${0%/*}/"

platform_sources=
test -z "$1" || platform_sources="client244/platform/$1/*.c client244/platform/$1/*.h"

${CTAGS:-ctags} client244/*.c client244/*.h client244/lib/*.c client244/lib/*.h $platform_sources
