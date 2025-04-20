#!/bin/sh
set -e
SCRIPT_DIR="$(cd -- "${0%/*}/" && pwd)"

CC="${CC:-cc}"
PKG_CONFIG="${PKG_CONFIG:-pkg-config}"

PARALLEL="${PARALLEL:-1}"
DEP_CFLAGS="${DEP_CFLAGS-"-MD -MP"}"
test "$CONFIGURE" || DEP_CFLAGS=

GL_CFLAGS="${GL_CFLAGS-"$("$PKG_CONFIG" --cflags gl)"}"
SDL3_CFLAGS="${SDL3_CFLAGS-"$("$PKG_CONFIG" --cflags sdl3)"}"
SDL3_LIBS="${SDL3_LIBS-"$("$PKG_CONFIG" --libs sdl3)"}"

escape() {
    printf \'
    unescaped="$1"
    while :; do
        case "$unescaped" in
        *\'*)
            printf %s "${unescaped%%\'*}'\''"
            unescaped="${unescaped#*\'}"
            ;;
        *)
            printf %s "$unescaped"
            break
        esac
    done
    printf \'
}

compile() {
    cmd="$CC -c $DEP_CFLAGS $GL_CFLAGS $SDL3_CFLAGS $CFLAGS $(escape "$SCRIPT_DIR/$1.c")"
    printf "%s\n" "$cmd"
    eval "$cmd"
    if test "$CONFIGURE"; then
        dep_cmd="mv $source.d $source.o.d"
        eval "$dep_cmd"
        printf "%s.o:\n\t%s\n\t@%s\ninclude %s.o.d\n" "$1" "$cmd" "$dep_cmd" "$1" >> Makefile
    fi
}

wait_compile() {
    set -- $pids
    wait "$1"
    shift
    pids="$@"
    running=$(($running-1))
}

test -z "$CONFIGURE" || printf ".POSIX:\nsdl3-opengl:\n" > Makefile

main=sdl3-opengl
c_sources="$main gl"
pids=
objects=
running=0
for source in $c_sources
do
    compile "$source" &
    pids="$pids $!"
    objects="$objects $source.o"
    running=$(($running+1))
    test "$running" -lt "$PARALLEL" || wait_compile
done
for pid in $pids
do
    wait "$pid"
done

cmd="$CC -o $main $LDFLAGS$objects $SDL3_LIBS $LDLIBS"
printf "%s\n" "$cmd"
eval "$cmd"
test -z "$CONFIGURE" || printf "%s: %s\n\t%s\nclean:\n\trm -f %s%s\n" "$main" "$objects" "$cmd" "$main" "$objects" >> Makefile
