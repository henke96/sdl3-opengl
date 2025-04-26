#!/bin/sh
set -e
SCRIPT_DIR="${0%/*}/"

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

if test -z "$DIRECT_BUILD"; then
    printf '#!/bin/sh\nexec env -i DIRECT_BUILD="$DIRECT_BUILD" PATH="$PATH" TERM="$TERM" SOURCE_DATE_EPOCH=0 TZ=UTC0 LC_ALL=C %s %s %s %s %s %s\n' \
"${CONFIGURE+"CONFIGURE=$(escape "$CONFIGURE")"} ${OUT+"OUT=$(escape "$OUT")"} ${PARALLEL+"PARALLEL=$(escape "$PARALLEL")"}" \
"${CC+"CC=$(escape "$CC")"} ${PKG_CONFIG+"PKG_CONFIG=$(escape "$PKG_CONFIG")"} ${PKG_CONFIG_PATH+"PKG_CONFIG_PATH=$(escape "$PKG_CONFIG_PATH")"} ${PKG_CONFIG_SYSROOT_DIR+"PKG_CONFIG_SYSROOT_DIR=$(escape "$PKG_CONFIG_SYSROOT_DIR")"}" \
"${CFLAGS+"CFLAGS=$(escape "$CFLAGS")"} ${LDFLAGS+"LDFLAGS=$(escape "$LDFLAGS")"} ${LDLIBS+"LDLIBS=$(escape "$LDLIBS")"} ${DEP_CFLAGS+"DEP_CFLAGS=$(escape "$DEP_CFLAGS")"}" \
"${GLESV2_CFLAGS+"GLESV2_CFLAGS=$(escape "$GLESV2_CFLAGS")"} ${SDL3_CFLAGS+"SDL3_CFLAGS=$(escape "$SDL3_CFLAGS")"}" \
"${SDL3_LIBS+"SDL3_LIBS=$(escape "$SDL3_LIBS")"}" \
"sh $(escape "$0")" > rebuild.sh

    chmod a+x ./rebuild.sh
    DIRECT_BUILD=1 exec sh ./rebuild.sh
fi

CC="${CC:-cc}"
PKG_CONFIG="${PKG_CONFIG:-pkg-config}"

OUT="${OUT:-sdl3-opengl}"
PARALLEL="${PARALLEL:-1}"
DEP_CFLAGS="${DEP_CFLAGS-"-MD -MP"}"
test "$CONFIGURE" || DEP_CFLAGS=

GLESV2_CFLAGS="${GLESV2_CFLAGS-"$("$PKG_CONFIG" --cflags glesv2)"}"
SDL3_CFLAGS="${SDL3_CFLAGS-"$("$PKG_CONFIG" --cflags sdl3)"}"
SDL3_LIBS="${SDL3_LIBS-"$("$PKG_CONFIG" --libs sdl3)"}"

compile() {
    cmd="$CC -c $DEP_CFLAGS $GLESV2_CFLAGS $SDL3_CFLAGS $CFLAGS $(escape "$SCRIPT_DIR$1.c")"
    printf "%s\n" "$cmd"
    eval "$cmd"
    if test "$CONFIGURE"; then
        dep_cmd="mv $source.d $source.o.d"
        eval "$dep_cmd"
        printf "%s.o:\n\t%s\n\t@%s\ninclude %s.o.d\n" "$1" "$cmd" "$dep_cmd" "$1" >> Makefile.temp
    fi
}

wait_compile() {
    set -- $pids
    wait "$1"
    shift
    pids="$@"
    running=$(($running-1))
}

test -z "$CONFIGURE" || printf ".POSIX:\n%s:\nMakefile: %s\n\tsh ./rebuild.sh\n" "$OUT" "$0" > Makefile.temp

c_sources="sdl3-opengl gl"
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

cmd="$CC -o $OUT $LDFLAGS$objects $SDL3_LIBS $LDLIBS"
printf "%s\n" "$cmd"
eval "$cmd"
if test "$CONFIGURE"; then
    printf "%s: %s\n\t%s\nclean:\n\trm -f %s%s\n" "$OUT" "$objects" "$cmd" "$OUT" "$objects" >> Makefile.temp
    mv Makefile.temp Makefile
fi
