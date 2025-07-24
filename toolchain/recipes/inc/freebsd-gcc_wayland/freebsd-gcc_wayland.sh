# Inputs: $SCRIPT_DIR, $arch, $freebsd_os
version="1.23.1"
sha512="818eda003e3f7aa15690eedb1ff227a6056b2ce54bf23d45ffe573dc40a914623c5a1358218b59444dcdc483db0503324f0d27091d0ea954412a8b290de5f50a"

. "$SCRIPT_DIR/../../recipe.sh"
DEPENDENCIES="../xz ../python ../meson ../samurai ../pkgconf ../wayland-scanner $arch-$freebsd_os-gcc $arch-$freebsd_os-gcc_libffi $arch-$freebsd_os-gcc_libepoll-shim"
DIR_DEPENDENCIES="../inc/freebsd-gcc_wayland"

recipe_start
export PATH="$OUT/xz/bin:$OUT/python/bin:$OUT/meson/bin:$OUT/pkgconf/bin:$PATH"
xz -d -c "$(recipe_download "https://gitlab.freedesktop.org/wayland/wayland/-/releases/$version/downloads/wayland-$version.tar.xz" "$sha512")" | tar xf -
cd "./wayland-$version"

mkdir ./sysroot
cd ./sysroot
(cd "$OUT/$arch-$freebsd_os-gcc_libffi" && tar cf - usr) | tar xf -
(cd "$OUT/$arch-$freebsd_os-gcc_libepoll-shim" && tar cf - usr) | tar xf -
cd ..

cat >cross.txt <<END
[binaries]
c = '$OUT/$arch-$freebsd_os-gcc/usr/bin/$arch-x-$freebsd_os-gcc'
cpp = ''
ar = '$OUT/$arch-$freebsd_os-gcc/usr/bin/$arch-x-$freebsd_os-ar'
strip = '$OUT/$arch-$freebsd_os-gcc/usr/bin/$arch-x-$freebsd_os-strip'
pkg-config = '$OUT/pkgconf/bin/pkgconf'

[host_machine]
system = 'freebsd'
cpu_family = '$arch'
cpu = '$arch'
endian = 'little'

[properties]
sys_root = '$PWD/sysroot'
END

export NINJA="$OUT/samurai/bin/samu"
meson setup --cross-file ./cross.txt -Dprefix=/usr --buildtype=plain -Dc_args="-ffile-prefix-map=$OUT=." \
-Dpkg_config_path="$PWD/sysroot/usr/lib/pkgconfig:$PWD/sysroot/usr/libdata/pkgconfig" -Dbuild.pkg_config_path="$OUT/wayland-scanner/lib/pkgconfig:$OUT/wayland-scanner/libdata/pkgconfig" \
-Ddocumentation=false -Ddtd_validation=false -Dtests=false -Dscanner=false build

meson compile -j "$NUM_CPUS" -C build
DESTDIR="$OUT/$SCRIPT_NAME" meson install -C build

rm -rf "$PWD"
recipe_finish
