#!/usr/bin/env bash

# Build the WebStrada Docker image (Arch Linux based).
#
# The image is multi-stage: a builder stage (archlinux:base-devel) compiles
# textparser from upstream plus the WebStrada sources, and a runtime stage
# (archlinux) keeps only the binaries and shared libraries the server needs.
#
# Usage:
#   ./build_docker.sh [--no-cache] [extra docker build args...]
#
# Environment:
#   IMAGE=name:tag             image name/tag to produce (default webstrada:latest)
#   TEXTPARSER_VERSION=x.y.z   textparser tag to build (default 1.0.8)
#
# Examples:
#   ./build_docker.sh
#   IMAGE=webstrada:dev ./build_docker.sh --no-cache

set -euo pipefail

cd "$(dirname "$0")"

IMAGE="${IMAGE:-webstrada:latest}"
TEXTPARSER_VERSION="${TEXTPARSER_VERSION:-1.0.8}"

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

echo ">> Building image '${IMAGE}' (textparser ${TEXTPARSER_VERSION})"
docker build \
    --build-arg TEXTPARSER_VERSION="${TEXTPARSER_VERSION}" \
    "${EXTRA_ARGS[@]}" \
    -t "${IMAGE}" \
    -f Dockerfile \
    .

echo ">> Done."
echo
echo "Serve your own CFML files by mounting them as the web root (the admin"
echo "panel stays available at /admin/ regardless):"
echo "    docker run --rm -p 8501:8501 \\"
echo "        -v /path/to/webroot:/app/webroot -e WEBROOT=/app/webroot ${IMAGE}"
echo "then visit http://localhost:8501/ and http://localhost:8501/admin/"
echo
echo "Or just run the stock image (web root = /app, includes /admin):"
echo "    docker run --rm -p 8501:8501 ${IMAGE}"
