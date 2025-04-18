unset tc_bash_read
read -rep "test" tc_bash_read 2>/dev/null <<end
1
end

unset tc_toolchain
while test -z "$tc_toolchain"; do
    printf "%s" 'Select a toolchain:
    0) none (restore environment)
    1) x86_64-linux-gnu-gcc
Option: '
    read tc_option
    case "$tc_option" in
        0) tc_toolchain=none ;;
        1)
            tc_toolchain=x86_64-linux-gnu-gcc
            tc_toolchain_cc=x86_64-x-linux-gnu-gcc
            ;;
    esac
done

if test "$tc_toolchain" = none; then
    unset CC
    printf "\nClearing CC\n"
    unset PKG_CONFIG
    printf "Clearing PKG_CONFIG\n"
    unset PKG_CONFIG_PATH
    printf "Clearing PKG_CONFIG_PATH\n"
    unset PKG_CONFIG_SYSROOT_DIR
    printf "Clearing PKG_CONFIG_SYSROOT_DIR\n"

    printf "\nEnvironment restored\n"
    return
fi

tc_default="${tc_build_directory:-../sdl3-opengl-out}"
tc_prompt="Build directory [$tc_default]: "
if test "$tc_bash_read"; then read -rep "$tc_prompt" tc_option || { echo "ERROR"; return; } else { printf "$tc_prompt" && read -r tc_option; } || { echo "ERROR"; return; } fi
eval tc_build_directory="$tc_option"
tc_build_directory="${tc_build_directory:-$tc_default}"
test -d "$tc_build_directory" || { echo "ERROR: Directory doesn't exist"; return; }

tc_default="${tc_downloads_directory:-$tc_build_directory}"
tc_prompt="Downloads directory [$tc_default]: "
if test "$tc_bash_read"; then read -rep "$tc_prompt" tc_option || { echo "ERROR"; return; } else { printf "$tc_prompt" && read -r tc_option; } || { echo "ERROR"; return; } fi
eval tc_downloads_directory="$tc_option"
tc_downloads_directory="${tc_downloads_directory:-$tc_default}"
test -d "$tc_downloads_directory" || { echo "ERROR: Directory doesn't exist"; return; }

tc_default="${tc_host_c_compiler:-cc -std=gnu11}"
tc_prompt="Host C compiler [$tc_default]: "
if test "$tc_bash_read"; then read -rep "$tc_prompt" tc_option || { echo "ERROR"; return; } else { printf "$tc_prompt" && read -r tc_option; } || { echo "ERROR"; return; } fi
tc_host_c_compiler="${tc_option:-$tc_default}"

tc_default="${tc_host_cxx_compiler:-c++}"
tc_prompt="Host C++ compiler [$tc_default]: "
if test "$tc_bash_read"; then read -rep "$tc_prompt" tc_option || { echo "ERROR"; return; } else { printf "$tc_prompt" && read -r tc_option; } || { echo "ERROR"; return; } fi
tc_host_cxx_compiler="${tc_option:-$tc_default}"

tc_default="$(nproc 2>/dev/null)"
tc_default="${tc_default:-1}"
tc_default="${tc_parallell_jobs:-$tc_default}"
tc_prompt="Parallell jobs [$tc_default]: "
if test "$tc_bash_read"; then read -rep "$tc_prompt" tc_option || { echo "ERROR"; return; } else { printf "$tc_prompt" && read -r tc_option; } || { echo "ERROR"; return; } fi
tc_parallell_jobs="${tc_option:-$tc_default}"

OUT="$tc_build_directory" DOWNLOADS="$tc_downloads_directory" NUM_CPUS="$tc_parallell_jobs" CC="$tc_host_c_compiler" CXX="$tc_host_cxx_compiler" "toolchain/recipes/$tc_toolchain/${tc_toolchain}_sdl3" || return
tc_out="$(cd -- "$tc_build_directory" && pwd)"

export CC="$tc_out/$tc_toolchain/usr/bin/$tc_toolchain_cc"
printf "\nSetting CC=%s\n" "$CC"
export PKG_CONFIG="$tc_out/pkgconf/bin/pkgconf"
printf "Setting PKG_CONFIG=%s\n" "$PKG_CONFIG"
export PKG_CONFIG_PATH="$tc_out/${tc_toolchain}_sdl3/usr/lib/pkgconfig"
printf "Setting PKG_CONFIG_PATH=%s\n" "$PKG_CONFIG_PATH"
export PKG_CONFIG_SYSROOT_DIR="$tc_out/${tc_toolchain}_sdl3"
printf "Setting PKG_CONFIG_SYSROOT_DIR=%s\n" "$PKG_CONFIG_SYSROOT_DIR"

printf "\nEnvironment setup for toolchain %s\n" "$tc_toolchain"
