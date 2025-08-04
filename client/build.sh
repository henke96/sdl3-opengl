#!/bin/sh
set -e
SCRIPT_DIR="${0%/*}/"

escape() {
    unescaped="$1"
    while :; do
        case "$unescaped" in
        *\'*)
            printf "'%s'\\'" "${unescaped%%\'*}"
            unescaped="${unescaped#*\'}"
            ;;
        *)
            printf "'%s'" "$unescaped"
            break
        esac
    done
}

if test -z "$DIRECT_BUILD"; then
    env_vars=
    for env_var in \
MAKEFILE MAKEFILE_DEP_CFLAGS PARALLEL OUT_NAME \
CC PKG_CONFIG PKG_CONFIG_PATH PKG_CONFIG_SYSROOT_DIR \
CFLAGS LDFLAGS LDLIBS \
PLATFORM \
SDL3_CFLAGS \
SDL3_LIBS
    do
        eval 'env_vars="$env_vars${'"$env_var"'+"'"$env_var"'=$(escape "$'"$env_var"'") "}"'
    done

    printf '#!/bin/sh\n%sexec sh %s\n' "$env_vars" "$(escape "$0")" > rebuild.sh
    chmod a+x ./rebuild.sh
    eval 'exec env -i DIRECT_BUILD=1 PATH="$PATH" TERM="$TERM" TMP="$TMP" SOURCE_DATE_EPOCH=0 TZ=UTC0 LC_ALL=C '"$env_vars"' sh ./rebuild.sh'
fi

CC="${CC:-cc}"
PKG_CONFIG="${PKG_CONFIG:-pkg-config}"

OUT_NAME="${OUT_NAME:-client.com}"
PARALLEL="${PARALLEL:-1}"
MAKEFILE_DEP_CFLAGS="${MAKEFILE_DEP_CFLAGS:-"-MD -MP"}"
test "$MAKEFILE" || MAKEFILE_DEP_CFLAGS=

PLATFORM="${PLATFORM:-sdl3}"

if test "$PLATFORM" = "sdl3"; then
    SDL3_CFLAGS="${SDL3_CFLAGS-"$("$PKG_CONFIG" --cflags sdl3)"}"
    SDL3_LIBS="${SDL3_LIBS-"$("$PKG_CONFIG" --libs sdl3)"}"
    platform_cflags="-I $(escape "${SCRIPT_DIR}platform/sdl3") $SDL3_CFLAGS"
    platform_libs="$SDL3_LIBS"
    platform_sources="platform/sdl3/platform"
else
    echo "Invalid PLATFORM $PLATFORM"
    exit 1
fi

compile() {
    cmd="$CC -c $MAKEFILE_DEP_CFLAGS $platform_cflags $CFLAGS $(escape "$SCRIPT_DIR$1.c")"
    printf "%s\n" "$cmd"
    eval "$cmd"
    if test "$MAKEFILE"; then
        dep_cmd="mv ${1##*/}.d ${1##*/}.o.d"
        eval "$dep_cmd"
        printf "%s.o:\n\t%s\n\t@%s\ninclude %s.o.d\n" "${1##*/}" "$cmd" "$dep_cmd" "${1##*/}" >> Makefile.temp
    fi
}

wait_compile() {
    set -- $pids
    wait "$1"
    shift
    pids="$@"
    running=$(($running-1))
}

test -z "$MAKEFILE" || printf ".POSIX:\n%s:\nMakefile: %s\n\tsh ./rebuild.sh\n" "$OUT_NAME" "$0" > Makefile.temp

pids=
objects=
running=0
for source in \
$platform_sources \
client \
util \
pix2d \
pix3d \
world \
world3d \
bzip2 \
packet \
file_stream \
jagfile \
pix_font \
jpeg \
pix32 \
pix_map
do
    compile "$source" &
    pids="$pids $!"
    objects="$objects ${source##*/}.o"
    running=$(($running+1))
    test "$running" -lt "$PARALLEL" || wait_compile
done
for pid in $pids
do
    wait "$pid"
done

cmd="$CC -o $OUT_NAME $LDFLAGS$objects $platform_libs $LDLIBS"
printf "%s\n" "$cmd"
eval "$cmd"
if test "$MAKEFILE"; then
    printf "%s: %s\n\t%s\nclean:\n\trm -f %s%s\n" "$OUT_NAME" "$objects" "$cmd" "$OUT_NAME" "$objects" >> Makefile.temp
    mv Makefile.temp Makefile
fi
