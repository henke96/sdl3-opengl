# Inputs: $SCRIPT_DIR, $arch, $freebsd_os
version="v0.0.20240608"
sha512="fcf8faf5adad03a9ca489e504ca0d9e0b679c1bc6b198e13ffdd9222c597b637b3dba82a7eb3479af4750443ce3fc0cc18a19d4bef00b19ed33ecbb1e2c7373e"

. "$SCRIPT_DIR/../../recipe.sh"
DEPENDENCIES="../make ../xz ../cmake $arch-$freebsd_os-gcc"
DIR_DEPENDENCIES="../inc/freebsd-gcc_libepoll-shim"

recipe_start
export PATH="$OUT/make/bin:$OUT/xz/bin:$OUT/cmake/bin:$PATH"
xz -d -c "$(recipe_git_xz_download "https://github.com/jiixyj/epoll-shim.git" "$version" "$sha512")" | tar xf -
cd "./epoll-shim.git-$version"

cat >cross.cmake <<END
set(CMAKE_SYSTEM_NAME FreeBSD)

set(CMAKE_C_COMPILER $OUT/$arch-$freebsd_os-gcc/usr/bin/$arch-x-$freebsd_os-gcc)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
END

cmake -B build -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=./cross.cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_C_FLAGS="-ffile-prefix-map=$OUT=." \
-DBUILD_TESTING=OFF -DALLOWS_ONESHOT_TIMERS_WITH_TIMEOUT_ZERO_EXITCODE=0
cmake --build build -j "$NUM_CPUS"
DESTDIR="$OUT/$SCRIPT_NAME" cmake --install build

rm -rf "$PWD"
recipe_finish
