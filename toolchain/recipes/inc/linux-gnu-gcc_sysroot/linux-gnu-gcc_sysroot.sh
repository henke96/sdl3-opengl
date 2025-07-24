# Inputs: $SCRIPT_DIR, $arch
. "$SCRIPT_DIR/../../recipe.sh"

# Note that sdl3 already depends on the khronos directory.
DEPENDENCIES="$arch-linux-gnu-gcc_sdl3"
DIR_DEPENDENCIES="../inc/linux-gnu-gcc_sysroot"

recipe_start
(cd "$OUT/$arch-linux-gnu-gcc_sdl3" && tar cf - usr) | tar xf -
(cd "$SCRIPT_DIR/../../khronos" && tar cf - usr) | tar xf -
recipe_finish
