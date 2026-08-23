#!/usr/bin/env python3
# CF 2021 test-server caveats for .cfm files in tests/cfm/:
# any CFML that throws a runtime error on the test server aborts the HTTP
# response (curl error 18 / http.client.IncompleteRead), so new tests must
# avoid functions this server lacks or that error at runtime (CanSerialize,
# CanDeSerialize, Deserialize, the Set* date functions, FileSeek,
# FileSkipBytes, FileReadBinary(fileObject)) and constructs it rejects
# (#x##|, bare #yes#/#no# literals inside <cfoutput>).
import os
import sys
import tempfile
import subprocess
import urllib.request
import urllib.error
import argparse
import difflib
import http.client
import uuid

# Default configuration
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)

RDS_HOST = os.environ.get("RDS_HOST", "127.0.0.1")
try:
    RDS_PORT = int(os.environ.get("RDS_PORT", "8500"))
except ValueError:
    RDS_PORT = 8500

DEFAULT_RDS_BASE = f"rds://admin:admin@{RDS_HOST}:{RDS_PORT}/app"
DEFAULT_HTTP_BASE = f"http://{RDS_HOST}:{RDS_PORT}"
DEFAULT_CLI_PATH = os.path.join(REPO_ROOT, "bin", "WebStrada-cli")
DEFAULT_TEST_DIR = os.path.join(SCRIPT_DIR, "cfm")
# Relative file paths in the .cfm tests (ImageWrite/ImageRead, FileWrite, ...)
# resolve against the CLI's working directory. Run the CLI from the repo's
# tmp/ dir so those test artifacts land there instead of polluting the repo
# root (e.g. the img_*.png / img_*.jpg files from the image tests).
DEFAULT_CLI_CWD = os.path.join(REPO_ROOT, "tmp")

# ANSI Escape Sequences for Colors
COLOR_GREEN = "\033[92m"
COLOR_RED = "\033[91m"
COLOR_YELLOW = "\033[93m"
COLOR_CYAN = "\033[96m"
COLOR_RESET = "\033[0m"

def print_colored(text, color, file=sys.stdout):
    if file.isatty():
        file.write(f"{color}{text}{COLOR_RESET}\n")
    else:
        file.write(f"{text}\n")
    file.flush()

def main():
    parser = argparse.ArgumentParser(
        description="Verify WebStrada implementation against a local ColdFusion server."
    )
    parser.add_argument(
        "--rds",
        default=DEFAULT_RDS_BASE,
        help=f"RDS base URL (default: {DEFAULT_RDS_BASE})"
    )
    parser.add_argument(
        "--http",
        default=DEFAULT_HTTP_BASE,
        help=f"ColdFusion HTTP base URL (default: {DEFAULT_HTTP_BASE})"
    )
    parser.add_argument(
        "--cli",
        default=DEFAULT_CLI_PATH,
        help=f"Path to WebStrada-cli binary (default: {DEFAULT_CLI_PATH})"
    )
    parser.add_argument(
        "--dir",
        default=DEFAULT_TEST_DIR,
        help=f"Directory containing .cfm files to verify (default: {DEFAULT_TEST_DIR})"
    )
    parser.add_argument(
        "--exact",
        action="store_true",
        help="Perform exact byte-for-byte/string comparison (default is trimmed comparison)"
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Print verbose execution details and full outputs on failure"
    )
    
    args = parser.parse_args()
    
    # Validate CLI binary exists
    if not os.path.exists(args.cli):
        print_colored(f"Error: WebStrada-cli binary not found at '{args.cli}'", COLOR_RED, sys.stderr)
        print_colored("Please build the project first using ./build.sh", COLOR_YELLOW, sys.stderr)
        sys.exit(1)
        
    # Find all .cfm files
    test_dir = args.dir
    if not os.path.exists(test_dir):
        print_colored(f"Error: Test directory '{test_dir}' does not exist.", COLOR_RED, sys.stderr)
        sys.exit(1)
        
    cfm_files = []
    if os.path.isfile(test_dir):
        if test_dir.endswith(".cfm"):
            cfm_files = [test_dir]
            base_dir = os.path.dirname(test_dir) or "."
        else:
            print_colored(f"Error: Specified file '{test_dir}' is not a .cfm file.", COLOR_RED, sys.stderr)
            sys.exit(1)
    else:
        base_dir = test_dir
        for root, _, files in os.walk(test_dir):
            # The shared include library is uploaded as fixed-path helper files
            # (see below); its members (including subdirectories) are not
            # standalone tests.
            rel_root = os.path.relpath(root, test_dir).replace(os.sep, "/")
            if rel_root == "include_lib" or rel_root.startswith("include_lib/"):
                continue
            for file in files:
                if file.endswith(".cfm"):
                    cfm_files.append(os.path.join(root, file))
                    
    if not cfm_files:
        print_colored(f"No .cfm files found in '{test_dir}'", COLOR_YELLOW)
        sys.exit(0)
        
    cfm_files.sort()
    print_colored(f"Found {len(cfm_files)} CFML file(s) to verify.\n", COLOR_CYAN)
    
    passed_count = 0
    failed_count = 0
    error_count = 0
    uploaded_remote_files = []
    uploaded_lib_files = []

    # Upload the shared <cfinclude> library (if any). Tests that use cfinclude
    # reference these files under the web root via include_lib/<path>; they are
    # uploaded with their real (fixed) names so the includes resolve. The test
    # files themselves still use random names to avoid stale compiles.
    include_lib_dir = os.path.join(base_dir, "include_lib")
    if os.path.isdir(include_lib_dir):
        lib_files = []
        for root, _, files in os.walk(include_lib_dir):
            for file in files:
                abs_path = os.path.join(root, file)
                rel_path = os.path.relpath(abs_path, include_lib_dir).replace(os.sep, "/")
                lib_files.append((abs_path, f"include_lib/{rel_path}"))
        lib_files.sort()
        for local_lib, remote_rel in lib_files:
            rds_target = f"{args.rds.rstrip('/')}/{remote_rel}"
            remote_dir = os.path.dirname(remote_rel)
            if remote_dir and remote_dir != ".":
                mkdir_cmd = ["cfrds", "mkdir", f"{args.rds.rstrip('/')}/{remote_dir}"]
                if args.verbose:
                    print(f"[RDS mkdir] Running: {' '.join(mkdir_cmd)}")
                subprocess.run(mkdir_cmd, capture_output=True)
            if args.verbose:
                print(f"[RDS Upload lib] {local_lib} -> {rds_target}")
            try:
                subprocess.run(["cfrds", "upload", local_lib, rds_target], capture_output=True, check=True)
                uploaded_lib_files.append(rds_target)
            except subprocess.CalledProcessError as e:
                print_colored(f"  [ERROR] Failed to upload include library file {local_lib}.", COLOR_RED, sys.stderr)
                if e.stderr:
                    print_colored(f"  Details: {e.stderr.strip()}", COLOR_YELLOW, sys.stderr)
                error_count += 1

    # The shared ColdFusion component library (tests/cfm/components/). Test
    # .cfm files reference these via CreateObject("component", "components/...")
    # (relative to the uploaded test file, which sits at the web root, so the
    # components land at /components/...). Locally the same relative resolution
    # finds them next to the test file (tests/cfm/components/...).
    components_dir = os.path.join(base_dir, "components")
    if os.path.isdir(components_dir):
        comp_files = []
        for root, _, files in os.walk(components_dir):
            for file in files:
                abs_path = os.path.join(root, file)
                rel_path = os.path.relpath(abs_path, components_dir).replace(os.sep, "/")
                comp_files.append((abs_path, f"components/{rel_path}"))
        comp_files.sort()
        for local_comp, remote_rel in comp_files:
            rds_target = f"{args.rds.rstrip('/')}/{remote_rel}"
            remote_dir = os.path.dirname(remote_rel)
            if remote_dir and remote_dir != ".":
                mkdir_cmd = ["cfrds", "mkdir", f"{args.rds.rstrip('/')}/{remote_dir}"]
                if args.verbose:
                    print(f"[RDS mkdir] Running: {' '.join(mkdir_cmd)}")
                subprocess.run(mkdir_cmd, capture_output=True)
            if args.verbose:
                print(f"[RDS Upload comp] {local_comp} -> {rds_target}")
            try:
                subprocess.run(["cfrds", "upload", local_comp, rds_target], capture_output=True, check=True)
                uploaded_lib_files.append(rds_target)
            except subprocess.CalledProcessError as e:
                print_colored(f"  [ERROR] Failed to upload component file {local_comp}.", COLOR_RED, sys.stderr)
    # If base_dir contains Application.cfc or Application.cfm, upload them to RDS root
    for app_file in ["Application.cfc", "Application.cfm"]:
        local_app = os.path.join(base_dir, app_file)
        if os.path.isfile(local_app):
            rds_target = f"{args.rds.rstrip('/')}/{app_file}"
            if args.verbose:
                print(f"[RDS Upload app] {local_app} -> {rds_target}")
            try:
                subprocess.run(["cfrds", "upload", local_app, rds_target], capture_output=True, check=True)
                uploaded_lib_files.append(rds_target)
            except subprocess.CalledProcessError as e:
                print_colored(f"  [ERROR] Failed to upload {app_file}.", COLOR_RED, sys.stderr)
                if e.stderr:
                    print_colored(f"  Details: {e.stderr.strip()}", COLOR_YELLOW, sys.stderr)
                error_count += 1

    try:
        for local_path in cfm_files:
            print(f"Verifying {local_path}...", end="", flush=True)
            
            # 1. Upload to ColdFusion via cfrds. Use a fresh random target
            #    filename per test: ColdFusion caches compiled templates keyed
            #    on the file's mtime (second granularity), so re-uploading a
            #    fixed name (e.g. WebStrada.cfm) can serve a stale compile; a
            #    brand-new name is never cached and always recompiles.
            remote_name = f"tmpfile_{uuid.uuid4().hex}.cfm"
            uploaded_remote_files.append(remote_name)
            rds_target = f"{args.rds.rstrip('/')}/{remote_name}"
            upload_cmd = ["cfrds", "upload", local_path, rds_target]
            
            if args.verbose:
                print(f"\n[RDS Upload] Running: {' '.join(upload_cmd)}")
                
            try:
                upload_res = subprocess.run(upload_cmd, capture_output=True, text=True, check=True)
            except subprocess.CalledProcessError as e:
                print("")  # Newline
                print_colored(f"  [ERROR] Failed to upload {local_path} to ColdFusion server via cfrds.", COLOR_RED, sys.stderr)
                if e.stderr:
                    print_colored(f"  Details: {e.stderr.strip()}", COLOR_YELLOW, sys.stderr)
                else:
                    print_colored(f"  Details: {e.stdout.strip()}", COLOR_YELLOW, sys.stderr)
                error_count += 1
                continue
            except FileNotFoundError:
                print("")  # Newline
                print_colored("  [ERROR] 'cfrds' executable not found in PATH.", COLOR_RED, sys.stderr)
                print_colored("  Please ensure 'cfrds' is installed and available in your environment.", COLOR_YELLOW, sys.stderr)
                sys.exit(1)
                
            # 2. Request output from ColdFusion server via HTTP
            cf_url = f"{args.http.rstrip('/')}/{remote_name}"
            if args.verbose:
                print(f"[HTTP Request] GET {cf_url}")
                
            try:
                with urllib.request.urlopen(cf_url) as response:
                    cf_output = response.read().decode('utf-8', errors='replace').replace('\r\n', '\n').replace('\r', '\n')
            except urllib.error.URLError as e:
                print("")  # Newline
                print_colored(f"  [ERROR] Failed to fetch response from ColdFusion server at {cf_url}.", COLOR_RED, sys.stderr)
                print_colored(f"  Reason: {e}", COLOR_YELLOW, sys.stderr)
                error_count += 1
                continue
            except http.client.IncompleteRead as e:
                # ColdFusion sometimes truncates a response mid-render (a
                # server-side issue); report it instead of crashing the run.
                print("")  # Newline
                print_colored(f"  [ERROR] ColdFusion truncated the HTTP response mid-render ({len(e.partial)} bytes read).", COLOR_RED, sys.stderr)
                print_colored("  This is a ColdFusion server-side rendering issue, not a WebStrada difference.", COLOR_YELLOW, sys.stderr)
                error_count += 1
                continue
            except http.client.HTTPException as e:
                print("")  # Newline
                print_colored(f"  [ERROR] HTTP error while fetching response from ColdFusion server at {cf_url}.", COLOR_RED, sys.stderr)
                print_colored(f"  Reason: {e}", COLOR_YELLOW, sys.stderr)
                error_count += 1
                continue
                
            # 3. Run our WebStrada-cli implementation. The CLI runs with cwd
            #    set to the repo's tmp/ dir (so relative file paths in the cfm
            #    tests land there), so pass the template path as absolute.
            cli_cmd = [args.cli, os.path.abspath(local_path)]
            # Redirect WriteLog output to a per-run writable directory so the
            # default /var/log/WebStrada/ (uncreatable by non-root) never fails.
            cli_env = dict(os.environ)
            log_dir = os.path.join(tempfile.gettempdir(), "WebStrada_verify_logs")
            os.makedirs(log_dir, exist_ok=True)
            cli_env["WEBSTRADA_LOG_DIR"] = log_dir
            if args.verbose:
                print(f"[WebStrada-cli] Running: {' '.join(cli_cmd)}")
                
            try:
                # Read the CLI output as raw bytes and decode like the CF side
                # (UTF-8 with errors='replace') so tests that exercise a
                # non-UTF-8 output charset (e.g. <cfcontent charset=ISO-8859-1>
                # emits 0xE9) still compare correctly instead of crashing on
                # the UTF-8 decode.
                cli_res = subprocess.run(cli_cmd, capture_output=True, text=False, check=True, env=cli_env, cwd=DEFAULT_CLI_CWD)
                our_output = cli_res.stdout.decode('utf-8', errors='replace').replace('\r\n', '\n').replace('\r', '\n')
            except subprocess.CalledProcessError as e:
                print("")  # Newline
                print_colored(f"  [ERROR] WebStrada-cli execution failed for {local_path}.", COLOR_RED, sys.stderr)
                if e.stderr:
                    try:
                        err_text = e.stderr.decode('utf-8', errors='replace')
                    except Exception:
                        err_text = repr(e.stderr)
                    print_colored(f"  Stderr: {err_text.strip()}", COLOR_YELLOW, sys.stderr)
                error_count += 1
                continue
                
            # 4. Compare results
            if args.exact:
                match = (cf_output == our_output)
                cf_disp = cf_output
                our_disp = our_output
            else:
                cf_disp = cf_output.strip()
                our_disp = our_output.strip()
                match = (cf_disp == our_disp)
                
            if match:
                print_colored(" 🟢", COLOR_GREEN)
                passed_count += 1
            else:
                print_colored(" 🔴", COLOR_RED)
                failed_count += 1
                
                # Print outputs or diff
                if args.verbose:
                    print_colored("--- ColdFusion Output (Raw) ---", COLOR_CYAN)
                    print(repr(cf_output))
                    print_colored("--- Our Implementation Output (Raw) ---", COLOR_CYAN)
                    print(repr(our_output))
                    print_colored("-------------------------", COLOR_CYAN)
                
                # Print structured diff
                diff = list(difflib.unified_diff(
                    cf_disp.splitlines(keepends=True),
                    our_disp.splitlines(keepends=True),
                    fromfile='ColdFusion Output',
                    tofile='Our Output'
                ))
                if diff:
                    print_colored("  Differences (ColdFusion [-] vs Our Output [+]):", COLOR_YELLOW)
                    for line in diff:
                        if line.startswith('+'):
                            print_colored(f"    {line.rstrip()}", COLOR_GREEN)
                        elif line.startswith('-'):
                            print_colored(f"    {line.rstrip()}", COLOR_RED)
                        elif line.startswith('^'):
                            print_colored(f"    {line.rstrip()}", COLOR_CYAN)
                        else:
                            print(f"    {line.rstrip()}")
                    print()
    finally:
        # Delete all uploaded remote files to avoid polluting the filesystem.
        for remote_name in uploaded_remote_files:
            delete_target = f"{args.rds.rstrip('/')}/{remote_name}"
            delete_cmd = ["cfrds", "delete", delete_target]
            if args.verbose:
                print(f"\n[Cleanup] Deleting remote file: {delete_target}")
            try:
                subprocess.run(delete_cmd, capture_output=True)
            except Exception:
                pass
        for delete_target in uploaded_lib_files:
            delete_cmd = ["cfrds", "delete", delete_target]
            if args.verbose:
                print(f"\n[Cleanup] Deleting include library file: {delete_target}")
            try:
                subprocess.run(delete_cmd, capture_output=True)
            except Exception:
                pass
                
    # Summary
    print_colored("\n" + "=" * 45, COLOR_CYAN)
    print_colored("             Verification Summary", COLOR_CYAN)
    print_colored("=" * 45, COLOR_CYAN)
    print(f"Total CFML files checked: {len(cfm_files)}")
    print_colored(f"Passed 🟢:               {passed_count}", COLOR_GREEN if passed_count > 0 else COLOR_RESET)
    print_colored(f"Failed 🔴:               {failed_count}", COLOR_RED if failed_count > 0 else COLOR_RESET)
    print_colored(f"Errors (Failed runs):    {error_count}", COLOR_YELLOW if error_count > 0 else COLOR_RESET)
    print_colored("=" * 45, COLOR_CYAN)
    
    if failed_count > 0 or error_count > 0:
        sys.exit(1)
    else:
        sys.exit(0)

if __name__ == "__main__":
    main()
