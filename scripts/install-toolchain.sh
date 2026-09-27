#!/usr/bin/env bash
# Install the KerniX toolchain: cross compiler (macOS) or gcc multilib (Linux), NASM, QEMU, GDB.
set -euo pipefail

install_macos() {
    if ! command -v brew >/dev/null 2>&1; then
        echo "Homebrew not found, install it from https://brew.sh" >&2
        exit 1
    fi
    brew install i686-elf-gcc i686-elf-binutils i386-elf-gdb nasm qemu make clang-format
}

install_linux() {
    if ! command -v apt-get >/dev/null 2>&1; then
        echo "Only apt based distributions are supported, install manually:" >&2
        echo "  gcc (with 32-bit multilib), nasm, qemu-system-x86, gdb, make, clang-format" >&2
        exit 1
    fi
    local sudo=""
    [ "$(id -u)" -ne 0 ] && sudo="sudo"
    $sudo apt-get update
    $sudo apt-get install -y --no-install-recommends \
        build-essential gcc-multilib nasm qemu-system-x86 gdb make clang-format
}

case "$(uname -s)" in
    Darwin) install_macos ;;
    Linux) install_linux ;;
    *)
        echo "Unsupported OS: $(uname -s)" >&2
        exit 1
        ;;
esac

echo "Toolchain installed. Run 'make' to build."
