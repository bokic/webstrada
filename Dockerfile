
# ----------------------------------------------------------------
# Stage 1: builder - compile toolchain and all sources.
# ----------------------------------------------------------------
FROM ubuntu:26.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# textparser release tag to build from upstream (build-time dependency).
ARG TEXTPARSER_VERSION=1.0.11

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
RUN npm ci && npm run build

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
        nginx \
        curl \
        xz-utils \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# s6-overlay: lightweight C init + service supervisor (no Python needed).
ARG S6_VERSION=3.2.0.2
RUN curl -fsSL "https://github.com/just-containers/s6-overlay/releases/download/v${S6_VERSION}/s6-overlay-noarch.tar.xz" \
        | tar -C / -Jxp && \
    curl -fsSL "https://github.com/just-containers/s6-overlay/releases/download/v${S6_VERSION}/s6-overlay-x86_64.tar.xz" \
        | tar -C / -Jxp

# textparser shared library built in the builder stage.
COPY --from=builder /usr/lib/libtextparser.so* /usr/lib/

# s6-overlay service definitions (webstrada + nginx longrun services).
COPY deploy/s6-rc.d         /etc/s6-overlay/s6-rc.d
# Development-oriented nginx: single worker (see nginx.conf header comment).
COPY deploy/nginx.conf      /etc/nginx/nginx.conf
COPY deploy/nginx-site.conf /etc/nginx/conf.d/default.conf
# run scripts must be executable (--chmod not available without BuildKit).
RUN chmod +x /etc/s6-overlay/s6-rc.d/webstrada/run \
             /etc/s6-overlay/s6-rc.d/nginx/run

# Remove the nginx package's default site so only our config is active.
RUN rm -f /etc/nginx/sites-enabled/default

# WebStrada loads PCRE2 via dlopen("libpcre2-8.so"); the runtime package only
# ships the versioned .so.0 — create the unversioned symlink it expects.
RUN ln -sf /usr/lib/x86_64-linux-gnu/libpcre2-8.so.0 \
           /usr/lib/x86_64-linux-gnu/libpcre2-8.so

# WebStrada binaries and runtime directories.
# The scope SQLite database is created next to bin/WebStrada at runtime, so the
# whole /app tree is owned by the unprivileged service user.
# /webroot is the default CFML site root; mount a volume over it at runtime.
RUN useradd --system --create-home --home-dir /app webstrada && \
    mkdir -p /app/bin /app/tmp /webroot && \
    chown -R webstrada:webstrada /app /webroot

# Allow nginx (www-data) to read the FastCGI socket created by the webstrada user.
RUN usermod -aG webstrada www-data

COPY --from=builder --chown=webstrada:webstrada /src/bin/WebStrada     /app/bin/WebStrada
COPY --from=builder --chown=webstrada:webstrada /src/bin/WebStrada-cli /app/bin/WebStrada-cli

# Admin panel (webstrada-admin SPA) served at /admin/ plus its CFML API
# endpoints (/admin/api/*.cfm), both resolved against APP_ROOT at runtime.
COPY --from=admin-builder --chown=webstrada:webstrada /admin/dist/webstrada-admin/browser /app/admin/dist/webstrada-admin/browser
COPY --chown=webstrada:webstrada admin/api /app/admin/api

WORKDIR /app

# 80: nginx (HTTP)
EXPOSE 80

# s6-overlay /init is PID 1; it brings up all services in s6-rc.d/user/.
# No Python, no supervisor — just two C binaries (nginx + WebStrada) managed
# by a ~3.5 MB pure-C init system.
ENTRYPOINT ["/init"]
