recipe_start() {
    SCRIPT_NAME="${0##*/}"
    if test -z "$recipe_TIMESTAMP"; then
        if test -z "$OUT"; then
            set +x
            echo 'Please set environment variable `OUT` to the output directory.'
            exit 1
        fi
        OUT="$(cd -- "$OUT" && pwd)"
        test "$OUT" != "$SCRIPT_DIR"
        DOWNLOADS="$(cd -- "${DOWNLOADS:-"$OUT"}" && pwd)"
        # Run ourself with a clean environment.
        exec env -i recipe_TIMESTAMP="$(date)" recipe_DOWNLOADS="$DOWNLOADS" OUT="$OUT" NUM_CPUS="${NUM_CPUS:-1}" CC="${CC:-cc}" CXX="${CXX:-c++}" PATH="$PATH" TERM="$TERM" SOURCE_DATE_EPOCH=0 TZ=UTC0 LC_ALL=C sh "$SCRIPT_DIR/$SCRIPT_NAME"
    fi
    cd "$SCRIPT_DIR"
    recipe_outdir="$OUT/$SCRIPT_NAME"

    # Build dependencies if needed.
    for recipe_temp in $DEPENDENCIES; do
        if test "$recipe_TIMESTAMP" != "$(cat "$OUT/${recipe_temp##*/}/sha512-timestamp")"; then sh "./$recipe_temp"; fi
    done

    # Check if the recipe needs to be rebuilt.
    if test -n "$DIR_DEPENDENCIES"; then
        find $DIR_DEPENDENCIES -type d -print -o -exec sha512sum {} + | sort > "$recipe_outdir-dirdeps"
    fi
    if sha512sum -c "$recipe_outdir/sha512"; then
        echo "$recipe_TIMESTAMP" > "$recipe_outdir/sha512-timestamp"
        set +x
        echo "Already built $recipe_outdir"
        exit
    fi
    rm -rf "$recipe_outdir"
    mkdir -p "$recipe_outdir"
    cd "$recipe_outdir"
}

recipe_finish() {
    cd "$SCRIPT_DIR"
    sha512sum "./$SCRIPT_NAME" > "$recipe_outdir/temp.sha512"
    for recipe_temp in $DEPENDENCIES; do
        sha512sum "$OUT/${recipe_temp##*/}/sha512" >> "$recipe_outdir/temp.sha512"
    done
    if test -n "$DIR_DEPENDENCIES"; then
        sha512sum "$recipe_outdir-dirdeps" >> "$recipe_outdir/temp.sha512"
    fi
    mv "$recipe_outdir/temp.sha512" "$recipe_outdir/sha512"
    echo "$recipe_TIMESTAMP" > "$recipe_outdir/sha512-timestamp"
    set +x
    echo "Built $recipe_outdir"
}

recipe_download() {
    cd "$recipe_DOWNLOADS"
    recipe_temp="${1##*/}"
    if ! sha512sum -c - >&2 <<end
$2  $recipe_temp
end
    then
        wget -O "$recipe_temp" "$1" >&2 || fetch -o "$recipe_temp" "$1" >&2
        sha512sum -c - >&2 <<end
$2  $recipe_temp
end
    fi
    printf "%s" "$PWD/$recipe_temp"
}

recipe_git_download() {
    cd "$recipe_DOWNLOADS"
    recipe_temp="${1##*/}"
    if ! sha512sum -c - >&2 <<end
$3  $recipe_temp-$2.tar.gz
end
    then
        git -C "$recipe_outdir" clone --bare --depth 1 --branch "$2" "$1"
        git -C "$recipe_outdir/$recipe_temp" archive --prefix "$recipe_temp-$2/" -o "$PWD/$recipe_temp-$2.tar.gz" "$2"
        sha512sum -c - >&2 <<end
$3  $recipe_temp-$2.tar.gz
end
    fi
    rm -rf "$recipe_outdir/$recipe_temp"
    printf "%s" "$PWD/$recipe_temp-$2.tar.gz"
}
