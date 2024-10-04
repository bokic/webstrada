# syntax=docker/dockerfile:1

# ----------------------------------------------------------------
# Stage 1: builder - compile toolchain and all sources.
# ----------------------------------------------------------------
FROM archlinux:base-devel AS builder

# textparser release tag to build from upstream (build-time dependency).
ARG TEXTPARSER_VERSION=1.0.8

RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm --needed \
        clang llvm cmake ninja git \
        libxml2 libxslt openssl zlib json-c pcre2 cairo libjpeg-turbo sqlite \
        fcgi python pkgconf minizip && \
    pacman -Scc --noconfirm && \
    rm -rf /var/cache/pacman/pkg

# textparser is not shipped in the official repos, so build it from upstream.
# The definition headers must be regenerated before the CMake configure step
# (its add_executable() targets reference the generated *.json.h files).
RUN git clone --depth 1 --branch "${TEXTPARSER_VERSION}" \
        https://github.com/bokic/textparser.git /opt/textparser && \
    (cd /opt/textparser/definitions && ./regenerate.sh) && \
    cmake -S /opt/textparser -B /opt/textparser/build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
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
FROM archlinux AS runtime

RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm --needed \
        llvm libxml2 libxslt openssl zlib json-c pcre2 cairo libjpeg-turbo sqlite \
        fcgi gcc-libs python minizip && \
    pacman -Scc --noconfirm && \
    rm -rf /var/cache/pacman/pkg

# textparser shared libraries + CLI built in the builder stage.
COPY --from=builder /usr/lib/libtextparser.so* /usr/lib/
COPY --from=builder /usr/lib/libtextparser-json.so* /usr/lib/
COPY --from=builder /usr/bin/textparser /usr/local/bin/

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
