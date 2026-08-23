# syntax=docker/dockerfile:1

# ----------------------------------------------------------------
# Stage 1: builder - compile toolchain and all sources.
# ----------------------------------------------------------------
FROM ubuntu:26.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# textparser release tag to build from upstream (build-time dependency).
ARG TEXTPARSER_VERSION=1.0.10

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        clang \
        llvm-dev \
        libclang-dev \
        cmake \
        ninja-build \
        git \
        pkg-config \
        python3 \
        libxml2-dev \
        libxslt1-dev \
        libssl-dev \
        zlib1g-dev \
        libjson-c-dev \
        libpcre2-dev \
        libcairo2-dev \
        libjpeg-dev \
        libsqlite3-dev \
        libcurl4-openssl-dev \
        libfcgi-dev \
        libminizip-dev \
        libicu-dev \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# textparser is not shipped in the official repos, so build it from upstream.
# The definition headers must be regenerated before the CMake configure step
# (its add_executable() targets reference the generated *.json.h files).
RUN git clone --depth 1 --branch "${TEXTPARSER_VERSION}" \
        https://github.com/bokic/textparser.git /opt/textparser && \
    (cd /opt/textparser/definitions && ./regenerate.sh) && \
    cmake -S /opt/textparser -B /opt/textparser/build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DTEXTPARSER_VERSION_TAG="${TEXTPARSER_VERSION}" \
        -DBUILD_TESTS=OFF && \
    cmake --build /opt/textparser/build && \
    cmake --install /opt/textparser/build && \
    install -m 755 /opt/textparser/definitions/json2h.py /usr/bin/textparser_json2h.py

COPY . /src

WORKDIR /src
RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build

# ----------------------------------------------------------------
# Stage 1b: admin panel - build the Angular admin UI (webstrada-admin).
# ----------------------------------------------------------------
FROM node:22-alpine AS admin-builder

COPY admin /admin
WORKDIR /admin
RUN npm ci && npm run build -- --base-href=/admin/

# ----------------------------------------------------------------
# Stage 2: runtime - lean image with just what the server needs.
# ----------------------------------------------------------------
FROM ubuntu:26.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libllvm21 \
        libxml2-16 \
        libxslt1.1 \
        libssl3t64 \
        zlib1g \
        libjson-c5 \
        libpcre2-8-0 \
        libcairo2 \
        libjpeg-turbo8 \
        libsqlite3-0 \
        libcurl4t64 \
        libfcgi0t64 \
        libminizip1t64 \
        libicu78 \
        python3 \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# textparser shared library built in the builder stage.
COPY --from=builder /usr/lib/libtextparser.so* /usr/lib/

# Web root: http-dev.py (HTTP front end) plus the FastCGI server binaries.
# The scope SQLite database is created next to bin/WebStrada at runtime, so the
# whole /app tree is owned by the unprivileged service user.
RUN useradd --system --create-home --home-dir /app webstrada && \
    mkdir -p /app/bin /app/tmp && \
    chown -R webstrada:webstrada /app

COPY --from=builder --chown=webstrada:webstrada /src/bin/WebStrada /app/bin/WebStrada
COPY --from=builder --chown=webstrada:webstrada /src/bin/WebStrada-cli /app/bin/WebStrada-cli
COPY --chown=webstrada:webstrada http-dev.py /app/http-dev.py

# Admin panel (webstrada-admin SPA) served at /admin/ plus its CFML API
# endpoints (/admin/api/*.cfm), both resolved against APP_ROOT at runtime.
COPY --from=admin-builder --chown=webstrada:webstrada /admin/dist/webstrada-admin/browser /app/admin/dist/webstrada-admin/browser
COPY --chown=webstrada:webstrada admin/api /app/admin/api

USER webstrada
WORKDIR /app

# 8501: http-dev.py (HTTP front end, auto-starts the FastCGI daemon).
EXPOSE 8501

CMD ["python3", "/app/http-dev.py", "--port", "8501"]
