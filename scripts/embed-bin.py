#!/usr/bin/env python3
"""Emit a C array for a binary blob: const uint8_t NAME[] / NAME_len."""
import sys

if len(sys.argv) != 4:
    sys.stderr.write(f"usage: {sys.argv[0]} <input.bin> <symbol> <output.c>\n")
    sys.exit(1)

path, name, out = sys.argv[1], sys.argv[2], sys.argv[3]
data = open(path, "rb").read()
with open(out, "w") as f:
    f.write("#include <stdint.h>\n#include <stddef.h>\n\n")
    f.write(f"const uint8_t {name}[] = {{\n")
    for i, b in enumerate(data):
        if i % 12 == 0:
            f.write("    ")
        f.write(f"0x{b:02x},")
        if i % 12 == 11 or i == len(data) - 1:
            f.write("\n")
        else:
            f.write(" ")
    f.write("};\n")
    f.write(f"const size_t {name}_len = {len(data)};\n")
