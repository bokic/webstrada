
# Global build arg (declared before the first FROM so it is visible to the
# FROM --platform below). Node running under QEMU (cross-arch buildx) crashes
# with SIGILL during `npm ci && npm run build`, so the Angular SPA is built
# natively on the host platform. The output is a static SPA (arch-independent),
# so it can be copied into any target architecture unchanged. The automatic
# buildx arg BUILDPLATFORM is force-blanked by plain `docker build`, hence the
# custom name; build_docker.sh passes the host platform, the default covers
# direct `docker build` invocations.
ARG ADMIN_PLATFORM=linux/amd64

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
# The platform is pinned via ADMIN_PLATFORM (see the global arg at the top of
# the file) instead of the buildx automatic BUILDPLATFORM arg.
# ----------------------------------------------------------------
FROM --platform=$ADMIN_PLATFORM node:22-alpine AS admin-builder

# npm's bundled update notifier prints a "New major version of npm available!"
# reminder on every npm run; upgrade to the current major so the reminder stays
# quiet until the next npm release. The notifier itself is left enabled.
RUN npm install -g npm@12.0.2 && npm --version

# The version baked into the admin panel comes from the latest git tag, but the
# Docker context excludes .git, so build_docker.sh derives the tag on the host
# and passes it here as a build arg. When unset, the committed package.json
# version is left untouched.
ARG ADMIN_VERSION=

COPY admin /admin
WORKDIR /admin
RUN node scripts/set-version.mjs "${ADMIN_VERSION}" && npm ci && npm run build

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
# The architecture-specific tarball uses gcc arch names (x86_64 / aarch64),
# not dpkg/Docker names (amd64 / arm64). dpkg-architecture is in dpkg-dev
# which isn't installed at runtime, so map from dpkg --print-architecture.
ARG S6_VERSION=3.2.0.2
RUN case "$(dpkg --print-architecture)" in \
        amd64)  S6_ARCH="x86_64" ;; \
        arm64)  S6_ARCH="aarch64" ;; \
        *)      S6_ARCH="$(dpkg --print-architecture)" ;; \
    esac && \
    curl -fsSL "https://github.com/just-containers/s6-overlay/releases/download/v${S6_VERSION}/s6-overlay-noarch.tar.xz" \
        | tar -C / -Jxp && \
    curl -fsSL "https://github.com/just-containers/s6-overlay/releases/download/v${S6_VERSION}/s6-overlay-${S6_ARCH}.tar.xz" \
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
# ships the versioned .so.0 — create the unversioned symlink it expects. The
# library directory differs per architecture (x86_64 vs aarch64), so resolve it
# from the multiarch dpkg path instead of hardcoding it. dpkg-architecture is
# in dpkg-dev (not present at runtime), so map from dpkg --print-architecture.
RUN case "$(dpkg --print-architecture)" in \
        amd64) LIBDIR="x86_64-linux-gnu" ;; \
        arm64) LIBDIR="aarch64-linux-gnu" ;; \
        *) LIBDIR="$(dpkg --print-architecture)-linux-gnu" ;; \
    esac && \
    ln -sf /usr/lib/${LIBDIR}/libpcre2-8.so.0 \
           /usr/lib/${LIBDIR}/libpcre2-8.so

# WebStrada binaries and runtime directories.
# The scope SQLite database is created next to bin/webstrada at runtime, so the
# whole /app tree is owned by the unprivileged service user.
# /webroot is the default CFML site root; mount a volume over it at runtime.
RUN useradd --system --create-home --home-dir /app webstrada && \
    mkdir -p /app/bin /app/tmp /webroot && \
    chown -R webstrada:webstrada /app /webroot

# Allow nginx (www-data) to read the FastCGI socket created by the webstrada user.
RUN usermod -aG webstrada www-data

COPY --from=builder --chown=webstrada:webstrada /src/bin/webstrada     /app/bin/webstrada
COPY --from=builder --chown=webstrada:webstrada /src/bin/webstrada-cli /app/bin/webstrada-cli

# Admin panel (webstrada-admin SPA) served at /webstrada/ plus its CFML API
# endpoints (/webstrada/api/*.cfm). The tree is deployed under /app/webstrada
# so the browser-facing URL prefix matches the physical layout and the engine
# resolves DOCUMENT_ROOT (/app) + REQUEST_URI (/webstrada/...) directly.
COPY --from=admin-builder --chown=webstrada:webstrada /admin/dist/webstrada-admin/browser /app/webstrada/dist/webstrada-admin/browser
COPY --chown=webstrada:webstrada admin/api /app/webstrada/api

WORKDIR /app

# 80: nginx (HTTP)
EXPOSE 80

# s6-overlay /init is PID 1; it brings up all services in s6-rc.d/user/.
# No Python, no supervisor — just two C binaries (nginx + WebStrada) managed
# by a ~3.5 MB pure-C init system.
ENTRYPOINT ["/init"]
