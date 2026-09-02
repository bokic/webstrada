#!/usr/bin/env python3
"""Coarse pixel comparison for the ImageDraw*/ImageSet* drawing functions.

ColdFusion renders drawing shapes with Java2D's anti-aliasing-off rasterizer,
which is not pixel-identical to cairo (WebStrada's backend). This script
compares each rendered PNG produced by tests/cfm/draw_pixels.cfm using coarse
metrics instead of an exact byte match:

  - image dimensions must be equal,
  - the set of distinct colors used must be equal,
  - the non-background bounding box must match within `bbox_tol` pixels,
  - the per-color pixel counts must match within `count_tol` percent.

Exact primitives (rect fills, beveled rects, points, axis-aligned lines,
clearrect, XOR) are additionally checked for exact pixel equality.

Usage:
    RDS_HOST=<host> python3 tests/verify_draw_pixels.py
"""
import os
import sys
import subprocess
import tempfile
import urllib.request

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)
RDS_HOST = os.environ.get("RDS_HOST", "127.0.0.1")
RDS_PORT = os.environ.get("RDS_PORT", "8500")
HTTP_BASE = f"http://{RDS_HOST}:{RDS_PORT}"
CLI = os.path.join(REPO_ROOT, "bin", "webstrada-cli")
TEST_CFMS = [os.path.join(SCRIPT_DIR, "draw_pixels.cfm"),
             os.path.join(SCRIPT_DIR, "axis_draw_pixels.cfm")]

BBOX_TOL = 1
COUNT_TOL = 0.20  # +/- 20%

# Primitives whose geometry is axis-aligned/exact and must match pixel-for-pixel.
EXACT = {"dr_r1.png", "dr_r2.png", "dr_p1.png", "dr_l1.png", "dr_l2.png",
         "dr_b1.png", "dr_b2.png", "dr_b3.png", "dr_cl1.png", "dr_t1.png",
         "dr_x1.png", "ax_t1.png", "ax_x1.png", "ax_p1.png", "ax_clr.png"}


def summarize(img):
    from PIL import Image
    im = Image.open(img).convert("RGB")
    w, h = im.size
    colors = {}
    minx, miny, maxx, maxy = w, h, -1, -1
    for y in range(h):
        for x in range(w):
            c = im.getpixel((x, y))
            colors[c] = colors.get(c, 0) + 1
            if c != (0, 0, 0):
                minx, miny, maxx, maxy = min(minx, x), min(miny, y), max(maxx, x), max(maxy, y)
    bbox = (minx, miny, maxx, maxy) if maxx >= 0 else None
    return w, h, bbox, colors


def pixels(img):
    from PIL import Image
    im = Image.open(img).convert("RGB")
    return im.tobytes()

def main():
    for test_cfm in TEST_CFMS:
        run_one(test_cfm)
    print("All drawing images passed coarse pixel comparison.")
    return 0


def run_one(test_cfm):
    base = os.path.splitext(os.path.basename(test_cfm))[0]
    remote = "tmpfile_%s_%s.cfm" % (base, os.urandom(4).hex())
    rds_target = f"rds://admin:admin@{RDS_HOST}:{RDS_PORT}/app/{remote}"
    subprocess.run(["cfrds", "upload", test_cfm, rds_target], check=True)

    # Our CLI runs in a scratch dir so relative ImageWrite paths land locally.
    scratch = tempfile.mkdtemp(prefix="webstrada_draw_")
    subprocess.run([CLI, test_cfm], check=True, cwd=scratch)

    files = [f for f in os.listdir(scratch) if f.endswith(".png")]
    files.sort()
    failures = []
    print(f"{'image':10s} {'dims':7s} {'bbox ours':18s} {'bbox cf':18s}  result")
    for name in files:
        our = os.path.join(scratch, name)
        cf_path = os.path.join(scratch, "cf_" + name)
        urllib.request.urlretrieve(f"{HTTP_BASE}/{name}", cf_path)
        ow, oh, obb, ocol = summarize(our)
        cw, ch, cbb, ccol = summarize(cf_path)
        ok = True
        reason = ""
        if (ow, oh) != (cw, ch):
            ok = False
            reason = "dims"
        elif set(ocol) != set(ccol):
            ok = False
            reason = "colors"
        elif obb != cbb and not (obb and cbb and
                                 abs(obb[0] - cbb[0]) <= BBOX_TOL and abs(obb[1] - cbb[1]) <= BBOX_TOL and
                                 abs(obb[2] - cbb[2]) <= BBOX_TOL and abs(obb[3] - cbb[3]) <= BBOX_TOL):
            ok = False
            reason = f"bbox {obb} vs {cbb}"
        else:
            for col, cnt in ccol.items():
                ocnt = ocol.get(col, 0)
                if abs(ocnt - cnt) > max(2, COUNT_TOL * cnt):
                    ok = False
                    reason = f"count {col} {ocnt} vs {cnt}"
                    break
        if ok and name in EXACT:
            if pixels(our) != pixels(cf_path):
                ok = False
                reason = "exact pixels"
        ocol_s = str(sorted((f"#{c[0]:02x}{c[1]:02x}{c[2]:02x}" for c in ocol), key=lambda s: (len(s), s)))
        status = "PASS" if ok else "FAIL"
        print(f"{name:10s} {ow}x{oh:4d} {str(obb):18s} {str(cbb):18s}  {status} {reason}")
        if not ok:
            failures.append((name, reason))

    print()
    if failures:
        print(f"{len(failures)} image(s) failed coarse comparison: {[f[0] for f in failures]}")
        sys.exit(1)


if __name__ == "__main__":
    sys.exit(main())
