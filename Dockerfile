FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    python3 \
    gcc-riscv64-unknown-elf \
    binutils-riscv64-unknown-elf \
    clang-tidy \
    clang-format \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /xoros

CMD ["bash"]
