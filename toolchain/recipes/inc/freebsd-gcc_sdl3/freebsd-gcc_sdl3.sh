# Inputs: $SCRIPT_DIR, $arch, $freebsd_os
version="3.2.8"
sha512="741550120f26fe983fa829d38d37ff4caeded31e899e4ded99ebb5b084eecebaa4081454b8e176e0103fa7faa6a230ad3f23f8c2453c6babfe59fab981cac6db"

. "$SCRIPT_DIR/../../recipe.sh"
DEPENDENCIES="../make ../cmake ../pkgconf ../wayland-scanner $arch-$freebsd_os-gcc $arch-$freebsd_os-gcc_libffi $arch-$freebsd_os-gcc_wayland $arch-$freebsd_os-gcc_libxkbcommon"
DIR_DEPENDENCIES="../../khronos/usr ../inc/freebsd-gcc_sdl3"

recipe_start
export PATH="$OUT/make/bin:$OUT/cmake/bin:$OUT/pkgconf/bin:$OUT/wayland-scanner/bin:$PATH"
gzip -d -c "$(recipe_download "https://github.com/libsdl-org/SDL/releases/download/release-$version/SDL3-$version.tar.gz" "$sha512")" | tar xf -
cd "./SDL3-$version"

mkdir ./sysroot
cd ./sysroot
(cd "$OUT/$arch-$freebsd_os-gcc_wayland" && tar cf - usr) | tar xf -
(cd "$OUT/$arch-$freebsd_os-gcc_libepoll-shim" && tar cf - usr) | tar xf -
(cd "$OUT/$arch-$freebsd_os-gcc_libffi" && tar cf - usr) | tar xf -
(cd "$OUT/$arch-$freebsd_os-gcc_libxkbcommon" && tar cf - usr) | tar xf -
(cd "$SCRIPT_DIR/../../khronos" && tar cf - usr) | tar xf -
export PKG_CONFIG_PATH="$PWD/usr/lib/pkgconfig:$PWD/usr/libdata/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$PWD"
cd ..

cat >cross.cmake <<END
set(CMAKE_SYSTEM_NAME FreeBSD)

set(CMAKE_C_COMPILER $OUT/$arch-$freebsd_os-gcc/usr/bin/$arch-x-$freebsd_os-gcc)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
END

cmake -B build -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=./cross.cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_C_FLAGS="-ffile-prefix-map=$OUT=." \
-DSDL_SHARED=OFF -DSDL_STATIC=ON -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON \
-DSDL_ALSA=OFF -DSDL_DBUS=OFF -DSDL_DISKAUDIO=OFF -DSDL_DUMMYAUDIO=OFF -DSDL_DUMMYCAMERA=OFF -DSDL_DUMMYVIDEO=OFF \
-DSDL_IBUS=OFF -DSDL_JACK=OFF -DSDL_KMSDRM=OFF -DSDL_LIBUDEV=OFF -DSDL_LIBURING=OFF -DSDL_OFFSCREEN=OFF \
-DSDL_OPENGL=OFF -DSDL_PIPEWIRE=OFF -DSDL_PULSEAUDIO=OFF -DSDL_RENDER_GPU=OFF -DSDL_RENDER_METAL=OFF -DSDL_RENDER_VULKAN=OFF \
-DSDL_SNDIO=OFF -DSDL_VULKAN=OFF -DSDL_X11=OFF
cmake --build build -j "$NUM_CPUS"
DESTDIR="$OUT/$SCRIPT_NAME" cmake --install build

rm -rf "$PWD"
recipe_finish
