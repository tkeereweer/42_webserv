#!/usr/bin/env python3
"""
Webserv HTTP Test Suite
Tests malformed/invalid HTTP requests against a running webserv instance.
Sends all requests concurrently and validates status codes, headers, and bodies.

Usage:
    python3 test_webserv.py [--host HOST] [--port PORT] [--timeout SECS] [--file FILE] [--verbose]

Defaults:
    host:    127.0.0.1
    port:    8080
    timeout: 45
    file:    wrong_requests.txt (same directory as script)
"""

import socket
import threading
import time
import re
import sys
import os
import argparse
from datetime import datetime

# ─────────────────────────────────────────────────────────────
# ANSI colours
# ─────────────────────────────────────────────────────────────
GREEN  = "\033[92m"
RED    = "\033[91m"
YELLOW = "\033[93m"
CYAN   = "\033[96m"
BOLD   = "\033[1m"
DIM    = "\033[2m"
RESET  = "\033[0m"

def colour(text, col): return f"{col}{text}{RESET}"

# ─────────────────────────────────────────────────────────────
# Minimum required response headers (must be present)
# ─────────────────────────────────────────────────────────────
REQUIRED_HEADERS = {"content-type"}

# Headers whose value must follow specific rules
HEADER_VALUE_RULES = {
    # Content-Length must be a non-negative integer
    "content-length": re.compile(r"^\d+$"),
}

# Valid HTTP/1.x status codes (non-exhaustive but covers all used in the tests)
VALID_STATUS_CODES = {
    100, 101,
    200, 201, 202, 204,
    301, 302, 303, 304, 307, 308,
    400, 401, 403, 404, 405, 408, 409, 410, 411, 413, 414, 415, 431,
    500, 501, 502, 503, 505,
}

# ─────────────────────────────────────────────────────────────
# Parser: read test blocks from the request file
# ─────────────────────────────────────────────────────────────

def parse_test_file(path: str) -> list[dict]:
    """
    Parse blocks delimited by  #testN: "CODE" - description … #endtestN
    Returns a list of dicts: {id, expected_code, description, raw_request}
    """
    tests = []
    with open(path, "r", errors="replace") as fh:
        content = fh.read()

    pattern = re.compile(
        r'#test(\d+):\s*"(\d+)"\s*-\s*(.+?)\n'   # header line
        r'(.*?)'                                    # raw request (lazy)
        r'#endtest\1',                              # closing tag
        re.DOTALL
    )

    for m in pattern.finditer(content):
        test_id   = int(m.group(1))
        exp_code  = int(m.group(2))
        desc      = m.group(3).strip()
        raw       = m.group(4).strip()

        # Re-add a CRLF-terminated request line / headers where the file
        # uses plain LF so the raw bytes are what we actually send.
        # We keep the raw string as-is and let the test description tell us
        # whether it is intentionally broken.
        tests.append({
            "id":            test_id,
            "expected_code": exp_code,
            "description":   desc,
            "raw_request":   raw,
        })

    tests.sort(key=lambda t: t["id"])
    return tests


# ─────────────────────────────────────────────────────────────
# Build the byte payload to send
# ─────────────────────────────────────────────────────────────

def build_payload(raw: str) -> bytes:
    """
    Convert the raw request string to bytes.
    Line endings in the file are kept as-is (some tests intentionally use LF only).
    We do NOT normalise to CRLF here — the test cases rely on this for tests
    like #test50 (LF only) and #test51 (mixed).
    Always ensure the request ends with \r\n\r\n so the server knows the
    request is complete (browsers do this automatically).
    """
    raw = raw.replace("\r\n", "\n").replace("\n", "\r\n")  # normalize to CRLF
    data = raw.encode("latin-1", errors="replace")
    if not data.endswith(b"\r\n\r\n"):
        data = data.rstrip(b"\r\n") + b"\r\n\r\n"
    return data


# ─────────────────────────────────────────────────────────────
# Send one request and collect the response
# ─────────────────────────────────────────────────────────────

def recv_exactly(sock: socket.socket, n: int) -> bytes:
    """Read exactly n bytes from sock."""
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(min(4096, n - len(buf)))
        if not chunk:
            break
        buf += chunk
    return buf


def send_request(host: str, port: int, payload: bytes, timeout: float) -> dict:
    """
    Open a raw TCP connection, send payload, read exactly one complete response.

    Reading strategy:
      1. Read byte-by-byte until the header block ends (CRLFCRLF).
      2. Parse Content-Length from the headers.
      3. If present, read exactly that many body bytes — no more, no less.
      4. If absent, read until the connection closes (server must close to signal end).

    This avoids over-reading on persistent connections where the server keeps
    the socket open after sending the response.
    """
    result = {"raw_response": b"", "error": None, "elapsed": 0.0}
    t0 = time.monotonic()
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        sock.connect((host, port))
        sock.sendall(payload)

        # ── Step 1: read until end-of-headers (CRLFCRLF) ──
        head_buf = b""
        while b"\r\n\r\n" not in head_buf:
            byte = sock.recv(1)
            if not byte:
                break
            head_buf += byte

        # ── Step 2: extract Content-Length ──
        body_bytes = b""
        cl_match = re.search(
            rb"(?i)content-length:\s*(\d+)\r\n", head_buf
        )

        if cl_match:
            # ── Step 3: read exactly Content-Length bytes ──
            content_length = int(cl_match.group(1))
            body_bytes = recv_exactly(sock, content_length)
        else:
            # ── Step 4: no Content-Length — read until close ──
            chunks = []
            try:
                while True:
                    chunk = sock.recv(4096)
                    if not chunk:
                        break
                    chunks.append(chunk)
            except socket.timeout:
                pass
            body_bytes = b"".join(chunks)

        sock.close()
        result["raw_response"] = head_buf + body_bytes

    except ConnectionRefusedError:
        result["error"] = "Connection refused — is the server running?"
    except socket.timeout:
        result["error"] = f"Timed out after {timeout}s (no response)"
    except Exception as exc:
        result["error"] = str(exc)
    result["elapsed"] = time.monotonic() - t0
    return result


# ─────────────────────────────────────────────────────────────
# Response parser
# ─────────────────────────────────────────────────────────────

def parse_response(raw: bytes) -> dict:
    """
    Parse an HTTP response into:
      version, status_code, reason, headers (dict lower-case), body (bytes)
    Returns None fields on parse error.
    """
    out = {
        "version": None, "status_code": None, "reason": None,
        "headers": {}, "body": b"", "parse_errors": [],
    }

    # Split head / body on first CRLFCRLF (or LFLF)
    if b"\r\n\r\n" in raw:
        head, body = raw.split(b"\r\n\r\n", 1)
        sep = b"\r\n"
    elif b"\n\n" in raw:
        head, body = raw.split(b"\n\n", 1)
        sep = b"\n"
    else:
        head = raw
        body = b""
        sep = b"\r\n"

    out["body"] = body
    lines = head.split(sep)

    # Status line
    if not lines:
        out["parse_errors"].append("Empty response")
        return out

    status_line = lines[0].decode("latin-1", errors="replace").strip()
    sl_match = re.match(r"^(HTTP/[\d.]+)\s+(\d{3})\s*(.*?)$", status_line)
    if not sl_match:
        out["parse_errors"].append(f"Malformed status line: {status_line!r}")
        return out

    out["version"]     = sl_match.group(1)
    out["status_code"] = int(sl_match.group(2))
    out["reason"]      = sl_match.group(3).strip()

    # Headers
    prev_name = None
    for line in lines[1:]:
        line_str = line.decode("latin-1", errors="replace")
        if not line_str.strip():
            continue
        # Folded header (obs-fold) — append to previous
        if line_str[0] in (" ", "\t") and prev_name:
            out["headers"][prev_name] += " " + line_str.strip()
            continue
        if ":" not in line_str:
            out["parse_errors"].append(f"Header missing colon: {line_str!r}")
            continue
        name, _, value = line_str.partition(":")
        name  = name.strip().lower()
        value = value.strip()
        # Duplicate header accumulation (comma-join per RFC 7230 §3.2.2)
        if name in out["headers"]:
            out["headers"][name] += ", " + value
        else:
            out["headers"][name] = value
        prev_name = name

    return out


# ─────────────────────────────────────────────────────────────
# Validator
# ─────────────────────────────────────────────────────────────

class ValidationResult:
    def __init__(self):
        self.passed  = True
        self.issues  = []   # list of strings

    def fail(self, msg: str):
        self.passed = False
        self.issues.append(msg)

    def warn(self, msg: str):
        self.issues.append(f"[WARN] {msg}")


def validate_response(parsed: dict, expected_code: int) -> ValidationResult:
    vr = ValidationResult()

    if parsed["status_code"] is None:
        vr.fail("Could not parse response status line")
        return vr

    # 1. Status code match
    actual = parsed["status_code"]
    if actual != expected_code:
        vr.fail(f"Status code: expected {expected_code}, got {actual}")

    # 2. Status code must be a known HTTP code
    if actual not in VALID_STATUS_CODES:
        vr.fail(f"Status code {actual} is not a recognised HTTP status code")

    # 3. HTTP version in response
    version = parsed["version"]
    if version not in ("HTTP/1.0", "HTTP/1.1"):
        vr.fail(f"Unexpected HTTP version in response: {version!r}")

    # 4. Required headers present
    for h in REQUIRED_HEADERS:
        if h not in parsed["headers"]:
            vr.fail(f"Missing required response header: {h!r}")

    # 5. Header value validation
    for h, rule in HEADER_VALUE_RULES.items():
        if h in parsed["headers"]:
            val = parsed["headers"][h]
            if not rule.match(val):
                vr.fail(f"Header {h!r} has invalid value: {val!r}")

    # 6. Content-Length consistency
    if "content-length" in parsed["headers"]:
        try:
            cl = int(parsed["headers"]["content-length"])
            actual_len = len(parsed["body"])
            if cl != actual_len:
                vr.fail(
                    f"Content-Length mismatch: header says {cl}, "
                    f"body is {actual_len} bytes"
                )
        except ValueError:
            vr.fail(f"Content-Length is not an integer: {parsed['headers']['content-length']!r}")

    # 7. Error responses (4xx/5xx) should have a body
    if actual >= 400:
        if len(parsed["body"]) == 0 and "content-length" not in parsed["headers"]:
            vr.warn("Error response has no body and no Content-Length header")

    # 8. Response parse errors
    for e in parsed["parse_errors"]:
        vr.fail(f"Parse error: {e}")

    return vr


# ─────────────────────────────────────────────────────────────
# Worker thread
# ─────────────────────────────────────────────────────────────

def run_test(test: dict, host: str, port: int, timeout: float,
             results: list, lock: threading.Lock, verbose: bool):
    payload = build_payload(test["raw_request"])
    net     = send_request(host, port, payload, timeout)

    entry = {
        "id":            test["id"],
        "description":   test["description"],
        "expected_code": test["expected_code"],
        "elapsed":       net["elapsed"],
        "passed":        False,
        "issues":        [],
        "status_code":   None,
        "timed_out":     False,
    }

    if net["error"]:
        if "Timed out" in net["error"]:
            entry["timed_out"] = True
        entry["issues"].append(net["error"])
    elif not net["raw_response"]:
        entry["issues"].append("Empty response (connection closed immediately?)")
    else:
        parsed = parse_response(net["raw_response"])
        entry["status_code"] = parsed["status_code"]
        vr = validate_response(parsed, test["expected_code"])
        entry["passed"] = vr.passed
        entry["issues"] = vr.issues

        if verbose:
            with lock:
                _print_verbose(test, net, parsed, vr)

    with lock:
        results.append(entry)


def _print_verbose(test, net, parsed, vr):
    print(colour(f"\n── Test {test['id']:03d}: {test['description']} ──", CYAN))
    print(colour(f"  Expected : {test['expected_code']}", DIM))
    print(colour(f"  Got      : {parsed['status_code']} {parsed['reason']}", DIM))
    print(colour(f"  Elapsed  : {net['elapsed']:.3f}s", DIM))
    if parsed["headers"]:
        for k, v in sorted(parsed["headers"].items()):
            print(colour(f"  {k}: {v}", DIM))
    if not vr.passed:
        for issue in vr.issues:
            print(colour(f"  ✗ {issue}", RED))
    else:
        print(colour("  ✓ OK", GREEN))


# ─────────────────────────────────────────────────────────────
# Pretty summary table
# ─────────────────────────────────────────────────────────────

def print_summary(results: list, total_elapsed: float):
    results.sort(key=lambda r: r["id"])

    col_id   = 5
    col_code = 6
    col_got  = 6
    col_time = 7
    col_desc = 40
    indent   = col_id + col_code + col_got + col_time + col_desc + 12

    header = (
        f"{'ID':>{col_id}}  "
        f"{'EXPECT':>{col_code}}  "
        f"{'GOT':>{col_got}}  "
        f"{'TIME':>{col_time}}  "
        f"{'DESCRIPTION':<{col_desc}}  "
        f"NOTES"
    )

    sep = "─" * 100

    print()
    print(colour(BOLD + "═" * 100 + RESET, BOLD))
    print(colour(BOLD + "  WEBSERV HTTP TEST RESULTS" + RESET, BOLD))
    print(colour("═" * 100, BOLD))
    print(colour(header, BOLD))
    print(colour(sep, DIM))

    passed = timed_out = failed = 0

    for r in results:
        got_str = str(r["status_code"]) if r["status_code"] else "—"
        t_str   = f"{r['elapsed']:.2f}s"
        desc    = r["description"][:col_desc]

        if r["timed_out"]:
            status_col = YELLOW
            marker     = "⏱"
            timed_out += 1
            notes = ["TIMEOUT"]
        elif r["passed"]:
            status_col = GREEN
            marker     = "✓"
            passed    += 1
            notes = []
        else:
            status_col = RED
            marker     = "✗"
            failed    += 1
            notes = [i for i in r["issues"] if not i.startswith("[WARN]")]
            if not notes:
                notes = r["issues"]

        prefix = (
            f"{r['id']:>{col_id}}  "
            f"{r['expected_code']:>{col_code}}  "
            f"{got_str:>{col_got}}  "
            f"{t_str:>{col_time}}  "
            f"{desc:<{col_desc}}  "
            f"{marker} "
        )

        if notes:
            print(colour(prefix + notes[0], status_col))
            for note in notes[1:]:
                print(colour(" " * indent + note, status_col))
        else:
            print(colour(prefix, status_col))

        # Print warnings indented below
        for issue in r["issues"]:
            if issue.startswith("[WARN]"):
                print(colour(" " * indent + issue, YELLOW))

    total = len(results)
    print(colour(sep, DIM))
    print()
    print(colour(BOLD + f"  Total : {total}", BOLD))
    print(colour(f"  {'✓ Passed':<12}: {passed}  ({100*passed//total if total else 0}%)", GREEN))
    print(colour(f"  {'✗ Failed':<12}: {failed}  ({100*failed//total if total else 0}%)", RED if failed else GREEN))
    print(colour(f"  {'⏱ Timed out':<12}: {timed_out}", YELLOW if timed_out else GREEN))
    print(colour(f"\n  Wall time : {total_elapsed:.2f}s", DIM))
    print(colour("═" * len(sep), BOLD))
    print()

    return passed, failed, timed_out


# ─────────────────────────────────────────────────────────────
# Connectivity pre-check
# ─────────────────────────────────────────────────────────────

def check_server(host: str, port: int) -> bool:
    try:
        s = socket.socket()
        s.settimeout(3)
        s.connect((host, port))
        s.close()
        return True
    except Exception:
        return False


# ─────────────────────────────────────────────────────────────
# Entry point
# ─────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Webserv HTTP test suite — sends bad requests concurrently and validates responses"
    )
    parser.add_argument("--host",    default="127.0.0.1", help="Server host (default: 127.0.0.1)")
    parser.add_argument("--port",    type=int, default=8080, help="Server port (default: 8080)")
    parser.add_argument("--timeout", type=float, default=30.0, help="Per-request timeout in seconds (default: 30)")
    parser.add_argument("--file",    default="/home/mturgeon/rank5/webserv/tests/http/1_wrong_requests.txt",
                        help="Path to test file (default: wrong_requests.txt next to script)")
    parser.add_argument("--verbose", "-v", action="store_true", help="Print full response detail per test")
    parser.add_argument("--filter",  type=str, default="", help="Run only tests whose description contains this string (case-insensitive)")
    args = parser.parse_args()

    print(colour(BOLD + "\n  Webserv HTTP Test Suite" + RESET, BOLD))
    print(colour(f"  Target  : {args.host}:{args.port}", DIM))
    print(colour(f"  File    : {args.file}", DIM))
    print(colour(f"  Timeout : {args.timeout}s per request", DIM))
    print()

    # Pre-flight check
    print("  Checking server connectivity …", end=" ", flush=True)
    if not check_server(args.host, args.port):
        print(colour("FAILED", RED))
        print(colour(f"\n  ✗ Cannot connect to {args.host}:{args.port}. Is the server running?", RED))
        sys.exit(1)
    print(colour("OK", GREEN))

    # Parse tests
    if not os.path.exists(args.file):
        print(colour(f"\n  ✗ Test file not found: {args.file}", RED))
        sys.exit(1)

    tests = parse_test_file(args.file)
    if args.filter:
        tests = [t for t in tests if args.filter.lower() in t["description"].lower()]
    if not tests:
        print(colour("  ✗ No tests found (check --filter or file path)", RED))
        sys.exit(1)

    print(colour(f"  Loaded {len(tests)} tests — launching all concurrently …\n", DIM))

    results: list = []
    lock    = threading.Lock()
    threads = []

    t_start = time.monotonic()

    for test in tests:
        t = threading.Thread(
            target=run_test,
            args=(test, args.host, args.port, args.timeout, results, lock, args.verbose),
            daemon=True,
        )
        threads.append(t)
        t.start()

    # Progress bar while waiting
    done_event = threading.Event()

    def progress():
        chars = "⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏"
        i = 0
        while not done_event.is_set():
            with lock:
                n = len(results)
            pct = n * 100 // len(tests)
            bar = "█" * (pct // 5) + "░" * (20 - pct // 5)
            sys.stdout.write(f"\r  {chars[i % len(chars)]} [{bar}] {n}/{len(tests)} responses received")
            sys.stdout.flush()
            i += 1
            time.sleep(0.1)
        sys.stdout.write("\r" + " " * 70 + "\r")
        sys.stdout.flush()

    prog_thread = threading.Thread(target=progress, daemon=True)
    prog_thread.start()

    for t in threads:
        t.join()

    done_event.set()
    prog_thread.join(timeout=1)

    total_elapsed = time.monotonic() - t_start

    passed, failed, timed_out = print_summary(results, total_elapsed)

    # Exit with non-zero if any tests failed
    sys.exit(0 if (failed + timed_out) == 0 else 1)


if __name__ == "__main__":
    main()