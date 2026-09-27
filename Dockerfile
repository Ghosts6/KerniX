# Toolchain image for building and testing KerniX (used by CI, optional locally).
# Source is mounted at /src instead of copied, so the image only changes when the toolchain does.
# amd64 is pinned because gcc-multilib is not available on arm64 (Apple Silicon hosts use emulation).
FROM --platform=linux/amd64 ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential gcc-multilib nasm qemu-system-x86 gdb make clang-format \
        python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

CMD ["make"]
