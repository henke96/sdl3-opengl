# Inputs: $SCRIPT_DIR, $arch
version="0.3.65"
sha512="f0810b7199110fb88231b260969acb88e67027edb4584558f2eefd8ec07d88f627157a16a3a7cbf39cb71b9f4ac3c10736bcda895888c83223653497a0e17961"

. "$SCRIPT_DIR/../../recipe.sh"
DEPENDENCIES="../xz ../python ../meson ../samurai $arch-linux-gnu-gcc"
DIR_DEPENDENCIES="../inc/linux-gnu-gcc_libpipewire"

recipe_start
export PATH="$OUT/xz/bin:$OUT/python/bin:$OUT/meson/bin:$PATH"
xz -d -c "$(recipe_git_xz_download "https://gitlab.freedesktop.org/pipewire/pipewire.git" "$version" "$sha512")" | tar xf -
cd "./pipewire.git-$version"

cat >cross.txt <<END
[binaries]
c = '$OUT/$arch-linux-gnu-gcc/usr/bin/$arch-x-linux-gnu-gcc'
cpp = ''
ar = '$OUT/$arch-linux-gnu-gcc/usr/bin/$arch-x-linux-gnu-ar'
strip = '$OUT/$arch-linux-gnu-gcc/usr/bin/$arch-x-linux-gnu-strip'
pkg-config = ''

[host_machine]
system = 'linux'
cpu_family = '$arch'
cpu = '$arch'
endian = 'little'
END

export NINJA="$OUT/samurai/bin/samu"
meson setup --cross-file ./cross.txt -Dprefix=/usr --buildtype=plain -Dc_args="-ffile-prefix-map=$OUT=." \
-Dauto_features=disabled -Ddbus=disabled -Dexamples=disabled -Dflatpak=disabled \
-Dsession-managers="[]" -Dtests=disabled build
meson compile -j "$NUM_CPUS" -C build
DESTDIR="$OUT/$SCRIPT_NAME" meson install -C build

rm -rf "$PWD"
recipe_finish
