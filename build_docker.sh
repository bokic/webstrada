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
echo "panel stays available at /webstrada/ regardless):"
echo "    docker run --rm -p 80:80 \\"
echo "        -v /path/to/webroot:/webroot ${IMAGE}"
echo "then visit http://localhost/ and http://localhost/webstrada/"
echo
echo "Or just run the stock image (empty /webroot, admin panel only):"
echo "    docker run --rm -p 80:80 ${IMAGE}"
