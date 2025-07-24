# Inputs: $SCRIPT_DIR, $arch, $freebsd_os
version="xkbcommon-1.8.1"
sha512="3e75dce17aaa042ca3134b6cd1b8c0da570b517420cd51e707087e0250186b122751ec84530b906b53faf0a85cae2696d67c2a8637fd61a35b36b1f9bc96ec2b"

. "$SCRIPT_DIR/../../recipe.sh"
DEPENDENCIES="../xz ../python ../meson ../samurai ../bison $arch-$freebsd_os-gcc"
DIR_DEPENDENCIES="../inc/freebsd-gcc_libxkbcommon"

recipe_start
export PATH="$OUT/xz/bin:$OUT/python/bin:$OUT/meson/bin:$OUT/bison/bin:$PATH"
xz -d -c "$(recipe_git_xz_download "https://github.com/xkbcommon/libxkbcommon.git" "$version" "$sha512")" | tar xf -
cd "./libxkbcommon.git-$version"

cat >cross.txt <<END
[binaries]
c = '$OUT/$arch-$freebsd_os-gcc/usr/bin/$arch-x-$freebsd_os-gcc'
cpp = ''
ar = '$OUT/$arch-$freebsd_os-gcc/usr/bin/$arch-x-$freebsd_os-ar'
strip = '$OUT/$arch-$freebsd_os-gcc/usr/bin/$arch-x-$freebsd_os-strip'
pkg-config = ''

[host_machine]
system = 'freebsd'
cpu_family = '$arch'
cpu = '$arch'
endian = 'little'
END

export NINJA="$OUT/samurai/bin/samu"
meson setup --cross-file ./cross.txt -Dprefix=/usr --buildtype=plain -Dc_args="-ffile-prefix-map=$OUT=." \
-Denable-wayland=false -Denable-x11=false -Denable-xkbregistry=false build

meson compile -j "$NUM_CPUS" -C build
DESTDIR="$OUT/$SCRIPT_NAME" meson install -C build

rm -rf "$PWD"
recipe_finish
