unset tc_bash_read
read -rep "test" tc_bash_read 2>/dev/null <<end
1
end

unset tc_toolchain
while test -z "$tc_toolchain"; do
    printf "%s" 'Select a toolchain:
    0) none (restore environment)
    1) x86_64-linux-gnu-gcc
    2) aarch64-linux-gnu-gcc
    3) riscv64-linux-gnu-gcc
    4) i686-linux-gnu-gcc
    5) x86_64-w64-mingw32-gcc
    6) aarch64-w64-mingw32-gcc
    7) i686-w64-mingw32-gcc
Option: '
    read tc_option
    case "$tc_option" in
        0) tc_toolchain=none ;;
        1)
            tc_toolchain=x86_64-linux-gnu-gcc
            tc_toolchain_cc=/usr/bin/x86_64-x-linux-gnu-gcc
            ;;
        2)
            tc_toolchain=aarch64-linux-gnu-gcc
            tc_toolchain_cc=/usr/bin/aarch64-x-linux-gnu-gcc
            ;;
        3)
            tc_toolchain=riscv64-linux-gnu-gcc
            tc_toolchain_cc=/usr/bin/riscv64-x-linux-gnu-gcc
            ;;
        4)
            tc_toolchain=i686-linux-gnu-gcc
            tc_toolchain_cc=/usr/bin/i686-x-linux-gnu-gcc
            ;;
        5)
            tc_toolchain=x86_64-w64-mingw32-gcc
            tc_toolchain_cc=/bin/x86_64-w64-mingw32-gcc
            ;;
        6)
            tc_toolchain=aarch64-w64-mingw32-gcc
            tc_toolchain_cc=/bin/aarch64-w64-mingw32-gcc
            ;;
        7)
            tc_toolchain=i686-w64-mingw32-gcc
            tc_toolchain_cc=/bin/i686-w64-mingw32-gcc
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
    unset CMAKE_PREFIX_PATH
    printf "Clearing CMAKE_PREFIX_PATH\n"

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

tc_default="$(getconf NPROCESSORS_ONLN 2>/dev/null || nproc 2>/dev/null)"
tc_default="${tc_default:-1}"
tc_default="${tc_parallel_jobs:-$tc_default}"
tc_prompt="Parallel jobs [$tc_default]: "
if test "$tc_bash_read"; then read -rep "$tc_prompt" tc_option || { echo "ERROR"; return; } else { printf "$tc_prompt" && read -r tc_option; } || { echo "ERROR"; return; } fi
tc_parallel_jobs="${tc_option:-$tc_default}"

OUT="$tc_build_directory" DOWNLOADS="$tc_downloads_directory" NUM_CPUS="$tc_parallel_jobs" CC="$tc_host_c_compiler" CXX="$tc_host_cxx_compiler" "toolchain/recipes/$tc_toolchain/${tc_toolchain}_sysroot" || return
tc_out="$(cd -- "$tc_build_directory" && pwd)"

export CC="$tc_out/$tc_toolchain$tc_toolchain_cc"
printf "\nSetting CC=%s\n" "$CC"
export PKG_CONFIG="$tc_out/pkgconf/bin/pkgconf"
printf "Setting PKG_CONFIG=%s\n" "$PKG_CONFIG"
export PKG_CONFIG_PATH="$tc_out/${tc_toolchain}_sysroot/usr/lib/pkgconfig"
printf "Setting PKG_CONFIG_PATH=%s\n" "$PKG_CONFIG_PATH"
export PKG_CONFIG_SYSROOT_DIR="$tc_out/${tc_toolchain}_sysroot"
printf "Setting PKG_CONFIG_SYSROOT_DIR=%s\n" "$PKG_CONFIG_SYSROOT_DIR"
export CMAKE_PREFIX_PATH="$tc_out/${tc_toolchain}_sysroot/usr"
printf "Setting CMAKE_PREFIX_PATH=%s\n" "$CMAKE_PREFIX_PATH"

printf "\nEnvironment setup for toolchain %s\n" "$tc_toolchain"
