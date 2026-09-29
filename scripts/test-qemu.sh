#!/usr/bin/env bash
# QEMU boot test: Multiboot header, serial banner, and isa-debug-exit success.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ELF="${1:-$ROOT/build/kernix.elf}"
QEMU="${QEMU:-qemu-system-i386}"
BANNER="KerniX boot OK"
# isa-debug-exit: writing code 0 yields process exit status 1.
EXPECT_EXIT=1

if [[ ! -f "$ELF" ]]; then
    echo "missing kernel ELF: $ELF (run make first)" >&2
    exit 1
fi

python3 - "$ELF" <<'PY'
import struct
import sys

path = sys.argv[1]
MB_MAGIC = 0x1BADB002

with open(path, "rb") as f:
    data = f.read()

if data[:4] != b"\x7fELF":
    sys.exit(f"not an ELF: {path}")
if data[4] != 1:
    sys.exit("expected ELF32")

e_phoff, = struct.unpack_from("<I", data, 0x1C)
e_phentsize, e_phnum = struct.unpack_from("<HH", data, 0x2A)

found = False
for i in range(e_phnum):
    off = e_phoff + i * e_phentsize
    p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align = (
        struct.unpack_from("<IIIIIIII", data, off)
    )
    if p_type != 1:  # PT_LOAD
        continue
    chunk = data[p_offset : p_offset + p_filesz]
    window = chunk[:8192]
    for j in range(0, max(0, len(window) - 12), 4):
        magic, flags, checksum = struct.unpack_from("<III", window, j)
        if magic == MB_MAGIC and (magic + flags + checksum) & 0xFFFFFFFF == 0:
            found = True
            print(f"ok multiboot header at file+0x{p_offset + j:x} (flags=0x{flags:x})")
            break
    if found:
        break

if not found:
    sys.exit("Multiboot header (magic 0x1BADB002) not found in first 8 KiB of any PT_LOAD")
PY

SERIAL="$(mktemp)"
trap 'rm -f "$SERIAL"' EXIT

set +e
"$QEMU" \
    -kernel "$ELF" \
    -serial stdio \
    -display none \
    -no-reboot \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    >"$SERIAL" 2>/dev/null
status=$?
set -e

if [[ "$status" -ne "$EXPECT_EXIT" ]]; then
    echo "FAIL qemu exit: got $status want $EXPECT_EXIT" >&2
    echo "--- serial ---" >&2
    cat "$SERIAL" >&2 || true
    exit 1
fi
echo "ok qemu exit $status"

if ! grep -q "$BANNER" "$SERIAL"; then
    echo "FAIL serial banner '$BANNER' not found" >&2
    echo "--- serial ---" >&2
    cat "$SERIAL" >&2 || true
    exit 1
fi
echo "ok serial banner"

echo "qemu boot test passed"
