FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive \
    OPAMYES=1 \
    OPAMWITHDOC=false \
    OPAMWITHTEST=false \
    RUST_VERSION=1.91.1 \
    OCAML_VERSION=5.2.0 \
    PICOLIBC_VERSION=1.8.8

# Install system dependencies and RISC-V toolchain in one layer
RUN apt-get update && apt-get install -y \
    # RISC-V toolchain
    gcc-riscv64-unknown-elf \
    binutils-riscv64-unknown-elf \
    libnewlib-dev \
    # Build tools
    build-essential \
    curl \
    git \
    pkg-config \
    # Picolibc dependencies
    meson \
    ninja-build \
    # OCaml/opam and dependencies
    opam \
    libffi-dev \
    # serialport dependency
    libudev-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 --branch ${PICOLIBC_VERSION} \
        https://github.com/picolibc/picolibc.git /tmp/picolibc && \
    cd /tmp/picolibc && \
    mkdir build-riscv && \
    cd build-riscv && \
    ../scripts/do-riscv-configure && \
    ninja && \
    ninja install && \
    cd / && \
    rm -rf /tmp/picolibc

RUN usermod -l builder -d /home/builder -m ubuntu && \
    groupmod -n builder ubuntu

USER builder
WORKDIR /home/builder

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | \
    sh -s -- -y \
    --default-toolchain ${RUST_VERSION} \
    --profile minimal
ENV PATH="/home/builder/.cargo/bin:${PATH}"

RUN opam init -y --disable-sandboxing --compiler=${OCAML_VERSION}

COPY --chown=builder:builder tools/tools.opam /tmp/tools.opam
RUN cd /tmp && \
    eval $(opam env) && \
    opam install . --deps-only -y && \
    rm -rf /tmp/tools.opam && \
    echo 'eval $(opam env)' >> /home/builder/.profile && \
    opam clean -a -c -s --logs -r && \
    rm -rf ~/.opam/download-cache

WORKDIR /workspace

CMD ["/bin/bash", "-l"]
