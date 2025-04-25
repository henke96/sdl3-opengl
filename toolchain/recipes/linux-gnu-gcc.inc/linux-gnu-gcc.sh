# Inputs: $arch, $linux_arch
linux_version="6.1.131"
linux_sha512="4fc8628e8593e7a713dbaba23f5f46eb94813d3a34791bbfd643d719e572ca4c2afa0b1e2b43dc3a664a9a2dca4d3c4145cd3c457636537022d52003d8b979cf"
binutils_version="2.44"
binutils_sha512="b85d3bbc0e334cf67a96219d3c7c65fbf3e832b2c98a7417bf131f3645a0307057ec81cd2b29ff2563cec53e3d42f73e2c60cc5708e80d4a730efdcc6ae14ad7"
gcc_version="14.2.0"
gcc_sha512="932bdef0cda94bacedf452ab17f103c0cb511ff2cec55e9112fc0328cbf1d803b42595728ea7b200e0a057c03e85626f937012e49a7515bc5dd256b2bf4bc396"
glibc_version="2.36"
glibc_sha512="9ea0bbda32f83a85b7da0c34f169607fb8a102f0a11a914e6bf531be47d1bef4f5307128286cffa1e2dc5879f0e6ccaef527dd353486883fa332a0b44bde8b3e"

DEPENDENCIES="../make ../xz ../python ../bison ../gawk ../sed ../grep ../gmp ../mpfr ../mpc"

recipe_start
export PATH="$OUT/make/bin:$OUT/xz/bin:$OUT/python/bin:$OUT/bison/bin:$OUT/gawk/bin:$OUT/sed/bin:$OUT/grep/bin:$PATH"

xz -d -c "$(recipe_download "https://kernel.org/pub/linux/kernel/v6.x/linux-$linux_version.tar.xz" "$linux_sha512")" | tar xf -
cd ./linux-$linux_version
# Allow building on FreeBSD (TODO upstream)
sed -e 's/uapi-asm-generic archheaders archscripts/uapi-asm-generic archheaders/' ./Makefile > ./sed.temp
mv ./sed.temp ./Makefile
make -j "$NUM_CPUS" headers HOSTCC="$CC" ARCH="$linux_arch"
fails="$(find usr/include -type f -name "*.h" ! -exec sh -ec 'dest="$1/$2"; mkdir -p "${dest%/*}/"; cp "$2" "$dest"' sh "$OUT/$SCRIPT_NAME" {} \; -print)"
test -z "$fails"

cd ..
rm -rf ./linux-$linux_version

xz -d -c "$(recipe_download "https://ftp.gnu.org/gnu/binutils/binutils-$binutils_version.tar.xz" "$binutils_sha512")" | tar xf -
cd "./binutils-$binutils_version"
./configure --prefix="$OUT/$SCRIPT_NAME/usr" --target="$arch-x-linux-gnu" --with-sysroot="$OUT/$SCRIPT_NAME" --without-libiconv-prefix --without-libintl-prefix --without-zstd \
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

"../gcc-$gcc_version/configure" --prefix="$OUT/$SCRIPT_NAME/usr" --target="$arch-x-linux-gnu" --with-glibc-version="$glibc_version" --with-sysroot="$OUT/$SCRIPT_NAME" --with-gmp="$OUT/gmp" --with-mpfr="$OUT/mpfr" --with-mpc="$OUT/mpc" --without-zstd \
--enable-languages=c --disable-shared --disable-nls --disable-multilib --disable-libstdcxx --disable-bootstrap --disable-gcov --disable-lto --disable-libatomic --disable-decimal-float --disable-libgomp --disable-libquadmath --disable-libssp \
--disable-fixincludes LDFLAGS="-Wl,-rpath,$OUT/gmp/lib,-rpath,$OUT/mpfr/lib,-rpath,$OUT/mpc/lib" CFLAGS="-O2" CXXFLAGS="-O2" CFLAGS_FOR_TARGET="-O2 -ffile-prefix-map=$OUT=."

# Fake limits.h until we install real glibc headers.
: > "$OUT/$SCRIPT_NAME/usr/include/limits.h"
make -j "$NUM_CPUS" all-gcc
make -j "$NUM_CPUS" install-gcc
rm "$OUT/$SCRIPT_NAME/usr/include/limits.h"

cd ..
xz -d -c "$(recipe_download "https://ftp.gnu.org/gnu/glibc/glibc-$glibc_version.tar.xz" "$glibc_sha512")" | tar xf -

# Fix check on FreeBSD where wc -l output is indented (TODO upstream)
sed -e 's/"$count" = 1/"$count" -eq 1/' ./glibc-$glibc_version/sysdeps/x86/configure > ./sed.temp
mv ./sed.temp ./glibc-$glibc_version/sysdeps/x86/configure
chmod +x ./glibc-$glibc_version/sysdeps/x86/configure

mkdir glibc-build
cd ./glibc-build

export PATH="$OUT/$SCRIPT_NAME/usr/bin:$PATH"
"../glibc-$glibc_version/configure" --prefix=/usr --build="$(sh scripts/config.guess)" --host="$arch-x-linux-gnu" --with-headers="$OUT/$SCRIPT_NAME/usr/include" --without-selinux --without-gd \
--disable-mathvec --disable-werror CC= CPP= CXX=false CFLAGS="-O2 -ffile-prefix-map=$OUT=."

make -j "$NUM_CPUS" csu/subdir_lib
make -j "$NUM_CPUS" install-headers csu/install-lib DESTDIR="$OUT/$SCRIPT_NAME"

cd ../gcc-build

: > "$OUT/$SCRIPT_NAME/usr/include/gnu/stubs.h"
make -j "$NUM_CPUS"
make -j "$NUM_CPUS" install
rm "$OUT/$SCRIPT_NAME/usr/include/gnu/stubs.h"

cd ../glibc-build
rm -rf "../gcc-$gcc_version"
rm -rf ../gcc-build

make -j "$NUM_CPUS"
make -j "$NUM_CPUS" install DESTDIR="$OUT/$SCRIPT_NAME"

rm -rf "../glibc-$glibc_version"
rm -rf "$PWD"

recipe_finish
