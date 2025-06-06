# Inputs: $SCRIPT_DIR, $arch
version="0.1.1"
sha512="b4fd3d22bbc61cd7b0bfc70b1cd89bea60d563de0e7716d87e2e1a6ad32d3dec9850396a0692bb1f879b4aa341ceca816e196cb149a89067eb571d498ab7df25"

. "$SCRIPT_DIR/../../recipe.sh"
DEPENDENCIES="../xz ../python ../meson ../samurai ../pkgconf ../wayland-protocols $arch-linux-gnu-gcc $arch-linux-gnu-gcc_wayland"
DIR_DEPENDENCIES="../inc/linux-gnu-gcc_libdecor"

recipe_start
export PATH="$OUT/xz/bin:$OUT/python/bin:$OUT/meson/bin:$OUT/wayland-scanner/bin:$PATH"
xz -d -c "$(recipe_download "https://gitlab.freedesktop.org/-/project/18349/uploads/ee5ef0f2c3a4743e8501a855d61cb397/libdecor-$version.tar.xz" "$sha512")" | tar xf -
cd "./libdecor-$version"

mkdir ./sysroot
cd ./sysroot
(cd "$OUT/wayland-protocols" && tar cf - usr) | tar xf -
(cd "$OUT/$arch-linux-gnu-gcc_wayland" && tar cf - usr) | tar xf -
(cd "$OUT/$arch-linux-gnu-gcc_libffi" && tar cf - usr) | tar xf -
cd ..

cat >cross.txt <<END
[binaries]
c = '$OUT/$arch-linux-gnu-gcc/usr/bin/$arch-x-linux-gnu-gcc'
cpp = ''
ar = '$OUT/$arch-linux-gnu-gcc/usr/bin/$arch-x-linux-gnu-ar'
strip = '$OUT/$arch-linux-gnu-gcc/usr/bin/$arch-x-linux-gnu-strip'
pkg-config = '$OUT/pkgconf/bin/pkgconf'

[host_machine]
system = 'linux'
cpu_family = '$arch'
cpu = '$arch'
endian = 'little'

[properties]
sys_root = '$PWD/sysroot'
END

# TODO upstream ability to avoid building plugins
sed -e "s/subdir('plugins')//" ./src/meson.build > ./sed.temp
mv ./sed.temp ./src/meson.build

export NINJA="$OUT/samurai/bin/samu"
meson setup --cross-file ./cross.txt -Dprefix=/usr --buildtype=plain -Dc_args="-ffile-prefix-map=$OUT=." \
-Dpkg_config_path="$PWD/sysroot/usr/lib/pkgconfig:$PWD/sysroot/usr/share/pkgconfig" \
-Ddbus=disabled -Ddemo=false build

meson compile -j "$NUM_CPUS" -C build
DESTDIR="$OUT/$SCRIPT_NAME" meson install -C build

rm -rf "$PWD"
recipe_finish
