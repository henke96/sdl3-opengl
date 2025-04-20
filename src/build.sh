#!/bin/sh
set -e
SCRIPT_DIR="$(cd -- "${0%/*}/" && pwd)"

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

CC="${CC:-cc}"
PKG_CONFIG="${PKG_CONFIG:-pkg-config}"

DEP_CFLAGS="${DEP_CFLAGS-"-MD -MP"}"
if test "$CONFIGURE"; then
    printf ".POSIX:\nsdl3-opengl:\n" > Makefile
else
    unset DEP_CFLAGS
fi

GL_CFLAGS="${GL_CFLAGS-"$("$PKG_CONFIG" --cflags gl)"}"
SDL3_CFLAGS="${SDL3_CFLAGS-"$("$PKG_CONFIG" --cflags sdl3)"}"
SDL3_LIBS="${SDL3_LIBS-"$("$PKG_CONFIG" --libs sdl3)"}"

c_sources="sdl3-opengl"
objects=
for source in $c_sources
do
    cmd="$CC -c $DEP_CFLAGS $GL_CFLAGS $SDL3_CFLAGS $CFLAGS $(escape "$SCRIPT_DIR/$source.c")"
    printf "%s\n" "$cmd"
    eval "$cmd"
    if test "$CONFIGURE"; then
        dep_cmd="mv $source.d $source.c.d"
        eval "$dep_cmd"
        printf "%s\n" "$dep_cmd"
        printf "%s.o:\n\t%s\n\t%s\ninclude %s.c.d\n" "$source" "$cmd" "$dep_cmd" "$source" >> Makefile
    fi
    objects="$source.o $objects"
done

cmd="$CC -o sdl3-opengl $LDFLAGS $objects $SDL3_LIBS $LIBS"
printf "%s\n" "$cmd"
eval "$cmd"
test "$CONFIGURE" && printf "sdl3-opengl: %s\n\t%s\nclean:\n\trm -f sdl3-opengl %s\n" "$objects" "$cmd" "$objects" >> Makefile
