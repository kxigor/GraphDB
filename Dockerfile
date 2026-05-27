# =====================================================================
# GraphDB build image
# =====================================================================
# Reproducible builder for CI and local one-shot builds. Two-stage:
#   1. builder  — compiles all targets with the dev-debug-asan preset.
#   2. runtime  — minimal Debian slim with shared deps + the binary.
#
# Usage:
#   docker build -t graphdb:dev .                       # build
#   docker run --rm -it graphdb:dev                     # run REPL
#   docker build --target builder -t graphdb-ci . \     # CI builder image
#       && docker run --rm graphdb-ci ctest --preset dev-debug-asan
# =====================================================================

ARG UBUNTU_VERSION=24.04
ARG LLVM_VERSION=22
ARG GCC_VERSION=15

# =====================================================================
# Stage 1: builder
# =====================================================================
FROM ubuntu:${UBUNTU_VERSION} AS builder

ARG LLVM_VERSION
ARG GCC_VERSION
ENV DEBIAN_FRONTEND=noninteractive

# Base toolchain + clang tools + Boost + fmt + GTest.
# clang-format and clang-tidy are required for FormatCheck/TidyCheck CTest
# targets even when GCC is the compiler.
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl wget gnupg2 lsb-release software-properties-common \
        cmake ninja-build pkg-config python3 git \
        libboost-all-dev \
        libfmt-dev \
        libgtest-dev libgmock-dev \
    && rm -rf /var/lib/apt/lists/*

# Newer GCC (default Ubuntu 24.04 ships 13/14).
RUN add-apt-repository ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        gcc-${GCC_VERSION} g++-${GCC_VERSION} \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-${GCC_VERSION} 100 \
    && rm -rf /var/lib/apt/lists/*

# Newer LLVM via apt.llvm.org.
RUN wget -q https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && ./llvm.sh ${LLVM_VERSION} all \
    && rm llvm.sh \
    && update-alternatives --install /usr/bin/clang-format clang-format \
        /usr/bin/clang-format-${LLVM_VERSION} 100 \
    && update-alternatives --install /usr/bin/clang-tidy clang-tidy \
        /usr/bin/clang-tidy-${LLVM_VERSION} 100 \
    && rm -rf /var/lib/apt/lists/*

# Ubuntu ships GTest as sources; build & install.
RUN cd /usr/src/googletest \
    && cmake -B build -G Ninja \
    && cmake --build build \
    && cmake --install build

WORKDIR /src
COPY . .

# Default to the asan preset. Override with --build-arg PRESET=ci-tests-release.
ARG PRESET=dev-debug-asan
RUN cmake --preset ${PRESET} \
    && cmake --build --preset ${PRESET}

# Make the artefact paths predictable for stage 2.
RUN mkdir -p /out \
    && (cp -r build/${PRESET}/bin /out/bin || true) \
    && (cp -r build/${PRESET}/lib /out/lib || true)

# =====================================================================
# Stage 2: runtime
# =====================================================================
FROM ubuntu:${UBUNTU_VERSION} AS runtime

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        libboost-filesystem1.83.0 libboost-system1.83.0 libboost-serialization1.83.0 \
        libfmt9 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /out/ /opt/graphdb/

# When `apps/graphdb` is built (i.e. GRAPHDB_BUILD_APPS=ON), this is the
# entry point. Otherwise the image just contains the test binaries.
ENV PATH=/opt/graphdb/bin:${PATH}
CMD ["bash"]
