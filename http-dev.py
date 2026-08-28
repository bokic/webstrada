#!/usr/bin/env python3
"""Development web server for WebStrada.

Serves the directory containing this script as the web root. `.cfm` and `.cfc`
requests are forwarded over FastCGI to the WebStrada application server (the
daemon is started automatically on a unix socket inside <webroot>/tmp if it is
not already running), everything else is served as a static file.

Usage:
    python3 http-dev.py [--host 0.0.0.0] [--port 8501] [--workers 4]
"""

import argparse
import html
import mimetypes
import os
import pty
import signal
import socket
import struct
import subprocess
import sys
import threading
import time
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

# APP_ROOT is where the WebStrada binaries, admin panel build and runtime
# sockets live (the directory containing this script). WEBROOT is the directory
# served as the site root; it defaults to APP_ROOT but can be pointed at a
# separate directory (e.g. a mounted host volume) with --webroot / WEBROOT.
APP_ROOT = os.path.dirname(os.path.abspath(__file__))
BINARY = os.path.join(APP_ROOT, "bin", "WebStrada")
SOCK_PATH = os.path.join(APP_ROOT, "tmp", "webstrada-dev.sock")
BACKLOG = 100

WEBROOT = os.environ.get("WEBROOT") or APP_ROOT

# URL path prefixes (e.g. "/" or "/app,/admin") that fall back to the SPA shell
# (<webroot>/index.html) for extension-less GETs that resolve to no file.
# Empty by default, so the stock behavior is unchanged.
SPA_FALLBACK_PREFIXES = []

# Built Angular admin panel (webstrada-admin), served at /admin/ with an SPA
# fallback to index.html. Build it with:
#   (cd admin && npm run build)
ADMIN_DIST = os.path.join(APP_ROOT, "admin", "dist", "webstrada-admin", "browser")

# --- FastCGI protocol constants ---------------------------------------------
FCGI_BEGIN_REQUEST = 1
FCGI_ABORT_REQUEST = 2
FCGI_END_REQUEST = 3
FCGI_PARAMS = 4
FCGI_STDIN = 5
FCGI_STDOUT = 6
FCGI_STDERR = 7
FCGI_RESPONDER = 1
FCGI_MAX_LEN = 65535
REQ_ID = 1


def _encode_len(n):
    if n < 128:
        return bytes([n])
    return struct.pack(">I", 0x80000000 | n)


def _record(ftype, content):
    pad = (-len(content)) % 8
    header = struct.pack(">BBHHBB", 1, ftype, REQ_ID, len(content), pad, 0)
    return header + content + b"\x00" * pad


def _send_all(sock, data):
    while data:
        sent = sock.send(data)
        data = data[sent:]


def _recv_exact(sock, n):
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("FastCGI stream closed unexpectedly")
        buf += chunk
    return buf


def fcgi_request(params, body):
    """Send one FastCGI request over a fresh connection and return the raw
    stdout stream (header block + \r\n\r\n + payload) the daemon produced."""
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        sock.connect(SOCK_PATH)
        sock.settimeout(120)

        begin = struct.pack(">HB5s", FCGI_RESPONDER, 0, b"\x00" * 5)
        _send_all(sock, _record(FCGI_BEGIN_REQUEST, begin))

        pbody = b""
        for key, value in params:
            kb = key.encode()
            vb = value.encode()
            pbody += _encode_len(len(kb)) + _encode_len(len(vb)) + kb + vb
        for i in range(0, len(pbody), FCGI_MAX_LEN):
            _send_all(sock, _record(FCGI_PARAMS, pbody[i:i + FCGI_MAX_LEN]))
        _send_all(sock, _record(FCGI_PARAMS, b""))

        for i in range(0, len(body), FCGI_MAX_LEN):
            _send_all(sock, _record(FCGI_STDIN, body[i:i + FCGI_MAX_LEN]))
        _send_all(sock, _record(FCGI_STDIN, b""))

        stdout = b""
        while True:
            version, ftype, rid, clen, plen, _ = struct.unpack(">BBHHBB", _recv_exact(sock, 8))
            content = _recv_exact(sock, clen)
            if plen:
                _recv_exact(sock, plen)
            if ftype == FCGI_STDOUT:
                stdout += content
            elif ftype == FCGI_STDERR:
                sys.stderr.write(content.decode("utf-8", "replace"))
                sys.stderr.flush()
            elif ftype == FCGI_END_REQUEST:
                break
        return stdout
    finally:
        sock.close()


def _spa_fallback_match(rel):
    """True when rel is covered by one of --spa-fallback's prefixes."""
    for prefix in SPA_FALLBACK_PREFIXES:
        if prefix == "/":
            return True
        if rel == prefix or rel.startswith(prefix.rstrip("/") + "/"):
            return True
    return False


def parse_fcgi_response(data):
    """Split the daemon's stdout into (headers, payload, status). The worker
    emits CGI-style headers terminated by \r\n\r\n."""
    idx = data.find(b"\r\n\r\n")
    if idx < 0:
        return [], data, 200
    headers = []
    status = 200
    for line in data[:idx].decode("latin-1").split("\r\n"):
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        key = key.strip()
        value = value.strip()
        if key.lower() == "status":
            status = int(value.split(" ", 1)[0])
        else:
            headers.append((key, value))
    return headers, data[idx + 4:], status


# --- WebStrada daemon management --------------------------------------------
_daemon_proc = None
_daemon_log = None
DAEMON_LOG = os.path.join(APP_ROOT, "tmp", "webstrada-dev.log")
_daemon_lock = threading.Lock()


def _daemon_output_reader(master_fd, log):
    """Forward the daemon's stdout/stderr to the dev server's terminal live
    (prefixed) while still archiving the raw bytes in the log file.

    The daemon's output is attached to a pty (see ensure_daemon) so its stdio is
    line-buffered and every line reaches this reader immediately; a plain pipe
    would fully-buffer it and nothing would appear until 4 KiB accumulate or the
    daemon exits. Runs in a daemon thread that dies with the process; the log
    handle may be closed by stop_daemon mid-read, so every write is guarded."""
    try:
        while True:
            try:
                data = os.read(master_fd, 4096)
            except OSError:
                break
            if not data:
                break
            if log:
                try:
                    log.write(data)
                    log.flush()
                except (ValueError, OSError):
                    pass
            # The pty translates \n to \r\n (ONLCR); undo that for the terminal.
            text = data.decode("utf-8", "replace").replace("\r\n", "\n")
            if text:
                sys.stdout.write("[WebStrada] " + text)
                sys.stdout.flush()
    finally:
        try:
            os.close(master_fd)
        except OSError:
            pass


def _socket_alive():
    try:
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            s.connect(SOCK_PATH)
            return True
        finally:
            s.close()
    except OSError:
        return False


def ensure_daemon(workers):
    global _daemon_proc, _daemon_log
    with _daemon_lock:
        if _socket_alive():
            return
        if not os.path.isfile(BINARY):
            raise RuntimeError(
                f"WebStrada binary not found at {BINARY}. Run ./build.sh first."
            )
        try:
            os.unlink(SOCK_PATH)
        except OSError:
            pass
        # Daemon stdout/stderr are attached to a pty whose master is read by a
        # live reader thread. The pty makes the daemon's stdio line-buffered, so
        # its output (lifecycle messages, WriteLog, error backtraces) shows up in
        # the dev server's terminal in real time; the same bytes are archived in
        # the log file.
        _daemon_log = open(DAEMON_LOG, "ab")
        master_fd, slave_fd = pty.openpty()
        proc = subprocess.Popen(
            [BINARY, "-n", SOCK_PATH, "-b", str(BACKLOG), "-w", str(workers)],
            start_new_session=True,
            stdin=subprocess.DEVNULL,
            stdout=slave_fd,
            stderr=slave_fd,
        )
        os.close(slave_fd)
        threading.Thread(
            target=_daemon_output_reader,
            args=(master_fd, _daemon_log),
            daemon=True,
        ).start()
        deadline = time.time() + 15
        while time.time() < deadline:
            if _socket_alive():
                _daemon_proc = proc
                return
            if proc.poll() is not None:
                raise RuntimeError(
                    f"WebStrada daemon exited with code {proc.returncode}; "
                    f"see stderr above."
                )
            time.sleep(0.1)
        proc.kill()
        raise RuntimeError("timed out waiting for WebStrada FastCGI socket")


def stop_daemon():
    global _daemon_proc, _daemon_log
    with _daemon_lock:
        proc = _daemon_proc
        _daemon_proc = None
        log = _daemon_log
        _daemon_log = None
        if log is not None:
            try:
                log.close()
            except OSError:
                pass
        if proc is None:
            return
        # The daemon parent sigwaits on SIGINT for its graceful shutdown (it
        # then signals the workers via SIGUSR1 and waitpid()s them). SIGTERM
        # cannot be used: libfcgi installs a SIGTERM handler that merely sets a
        # flag, so the parent would stay alive forever.
        try:
            os.kill(proc.pid, signal.SIGINT)
        except OSError:
            pass
        try:
            proc.wait(timeout=5)
        except Exception:
            try:
                os.killpg(proc.pid, signal.SIGKILL)
            except OSError:
                pass
        try:
            os.unlink(SOCK_PATH)
        except OSError:
            pass


# --- HTTP handler -----------------------------------------------------------
class DevHandler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_GET(self):
        self._handle()

    def do_POST(self):
        self._handle()

    def do_HEAD(self):
        self._handle()

    def _webroot_path(self, url_path):
        norm = os.path.normpath("/" + url_path)
        if norm == ".." or norm.startswith("../"):
            return None
        return WEBROOT + norm

    def _handle(self):
        parsed = urllib.parse.urlsplit(self.path)
        rel = urllib.parse.unquote(parsed.path)
        query = parsed.query

        candidate = self._webroot_path(rel)
        if candidate is None:
            return self._send_error(403, "Forbidden")

        # The engine's own Angular admin panel lives under APP_ROOT/admin.
        if rel == "/admin" or rel.startswith("/admin/"):
            if os.path.splitext(candidate)[1].lower() in (".cfm", ".cfc"):
                norm = os.path.normpath("/admin" + rel[len("/admin"):])
                if norm.startswith("/admin/"):
                    candidate = APP_ROOT + norm
                    return self._serve_cfml(candidate, rel, query, document_root=APP_ROOT)
                return self._send_error(403, "Forbidden")
            return self._serve_admin(rel)

        if os.path.isdir(candidate):
            for index in ("index.cfm", "index.html"):
                ic = os.path.join(candidate, index)
                if os.path.isfile(ic):
                    candidate, rel = ic, rel.rstrip("/") + "/" + index
                    break
            else:
                return self._list_dir(candidate, rel)

        ext = os.path.splitext(candidate)[1].lower()
        if ext in (".cfm", ".cfc"):
            return self._serve_cfml(candidate, rel, query)
        if not os.path.isfile(candidate):
            # Opt-in SPA fallback: extension-less paths that don't resolve to a
            # file are served the SPA shell so client-side routes survive a
            # refresh (--spa-fallback "/", for example). Real files always win,
            # and anything with an extension 404s like a static server would.
            if self.command == "GET" and ext == "" and _spa_fallback_match(rel):
                shell = os.path.join(WEBROOT, "index.html")
                if os.path.isfile(shell):
                    return self._serve_static(shell)
            return self._send_error(404, "Not Found")
        return self._serve_static(candidate)

    def _list_dir(self, fs_path, rel):
        """Render an HTML index of a directory (no index.cfm/index.html)."""
        try:
            entries = os.listdir(fs_path)
        except OSError as exc:
            return self._send_error(500, str(exc))
        dirs = sorted((n for n in entries if os.path.isdir(os.path.join(fs_path, n))),
                      key=str.lower)
        files = sorted((n for n in entries if os.path.isfile(os.path.join(fs_path, n))),
                       key=str.lower)

        base = rel.rstrip("/") + "/"
        rows = ['<li><a href="../">../</a></li>']
        for name in dirs:
            href = html.escape(base + name, quote=True) + "/"
            rows.append(f'<li><a href="{href}">{html.escape(name)}/</a></li>')
        for name in files:
            href = html.escape(base + name, quote=True)
            rows.append(f'<li><a href="{href}">{html.escape(name)}</a></li>')

        title = html.escape(rel)
        body = (
            "<!DOCTYPE html>\n"
            "<html><head><meta charset='utf-8'>"
            f"<title>Index of {title}</title></head>\n"
            f"<body><h1>Index of {title}</h1>\n<ul>\n"
            + "\n".join(rows)
            + "\n</ul>\n</body></html>\n"
        ).encode()
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(body)

    def _serve_admin(self, rel):
        """Serve the built Angular admin panel (SPA) from ADMIN_DIST.

        `/admin` and `/admin/...` map into the admin browser build; asset files
        are served directly and any other path (an SPA route) falls back to
        index.html so client-side navigation survives a browser refresh."""
        sub = rel[len("/admin"):].lstrip("/") if rel != "/admin" else ""
        fs = os.path.normpath(os.path.join(ADMIN_DIST, sub))
        if not fs.startswith(ADMIN_DIST + os.sep) and fs != ADMIN_DIST:
            return self._send_error(403, "Forbidden")
        if sub and os.path.isfile(fs):
            return self._serve_static(fs)
        index = os.path.join(ADMIN_DIST, "index.html")
        if not os.path.isfile(index):
            return self._send_error(404, "Not Found")
        return self._serve_static(index)

    def _serve_static(self, path):
        if not os.path.isfile(path):
            return self._send_error(404, "Not Found")
        try:
            with open(path, "rb") as fh:
                data = fh.read()
        except OSError as exc:
            return self._send_error(500, str(exc))
        self.send_response(200)
        self.send_header("Content-Type", mimetypes.guess_type(path)[0] or "application/octet-stream")
        self.send_header("Content-Length", str(len(data)))
        # Dev server: never cache — hashed SPA assets change on every build and
        # a stale index.html would reference deleted files (blank page).
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(data)

    def _serve_cfml(self, path, rel, query, document_root=None):
        params = [
            ("REQUEST_METHOD", self.command),
            ("REQUEST_URI", rel),
            ("SCRIPT_NAME", rel),
            ("DOCUMENT_ROOT", document_root or WEBROOT),
            ("QUERY_STRING", query),
            ("SERVER_PROTOCOL", self.request_version),
            ("SERVER_NAME", self.headers.get("Host", "localhost").split(":", 1)[0]),
            ("SERVER_PORT", str(getattr(self.server, "server_address")[1])),
            ("SERVER_SOFTWARE", "webstrada-dev"),
            ("REMOTE_ADDR", self.client_address[0]),
        ]
        for header in ("Cookie", "User-Agent", "Accept", "Accept-Language",
                       "Referer", "Connection", "Host"):
            value = self.headers.get(header)
            if value:
                params.append((f"HTTP_{header.upper().replace('-', '_')}", value))

        body = b""
        if self.command == "POST":
            length = int(self.headers.get("Content-Length") or 0)
            if length > 0:
                body = self.rfile.read(length)
            params.append(("CONTENT_LENGTH", str(len(body))))
            content_type = self.headers.get("Content-Type")
            if content_type:
                params.append(("CONTENT_TYPE", content_type))

        try:
            raw = fcgi_request(params, body)
        except OSError as exc:
            return self._send_error(502, f"WebStrada unavailable: {exc}")

        headers, payload, status = parse_fcgi_response(raw)
        self.send_response(status)
        for key, value in headers:
            self.send_header(key, value)
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(payload)

    def _send_error(self, code, message):
        body = f"{code} {message}\n".encode()
        self.send_response(code)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Connection", "close")
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(body)

    def log_message(self, format, *args):
        status = self._status_code if hasattr(self, "_status_code") else "-"
        print(f"[http-dev] {self.client_address[0]} {self.command} {self.path} -> {status}")

    def send_response(self, code, message=None):
        self._status_code = code
        super().send_response(code, message)


def main():
    global WEBROOT

    ap = argparse.ArgumentParser(description="WebStrada development web server")
    ap.add_argument("--host", default="0.0.0.0", help="listen address (default 0.0.0.0)")
    ap.add_argument("--port", type=int, default=8501, help="listen port (default 8501)")
    ap.add_argument("--workers", type=int, default=1, help="WebStrada worker processes (default 1)")
    ap.add_argument("--webroot", default=None,
                    help="directory to serve as the site root "
                         "(default: WEBROOT env var, else this script's directory)")
    ap.add_argument("--spa-fallback", default="",
                    help="comma-separated URL prefixes (e.g. '/' or '/app,/admin') that "
                         "serve <webroot>/index.html for extension-less paths without a file; "
                         "empty by default")
    args = ap.parse_args()

    global SPA_FALLBACK_PREFIXES
    if args.spa_fallback:
        SPA_FALLBACK_PREFIXES = [p.strip() for p in args.spa_fallback.split(",") if p.strip()]

    if args.webroot:
        WEBROOT = os.path.abspath(args.webroot)

    if not os.path.isdir(os.path.join(APP_ROOT, "tmp")):
        os.makedirs(os.path.join(APP_ROOT, "tmp"), exist_ok=True)

    ensure_daemon(args.workers)
    print(f"[http-dev] web root: {WEBROOT}")
    print(f"[http-dev] WebStrada FastCGI socket: {SOCK_PATH}")
    print(f"[http-dev] serving http://{args.host}:{args.port}/ (Ctrl+C to stop)")

    httpd = None
    try:
        httpd = ThreadingHTTPServer((args.host, args.port), DevHandler)
        # Reinstall handlers: background shells leave SIGINT ignored, and a dev
        # server should clean up its daemon on SIGTERM too.
        def _shutdown(signum, frame):
            # Ignore further Ctrl+C / SIGTERM so a second signal cannot
            # interrupt the cleanup below (which would raise a raw traceback).
            signal.signal(signal.SIGINT, signal.SIG_IGN)
            signal.signal(signal.SIGTERM, signal.SIG_IGN)
            raise KeyboardInterrupt

        signal.signal(signal.SIGINT, _shutdown)
        signal.signal(signal.SIGTERM, _shutdown)
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n[http-dev] shutting down")
    finally:
        if httpd is not None:
            httpd.server_close()
        stop_daemon()


if __name__ == "__main__":
    main()
