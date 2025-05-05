# Inputs: $SCRIPT_DIR, $arch
. "$SCRIPT_DIR/../../recipe.sh"

DEPENDENCIES="$arch-w64-mingw32-gcc_sdl3"
DIR_DEPENDENCIES="../../khronos/usr ../inc/w64-mingw32-gcc_sysroot"

recipe_start
(cd "$OUT/$arch-w64-mingw32-gcc_sdl3" && tar cf - usr) | tar xf -
(cd "$SCRIPT_DIR/../../khronos" && tar cf - usr) | tar xf -
recipe_finish
