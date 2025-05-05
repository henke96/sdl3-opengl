# Inputs: $SCRIPT_DIR, $arch
binutils_version="2.44"
binutils_sha512="b85d3bbc0e334cf67a96219d3c7c65fbf3e832b2c98a7417bf131f3645a0307057ec81cd2b29ff2563cec53e3d42f73e2c60cc5708e80d4a730efdcc6ae14ad7"
gcc_version="14.2.0"
gcc_sha512="932bdef0cda94bacedf452ab17f103c0cb511ff2cec55e9112fc0328cbf1d803b42595728ea7b200e0a057c03e85626f937012e49a7515bc5dd256b2bf4bc396"
mingw_version="12.0.0"
mingw_sha512="949b2bfab8763ab10ec4e9fdfdaf5361517a4ab787fb98ab419b38d02694061c2e821ebbf6e2e4b39d92bdf17419d116daa8e63afd9e01d11592f39df4da69d7"

. "$SCRIPT_DIR/../../recipe.sh"
DEPENDENCIES="../make ../xz ../bzip2 ../gmp ../mpfr ../mpc"
DIR_DEPENDENCIES="../inc/w64-mingw32-gcc"

recipe_start
export PATH="$OUT/make/bin:$OUT/xz/bin:$OUT/bzip2/bin:$PATH"

xz -d -c "$(recipe_download "https://ftp.gnu.org/gnu/binutils/binutils-$binutils_version.tar.xz" "$binutils_sha512")" | tar xf -
cd "./binutils-$binutils_version"
./configure --prefix="$OUT/$SCRIPT_NAME" --target="$arch-w64-mingw32" --with-sysroot="$OUT/$SCRIPT_NAME" --without-libiconv-prefix --without-libintl-prefix --without-zstd \
--disable-dependency-tracking --disable-werror --disable-nls --enable-deterministic-archives CFLAGS="-O2"

make -j "$NUM_CPUS" all-binutils all-ld all-gas
make -j "$NUM_CPUS" install-binutils install-ld install-gas

cd ..
rm -rf "./binutils-$binutils_version"

bzip2 -d -c "$(recipe_download "https://sourceforge.net/projects/mingw-w64/files/mingw-w64/mingw-w64-release/mingw-w64-v$mingw_version.tar.bz2" "$mingw_sha512")" | tar xf -
cd "./mingw-w64-v$mingw_version/mingw-w64-headers"

./configure --prefix="$OUT/$SCRIPT_NAME/$arch-w64-mingw32" --host="$arch-w64-mingw32"

make -j "$NUM_CPUS"
make -j "$NUM_CPUS" install

cd ../..
xz -d -c "$(recipe_download "https://ftp.gnu.org/gnu/gcc/gcc-$gcc_version/gcc-$gcc_version.tar.xz" "$gcc_sha512")" | tar xf -

# Remove bad SIZE_MAX macro from generated flex code.
sed -e 's/#define SIZE_MAX               (~(size_t)0)//' "gcc-$gcc_version/gcc/gengtype-lex.cc" > sed.temp
mv sed.temp "gcc-$gcc_version/gcc/gengtype-lex.cc"

mkdir ./gcc-build
cd ./gcc-build

"../gcc-$gcc_version/configure" --prefix="$OUT/$SCRIPT_NAME" --target="$arch-w64-mingw32" --with-gmp="$OUT/gmp" --with-mpfr="$OUT/mpfr" --with-mpc="$OUT/mpc" --without-zstd \
--enable-languages=c --disable-shared --disable-nls --disable-multilib --disable-libstdcxx --disable-bootstrap --disable-gcov --disable-lto --disable-libatomic --disable-decimal-float --disable-libgomp --disable-libquadmath --disable-libssp \
LDFLAGS="-Wl,-rpath,$OUT/gmp/lib,-rpath,$OUT/mpfr/lib,-rpath,$OUT/mpc/lib" CFLAGS="-O2" CXXFLAGS="-O2" CFLAGS_FOR_TARGET="-O2 -ffile-prefix-map=$OUT=."

make -j "$NUM_CPUS" all-gcc
make -j "$NUM_CPUS" install-gcc

cd "../mingw-w64-v$mingw_version/mingw-w64-crt"

export PATH="$OUT/$SCRIPT_NAME/bin:$PATH"
./configure --prefix="$OUT/$SCRIPT_NAME/$arch-w64-mingw32" --host="$arch-w64-mingw32" --disable-dependency-tracking --disable-lib32 --enable-lib64 \
CC= CPP= CXX=false CFLAGS="-O2 -ffile-prefix-map=$OUT=."

make -j "$NUM_CPUS"
make -j "$NUM_CPUS" install

cd ../../gcc-build
rm -rf "../mingw-w64-v$mingw_version"

make -j "$NUM_CPUS"
make -j "$NUM_CPUS" install

rm -rf "../gcc-$gcc_version"
rm -rf "$PWD"

recipe_finish
