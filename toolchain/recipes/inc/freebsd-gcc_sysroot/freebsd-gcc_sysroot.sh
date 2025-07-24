# Inputs: $SCRIPT_DIR, $arch, $freebsd_os
. "$SCRIPT_DIR/../../recipe.sh"

# Note that sdl3 already depends on the khronos directory.
DEPENDENCIES="$arch-$freebsd_os-gcc_sdl3"
DIR_DEPENDENCIES="../inc/freebsd-gcc_sysroot"

recipe_start
(cd "$OUT/$arch-$freebsd_os-gcc_sdl3" && tar cf - usr) | tar xf -
(cd "$SCRIPT_DIR/../../khronos" && tar cf - usr) | tar xf -
recipe_finish
