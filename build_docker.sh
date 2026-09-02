#!/usr/bin/env bash

# Build the WebStrada Docker image (Ubuntu based).
#
# The image is multi-stage: a builder stage (ubuntu:26.04) compiles
# textparser from upstream plus the WebStrada sources, an admin-builder stage
# (node:22-alpine) builds the Angular admin UI, and a runtime stage
# (ubuntu:26.04) keeps only the binaries and shared libraries the server needs.
#
# Usage:
#   ./build_docker.sh [--no-cache] [extra docker build args...]
#
# Environment:
#   IMAGE=name:tag             image name/tag to produce (default webstrada:latest)
#   TEXTPARSER_VERSION=x.y.z   textparser tag to build (default 1.0.11)
#
# Examples:
#   ./build_docker.sh
#   IMAGE=webstrada:dev ./build_docker.sh --no-cache

set -euo pipefail

cd "$(dirname "$0")"

IMAGE="${IMAGE:-webstrada:latest}"
TEXTPARSER_VERSION="${TEXTPARSER_VERSION:-1.0.11}"

# The admin panel's package version is derived from the latest git tag at build
# time (0.0.0 when the checkout has no tags). Passed into the admin-builder
# stage because the Docker context excludes .git.
ADMIN_VERSION="$(git describe --tags --abbrev=0 2>/dev/null || echo 0.0.0)"

# The admin-builder stage builds the Angular SPA natively on the host arch
# (Node under QEMU crashes with SIGILL on cross-arch builds), so pass the host
# platform through. Plain `docker build` does not populate BUILDPLATFORM.
case "$(docker info --format '{{.Architecture}}' 2>/dev/null || uname -m)" in
    x86_64|amd64)                       ADMIN_PLATFORM="linux/amd64" ;;
    aarch64|arm64)                      ADMIN_PLATFORM="linux/arm64" ;;
    *)                                  ADMIN_PLATFORM="linux/amd64" ;;
esac

EXTRA_ARGS=()
if [ "${1:-}" = "--no-cache" ]; then
    EXTRA_ARGS+=(--no-cache)
    shift
fi
if [ $# -gt 0 ]; then
    EXTRA_ARGS+=("$@")
fi

command -v docker >/dev/null 2>&1 || {
    echo "error: docker is required but not installed." >&2
    exit 1
}

echo ">> Building image '${IMAGE}' (textparser ${TEXTPARSER_VERSION}, admin ${ADMIN_VERSION}, admin platform ${ADMIN_PLATFORM})"
docker build \
    --build-arg TEXTPARSER_VERSION="${TEXTPARSER_VERSION}" \
    --build-arg ADMIN_VERSION="${ADMIN_VERSION}" \
    --build-arg ADMIN_PLATFORM="${ADMIN_PLATFORM}" \
    "${EXTRA_ARGS[@]}" \
    -t "${IMAGE}" \
    -f Dockerfile \
    .

echo ">> Done."
echo
echo "Serve your own CFML files by mounting them as the web root (the admin"
echo "panel stays available at /webstrada/ regardless):"
echo "    docker run --rm -p 80:80 \\"
echo "        -v /path/to/webroot:/webroot ${IMAGE}"
echo "then visit http://localhost/ and http://localhost/webstrada/"
echo
echo "Or just run the stock image (empty /webroot, admin panel only):"
echo "    docker run --rm -p 80:80 ${IMAGE}"
