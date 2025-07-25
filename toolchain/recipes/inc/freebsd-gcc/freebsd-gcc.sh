# Inputs: $SCRIPT_DIR, $arch, $freebsd_arch, $freebsd_version, $freebsd_sha512
binutils_version="2.44"
binutils_sha512="b85d3bbc0e334cf67a96219d3c7c65fbf3e832b2c98a7417bf131f3645a0307057ec81cd2b29ff2563cec53e3d42f73e2c60cc5708e80d4a730efdcc6ae14ad7"
gcc_version="15.1.0"
gcc_sha512="ddd35ca6c653dffa88f7c7ef9ee4cd806e156e0f3b30f4d63e75a8363361285cd566ee73127734cde6a934611de815bee3e32e24bfd2e0ab9f7ff35c929821c1"

. "$SCRIPT_DIR/../../recipe.sh"
DEPENDENCIES="../make ../xz ../python ../gzip ../bison ../gawk ../sed ../grep ../gmp ../mpfr ../mpc"
DIR_DEPENDENCIES="../inc/freebsd-gcc"

recipe_start
export PATH="$OUT/make/bin:$OUT/xz/bin:$OUT/python/bin:$OUT/gzip/bin:$OUT/bison/bin:$OUT/gawk/bin:$OUT/sed/bin:$OUT/grep/bin:$PATH"
xz -d -c "$(recipe_download "http://ftp-archive.freebsd.org/pub/FreeBSD-Archive/old-releases/$freebsd_arch/$freebsd_version-RELEASE/base.txz" "$freebsd_sha512" "$arch-freebsd$freebsd_version-")" | tar xf - ./usr/include ./usr/lib ./lib
rm -rf ./usr/lib/gcc ./usr/lib/clang

xz -d -c "$(recipe_download "https://ftp.gnu.org/gnu/binutils/binutils-$binutils_version.tar.xz" "$binutils_sha512")" | tar xf -
cd "./binutils-$binutils_version"
./configure --prefix="$OUT/$SCRIPT_NAME/usr" --target="$arch-x-freebsd$freebsd_version" --with-sysroot="$OUT/$SCRIPT_NAME" --without-libiconv-prefix --without-libintl-prefix --without-zstd \
--disable-dependency-tracking --disable-werror --disable-nls --enable-deterministic-archives CFLAGS="-O2"

make -j "$NUM_CPUS" all-binutils all-ld all-gas
make -j "$NUM_CPUS" install-binutils install-ld install-gas

cd ..
rm -rf "./binutils-$binutils_version"

xz -d -c "$(recipe_download "https://ftp.gnu.org/gnu/gcc/gcc-$gcc_version/gcc-$gcc_version.tar.xz" "$gcc_sha512")" | tar xf -

# Remove bad SIZE_MAX macro from generated flex code.
sed -e 's/#define SIZE_MAX               (~(size_t)0)//' "gcc-$gcc_version/gcc/gengtype-lex.cc" > sed.temp
mv sed.temp "gcc-$gcc_version/gcc/gengtype-lex.cc"

mkdir ./gcc-build
cd ./gcc-build

"../gcc-$gcc_version/configure" --prefix="$OUT/$SCRIPT_NAME/usr" --target="$arch-x-freebsd$freebsd_version" --with-sysroot="$OUT/$SCRIPT_NAME" --with-gmp="$OUT/gmp" --with-mpfr="$OUT/mpfr" --with-mpc="$OUT/mpc" --without-zstd \
--enable-languages=c --disable-shared --disable-nls --disable-multilib --disable-libstdcxx --disable-bootstrap --disable-gcov --disable-lto --disable-libatomic --disable-decimal-float --disable-libgomp --disable-libquadmath --disable-libssp \
--disable-fixincludes LDFLAGS="-Wl,-rpath,$OUT/gmp/lib,-rpath,$OUT/mpfr/lib,-rpath,$OUT/mpc/lib" CFLAGS="-O2" CXXFLAGS="-O2" CFLAGS_FOR_TARGET="-O2 -ffile-prefix-map=$OUT=."

make -j "$NUM_CPUS"
make -j "$NUM_CPUS" install

rm -rf "../gcc-$gcc_version"
rm -rf "$PWD"

recipe_finish
