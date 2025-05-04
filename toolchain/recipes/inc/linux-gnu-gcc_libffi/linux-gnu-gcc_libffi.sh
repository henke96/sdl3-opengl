# Inputs: $SCRIPT_DIR, $arch
version="3.4.7"
sha512="d19f59a5b5d61bd7d9e8a7a74b8bf2e697201a19c247c410c789e93ca8678a4eb9f13c9bee19f129be80ade8514f6b1acb38d66f44d86edd32644ed7bbe31dd6"

. "$SCRIPT_DIR/../../recipe.sh"
DEPENDENCIES="../make $arch-linux-gnu-gcc"
DIR_DEPENDENCIES="../inc/linux-gnu-gcc_libffi"

recipe_start
export PATH="$OUT/make/bin:$OUT/$arch-linux-gnu-gcc/usr/bin:$PATH"
gzip -d -c "$(recipe_download "https://github.com/libffi/libffi/releases/download/v$version/libffi-$version.tar.gz" "$sha512")" | tar xf -
cd "./libffi-$version"

./configure --prefix=/usr --build="$(sh ./config.guess)" --host="$arch-x-linux-gnu" --disable-dependency-tracking CC= CPP= CXX=no CFLAGS="-ffile-prefix-map=$OUT=."

make -j "$NUM_CPUS"
make -j "$NUM_CPUS" install DESTDIR="$OUT/$SCRIPT_NAME"

rm -rf "$PWD"
recipe_finish
