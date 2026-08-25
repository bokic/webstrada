# WebStrada

A high-performance CFML (ColdFusion Markup Language) runtime engine and application server. Compiles `.cfm`/`.cfc` templates on-the-fly into native machine code via **LLVM JIT compilation** and serves requests through the **FastCGI** protocol.

## Architecture

`WebStrada` is a from-scratch re-implementation of a ColdFusion server. Instead of interpreting CFML, it parses templates into an AST using [TextParser](https://github.com/anomalyco/textparser), generates LLVM IR, and JIT-compiles to native code for execution.

```
CFML Source → TextParser AST → LLVM IR → MCJIT → Native Code → Execution
```

## Building

### Prerequisites

- CMake >= 3.16
- Ninja build system
- C++23 compiler (GCC or Clang)
- LLVM development libraries
- libfcgi (FastCGI)
- libminizip (for `<cfzip>` archive support; Debian/Arch package: `minizip`)

### Build

```bash
git submodule update --init --recursive
./build.sh          # GCC (default)
# or
./build_clang.sh    # Clang
# or
./build-release.sh  # Size-optimized release build (see below)
```

### Docker image (Ubuntu)

`build_docker.sh` builds a multi-stage Ubuntu Docker image: a `builder`
stage (`ubuntu:26.04`) compiles the `textparser` dependency from
upstream plus the project sources, an `admin-builder` stage (`node:22-alpine`)
builds the Angular admin UI, and a `runtime` stage (`ubuntu:26.04`) keeps
only the binaries and shared libraries the server needs. The image web root is
`/app`; `http-dev.py` serves HTTP on port `8501` and auto-starts the FastCGI
daemon, which can also be run directly on the TCP socket `:6000`.

```bash
./build_docker.sh                       # builds webstrada:latest
docker run --rm -p 8501:8501 webstrada  # dev web server
```

Environment overrides: `IMAGE=name:tag` (default `webstrada:latest`) and
`TEXTPARSER_VERSION=x.y.z` (default `1.0.11`).

## Usage

### CLI Tool

```bash
# Execute CFML from stdin without loading Application.cfm/cfc
echo '<cfdump var="hello" />' | ./bin/WebStrada-cli --stdin

# Print LLVM IR instead of executing
echo '<cfdump var="hello" />' | PRINT_AST= ./bin/WebStrada-cli --stdin

# Compile and run a file (loads Application.cfm/cfc if present)
./bin/WebStrada-cli mytemplate.cfm
```

### Application Server

```bash
./bin/WebStrada -n :6000 -b 100 -w 4
```

Options:
- `-n <socket>` — FastCGI socket name (default `:6000`)
- `-b <backlog>` — connection backlog (default `100`)
- `-w <workers>` — number of worker processes (default `1`)

### Development Web Server

`http-dev.py` is a single-file Python dev server (stdlib only) for quick
manual testing without nginx. It listens on TCP port `8501` (all interfaces)
by default, serves a web root and forwards `.cfm`/`.cfc` requests to the
WebStrada FastCGI daemon — which it starts automatically on a unix socket in
`<app-root>/tmp/` (reusing an already running daemon if present) — while
serving everything else as a static file.

By default the web root is the directory containing the script, but it can be
pointed at any directory (e.g. a mounted host volume) with `--webroot` or the
`WEBROOT` environment variable. The built admin panel (`admin/dist/...`) and
its `/admin/api/*.cfm` endpoints are always served from the app root at
`/admin/`, independent of the web root.

```bash
python3 http-dev.py [--host 0.0.0.0] [--port 8501] [--workers 4] [--webroot DIR]
WEBROOT=/path/to/site python3 http-dev.py --port 8501
```

## Docker

The `webstrada:latest` image bundles the WebStrada binaries, the textparser
libraries and the built admin panel. Run it with your CFML site mounted as the
web root:

```bash
# cd to root of your cfml project
cd {to root of your CFML application}

# Serve a host directory as the web root (port 8501):
docker run --rm -p 8501:8501 -v .:/app/webroot -e WEBROOT=/app/webroot bokic78/webstrada:latest

# then visit http://localhost:8501/ (your pages) and http://localhost:8501/admin/ (admin panel)
```

## License

MIT
