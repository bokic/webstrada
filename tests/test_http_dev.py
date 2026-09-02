#!/usr/bin/env python3
"""Unit tests for http-dev.py request routing.

Covers the web root selection (WEBROOT / --webroot), static vs CFML dispatch,
directory index resolution and the /webstrada routing that always resolves the
admin SPA and its CFML API against APP_ROOT regardless of the web root.
"""

import contextlib
import importlib.util
import io
import os
import sys
import tempfile
import unittest
from unittest import mock

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
http_dev_path = os.path.join(ROOT, "http-dev.py")
_spec = importlib.util.spec_from_file_location("http_dev", http_dev_path)
assert _spec is not None and _spec.loader is not None
http_dev = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(http_dev)


class _FakeHeaders(dict):
    def get(self, key, default=None):
        return dict.get(self, key, default)


def _make_handler(url_path, method="GET", body=b"", headers=None):
    """Build a DevHandler wired to in-memory sockets so _handle() can run
    without a real TCP connection."""
    h = http_dev.DevHandler.__new__(http_dev.DevHandler)
    h.path = url_path
    h.command = method
    h.request_version = "HTTP/1.1"
    h.requestline = f"{method} {url_path} HTTP/1.1"
    h.headers = _FakeHeaders(headers or {})
    h.rfile = io.BytesIO(body)
    h.wfile = io.BytesIO()
    h.client_address = ("127.0.0.1", 12345)
    h.server = mock.Mock()
    h.server.server_address = ("0.0.0.0", 8501)
    return h


def _dispatch(url_path, **kwargs):
    h = _make_handler(url_path, **kwargs)
    h._handle()
    return h


def _fcgi_response(content):
    return b"Status: 200\r\nContent-Type: text/plain\r\n\r\n" + content


class HttpDevRoutingTest(unittest.TestCase):
    def setUp(self):
        self.app_root = tempfile.mkdtemp(prefix="ws-app-")
        self.webroot = tempfile.mkdtemp(prefix="ws-web-")
        # The dev server exposes the admin panel under APP_ROOT/webstrada (the
        # same layout the Docker image deploys at /app/webstrada), so the /webstrada/
        # URL prefix equals the physical path the engine resolves.
        os.makedirs(os.path.join(self.app_root, "webstrada", "dist", "webstrada-admin", "browser"))
        os.makedirs(os.path.join(self.app_root, "webstrada", "api"))
        with open(os.path.join(self.webroot, "index.cfm"), "w") as fh:
            fh.write("<cfoutput>#1+1#</cfoutput>")
        with open(os.path.join(self.webroot, "readme.txt"), "w") as fh:
            fh.write("static")
        os.makedirs(os.path.join(self.webroot, "sub"))
        with open(os.path.join(self.webroot, "sub", "index.cfm"), "w") as fh:
            fh.write("subindex")
        with open(os.path.join(self.app_root, "webstrada", "dist", "webstrada-admin", "browser", "index.html"), "w") as fh:
            fh.write("<base href='/webstrada/'>admin-spa")
        with open(os.path.join(self.app_root, "webstrada", "api", "serverinfo.cfm"), "w") as fh:
            fh.write("serverinfo")

    def tearDown(self):
        for root in (self.app_root, self.webroot):
            for dirpath, dirnames, files in os.walk(root, topdown=False, followlinks=False):
                for name in files:
                    os.unlink(os.path.join(dirpath, name))
                for name in dirnames:
                    p = os.path.join(dirpath, name)
                    if os.path.islink(p):
                        os.unlink(p)
                os.rmdir(dirpath)

    @contextlib.contextmanager
    def _patch_globals(self):
        with mock.patch.object(http_dev, "APP_ROOT", self.app_root), \
             mock.patch.object(http_dev, "WEBROOT", self.webroot), \
             mock.patch.object(http_dev, "ADMIN_DIST",
                               os.path.join(self.app_root, "webstrada", "dist", "webstrada-admin", "browser")):
            yield

    def test_static_file_from_webroot(self):
        with self._patch_globals():
            h = _dispatch("/readme.txt")
        self.assertEqual(h._status_code, 200)
        self.assertEqual(h.wfile.getvalue().split(b"\r\n\r\n", 1)[1], b"static")

    def test_missing_file_404(self):
        with self._patch_globals():
            h = _dispatch("/nope.txt")
        self.assertEqual(h._status_code, 404)

    def test_cfml_request_goes_to_engine_with_webroot(self):
        with self._patch_globals(), mock.patch.object(http_dev, "fcgi_request",
                                                      return_value=_fcgi_response(b"2")) as fcgi:
            h = _dispatch("/index.cfm")
        self.assertEqual(h._status_code, 200)
        self.assertEqual(h.wfile.getvalue().split(b"\r\n\r\n", 1)[1], b"2")
        params = dict(fcgi.call_args[0][0])
        self.assertEqual(params["DOCUMENT_ROOT"], self.webroot)
        self.assertEqual(params["REQUEST_URI"], "/index.cfm")

    def test_directory_root_resolves_index_cfm_and_executes(self):
        with self._patch_globals(), mock.patch.object(http_dev, "fcgi_request",
                                                      return_value=_fcgi_response(b"2")) as fcgi:
            h = _dispatch("/")
        self.assertEqual(h._status_code, 200)
        self.assertEqual(h.wfile.getvalue().split(b"\r\n\r\n", 1)[1], b"2")
        params = dict(fcgi.call_args[0][0])
        self.assertEqual(params["REQUEST_URI"], "/index.cfm")

    def test_subdirectory_index_cfm(self):
        with self._patch_globals(), mock.patch.object(http_dev, "fcgi_request",
                                                      return_value=_fcgi_response(b"subindex")) as fcgi:
            h = _dispatch("/sub/")
        self.assertEqual(h._status_code, 200)
        params = dict(fcgi.call_args[0][0])
        self.assertEqual(params["REQUEST_URI"], "/sub/index.cfm")

    def test_admin_spa_served_from_app_root(self):
        with self._patch_globals():
            h = _dispatch("/webstrada/")
        self.assertEqual(h._status_code, 200)
        self.assertIn(b"admin-spa", h.wfile.getvalue())

    def test_admin_spa_route_falls_back_to_index(self):
        with self._patch_globals():
            h = _dispatch("/webstrada/datasources")
        self.assertEqual(h._status_code, 200)
        self.assertIn(b"admin-spa", h.wfile.getvalue())

    def test_admin_api_cfml_uses_app_root_document_root(self):
        with self._patch_globals(), mock.patch.object(http_dev, "fcgi_request",
                                                      return_value=_fcgi_response(b"serverinfo")) as fcgi:
            h = _dispatch("/webstrada/api/serverinfo.cfm")
        self.assertEqual(h._status_code, 200)
        self.assertEqual(h.wfile.getvalue().split(b"\r\n\r\n", 1)[1], b"serverinfo")
        params = dict(fcgi.call_args[0][0])
        self.assertEqual(params["DOCUMENT_ROOT"], self.app_root)
        # The admin panel is served from APP_ROOT/webstrada (mirroring Docker's
        # /app/webstrada), so the engine gets the browser-accurate /webstrada/ path.
        self.assertEqual(params["REQUEST_URI"], "/webstrada/api/serverinfo.cfm")

    def test_traversal_is_forbidden(self):
        # "/../../etc/passwd" normalizes to "/etc/passwd", which stays inside
        # the webroot prefix (WEBROOT + "/etc/passwd"); it must not resolve to
        # a host file, so a 4xx is expected and no content may leak.
        with self._patch_globals():
            h = _dispatch("/..%2f..%2fetc%2fpasswd")
        self.assertIn(h._status_code, (403, 404))

    def test_admin_cfml_traversal_is_forbidden(self):
        with self._patch_globals():
            h = _dispatch("/webstrada/..%2f..%2fetc%2fpasswd")
        self.assertEqual(h._status_code, 403)

    def test_post_reaches_engine_with_body(self):
        with self._patch_globals(), mock.patch.object(http_dev, "fcgi_request",
                                                      return_value=_fcgi_response(b"posted")) as fcgi:
            h = _dispatch("/index.cfm", method="POST", body=b"a=1",
                          headers={"Content-Length": "3", "Content-Type": "application/x-www-form-urlencoded"})
        self.assertEqual(h._status_code, 200)
        params = dict(fcgi.call_args[0][0])
        self.assertEqual(params["REQUEST_METHOD"], "POST")
        self.assertEqual(params["CONTENT_LENGTH"], "3")
        self.assertEqual(fcgi.call_args[0][1], b"a=1")


class AdminMirrorTest(unittest.TestCase):
    """ensure_admin_mirror() binds the repo's admin/ tree under APP_ROOT/webstrada
    so the dev /webstrada/ URLs match the physical path the engine resolves."""

    def setUp(self):
        self.app_root = tempfile.mkdtemp(prefix="ws-mirror-")
        self.src = os.path.join(self.app_root, "admin")

    def tearDown(self):
        import shutil
        shutil.rmtree(self.app_root, ignore_errors=True)

    def _call(self):
        with mock.patch.object(http_dev, "APP_ROOT", self.app_root):
            http_dev.ensure_admin_mirror()

    def test_binds_existing_admin_subdirs(self):
        os.makedirs(os.path.join(self.src, "api"))
        os.makedirs(os.path.join(self.src, "dist", "webstrada-admin", "browser"))
        self._call()
        self.assertTrue(os.path.islink(os.path.join(self.app_root, "webstrada", "api")))
        self.assertTrue(os.path.islink(os.path.join(
            self.app_root, "webstrada", "dist", "webstrada-admin", "browser")))

    def test_skips_missing_admin_subdirs(self):
        # No admin/ tree at all - the mirror must not raise or fabricate dirs.
        self._call()
        self.assertFalse(os.path.exists(os.path.join(self.app_root, "webstrada", "api")))
        self.assertFalse(os.path.exists(os.path.join(
            self.app_root, "webstrada", "dist", "webstrada-admin", "browser")))

    def test_is_idempotent(self):
        os.makedirs(os.path.join(self.src, "api"))
        self._call()
        first = os.readlink(os.path.join(self.app_root, "webstrada", "api"))
        self._call()
        self.assertEqual(os.readlink(os.path.join(self.app_root, "webstrada", "api")), first)

    def test_repairs_stale_symlink(self):
        # A previous run linked webstrada/api to an admin dir that was later
        # moved (dangling link, or repointed elsewhere); the mirror must recover.
        os.makedirs(os.path.join(self.src, "api"))
        other = os.path.join(self.app_root, "elsewhere")
        os.makedirs(self.app_root + "/elsewhere")
        os.makedirs(os.path.join(self.app_root, "webstrada"))
        stale = os.path.join(self.app_root, "webstrada", "api")
        os.symlink(other, stale)
        self._call()
        self.assertTrue(os.path.islink(stale))
        self.assertEqual(os.path.realpath(stale), os.path.join(self.src, "api"))

    def test_does_not_overwrite_existing_real_dir(self):
        # If a real webstrada/api checkout already exists (not a symlink), it is
        # left untouched.
        os.makedirs(os.path.join(self.src, "api"))
        os.makedirs(os.path.join(self.app_root, "webstrada", "api"))
        with open(os.path.join(self.app_root, "webstrada", "api", "keep.cfm"), "w") as fh:
            fh.write("x")
        self._call()
        self.assertTrue(os.path.isdir(os.path.join(self.app_root, "webstrada", "api")))
        self.assertTrue(os.path.isfile(os.path.join(self.app_root, "webstrada", "api", "keep.cfm")))

    def test_mirror_resolves_for_engine(self):
        os.makedirs(os.path.join(self.src, "api"))
        os.makedirs(os.path.join(self.src, "dist", "webstrada-admin", "browser"))
        with open(os.path.join(self.src, "api", "serverinfo.cfm"), "w") as fh:
            fh.write("x")
        with open(os.path.join(self.src, "dist", "webstrada-admin", "browser", "index.html"), "w") as fh:
            fh.write("y")
        self._call()
        # The engine resolves DOCUMENT_ROOT (/webstrada mirror) + REQUEST_URI.
        self.assertTrue(os.path.isfile(os.path.join(
            self.app_root, "webstrada", "api", "serverinfo.cfm")))
        self.assertTrue(os.path.isfile(os.path.join(
            self.app_root, "webstrada", "dist", "webstrada-admin", "browser", "index.html")))


if __name__ == "__main__":
    unittest.main()
