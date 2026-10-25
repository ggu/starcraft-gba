#!/usr/bin/env python3
"""Compare shared Z7 runtime state on an authenticated normal-input trace."""

from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import re
import subprocess
import sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
ROOT = TOOLS.parent
sys.path.insert(0, str(TOOLS))
import sc64_zp_controller_replay as replay  # noqa: E402

REFERENCE_TARGET = "build/test-sc64-z7-trace-reference"
CANDIDATE_TARGET = "build/test-sc64-z7-trace-count-snapshot"
VARIANTS = {
    "count-snapshot": (REFERENCE_TARGET, CANDIDATE_TARGET),
    "sight-spans": (CANDIDATE_TARGET, "build/test-sc64-z7-trace-sight-spans"),
}
MAX_TICKS = 1_000_000
HASH_RE = re.compile(r"^[0-9a-f]{16}$")


def validate_trace_file(path: Path) -> tuple[dict, bytes]:
    """Authenticate one complete Z7 trace and encode its sparse input tape."""
    trace = json.loads(path.read_text(encoding="utf-8"))
    rows = trace.get("events") if isinstance(trace, dict) else None
    final_kind = (rows[-1].get("kind") if isinstance(rows, list) and rows
                  and isinstance(rows[-1], dict) else None)
    failed_attempt = final_kind in ("lost", "limit")
    plan = replay.validate_trace(
        trace,
        capacity=256,
        profile_name="zerg-z7",
        windowed=True,
        window_ticks=250,
        attempt_prefix=failed_attempt,
    )
    stop_tick = plan["stop_tick"]
    if stop_tick > MAX_TICKS:
        raise ValueError(f"{path}: replay exceeds the {MAX_TICKS}-tick tool bound")
    final = plan["final"]
    for field in ("frame", "screen", "outcome", "started", "minerals", "gas"):
        if type(final.get(field)) is not int:
            raise ValueError(f"{path}: authenticated final event lacks integer {field}")
    events = []
    previous_held = 0
    for zero_based_tick, held in plan["inputs"]:
        host_tick = zero_based_tick + 1
        pressed = held & ~previous_held
        events.append(f"{host_tick} {held} {pressed}\n")
        previous_held = held
    header = (f"{stop_tick} {len(events)} {final['frame']} {final['screen']} "
              f"{final['outcome']} {final['started']} {final['minerals']} {final['gas']}\n")
    return plan, (header + "".join(events)).encode("ascii")


def _safe_stem(path: Path) -> str:
    stem = re.sub(r"[^A-Za-z0-9_.-]+", "_", path.stem).strip("._")
    return stem or "trace"


def _output_paths(output_dir: Path, index: int, trace_path: Path,
                  variant: str) -> tuple[Path, Path, Path, Path]:
    prefix = f"{index:02d}-{_safe_stem(trace_path)}-{variant}"
    return (output_dir / f"{prefix}-reference.txt",
            output_dir / f"{prefix}-candidate.txt",
            output_dir / f"{prefix}-reference.stderr",
            output_dir / f"{prefix}-candidate.stderr")


def _run_binary(binary: Path, tape: bytes, stdout_path: Path, stderr_path: Path) -> None:
    """Replay one validated tape, keeping complete update rows for review."""
    with stdout_path.open("wb") as stdout:
        result = subprocess.run([str(binary)], cwd=ROOT, input=tape, stdout=stdout,
                                stderr=subprocess.PIPE, check=False)
    stderr_path.write_bytes(result.stderr)
    if result.returncode != 0:
        detail = result.stderr.decode("utf-8", errors="replace").strip()
        raise RuntimeError(f"{binary.name} failed ({result.returncode}): {detail}")
    if result.stderr:
        raise RuntimeError(f"{binary.name} wrote diagnostics to {stderr_path}")


def _validate_rows(path: Path, plan: dict) -> tuple[int, str]:
    """Check update ordering and verify the last row against the trace result."""
    digest = hashlib.sha256()
    count = 0
    last: list[str] | None = None
    with path.open("rb") as stream:
        for count, raw in enumerate(stream, 1):
            digest.update(raw)
            try:
                fields = raw.decode("ascii").strip().split()
            except UnicodeDecodeError as exc:
                raise RuntimeError(f"{path}: non-ASCII output at update {count}") from exc
            if len(fields) != 20 or fields[0] != str(count) or not HASH_RE.fullmatch(fields[1]):
                raise RuntimeError(f"{path}: malformed output row at update {count}")
            if fields[13] != "0":
                raise RuntimeError(f"{path}: runtime fault at update {count}: {fields[13]}")
            last = fields
    expected_ticks = plan["stop_tick"]
    if count != expected_ticks or last is None:
        raise RuntimeError(f"{path}: expected {expected_ticks} rows, observed {count}")
    final = plan["final"]
    observed = {
        "frame": int(last[2]), "screen": int(last[3]), "won": int(last[5]),
        "lost": int(last[6]), "outcome": int(last[7]), "started": int(last[8]),
        "fault": int(last[13]), "minerals": int(last[14]), "gas": int(last[15]),
    }
    expected = {
        "frame": final["frame"], "screen": final["screen"],
        "won": int(final["kind"] == "won"), "lost": int(final["kind"] == "lost"),
        "outcome": final["outcome"], "started": final["started"],
        "fault": 0, "minerals": final["minerals"], "gas": final["gas"],
    }
    if observed != expected:
        raise RuntimeError(f"{path}: final row differs from authenticated result: "
                           f"expected {expected}, observed {observed}")
    return count, digest.hexdigest()


def _compare_streams(reference: Path, candidate: Path, expected_rows: int) -> None:
    """Compare every update row and name the first differing tick."""
    tick = 0
    with reference.open("rb") as left, candidate.open("rb") as right:
        for tick, pair in enumerate(itertools.zip_longest(left, right), 1):
            lhs, rhs = pair
            if lhs is None or rhs is None:
                raise RuntimeError(f"row-count mismatch at update {tick}: "
                                   f"reference={'EOF' if lhs is None else 'row'}, "
                                   f"candidate={'EOF' if rhs is None else 'row'}")
            if lhs != rhs:
                raise RuntimeError(f"state parity mismatch at update {tick}: "
                                   f"reference={lhs.decode('ascii', 'replace').strip()} "
                                   f"candidate={rhs.decode('ascii', 'replace').strip()}")
    if tick != expected_rows:
        raise RuntimeError(f"expected {expected_rows} compared updates, observed {tick}")


def run(traces: list[Path], output_dir: Path, variant: str = "count-snapshot") -> int:
    """Validate traces, build both host drivers, and compare full output streams."""
    if not traces:
        raise ValueError("provide at least one authenticated host trace")
    if variant not in VARIANTS:
        raise ValueError(f"unknown optimization variant: {variant}")
    reference_target, candidate_target = VARIANTS[variant]
    validated = [(path, *validate_trace_file(path)) for path in traces]
    output_dir.mkdir(parents=True, exist_ok=True)
    build = subprocess.run(["make", reference_target, candidate_target], cwd=ROOT,
                           check=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           text=True)
    if build.returncode:
        sys.stderr.write(build.stdout)
        sys.stderr.write(build.stderr)
        raise RuntimeError(f"host-driver build failed with status {build.returncode}")
    reference_bin = ROOT / reference_target
    candidate_bin = ROOT / candidate_target
    for index, (trace_path, plan, tape) in enumerate(validated, 1):
        reference, candidate, reference_err, candidate_err = _output_paths(
            output_dir, index, trace_path, variant)
        _run_binary(reference_bin, tape, reference, reference_err)
        _run_binary(candidate_bin, tape, candidate, candidate_err)
        reference_rows, reference_hash = _validate_rows(reference, plan)
        candidate_rows, candidate_hash = _validate_rows(candidate, plan)
        if reference_rows != candidate_rows:
            raise RuntimeError(f"{trace_path}: row count differs between binaries")
        _compare_streams(reference, candidate, plan["stop_tick"])
        final = plan["final"]
        print(f"{trace_path}: {reference_rows} updates identical; "
              f"final frame={final['frame']} screen={final['screen']} "
              f"outcome={final['outcome']} started={final['started']} "
              f"bank={final['minerals']}/{final['gas']}; "
              f"reference sha256={reference_hash}; candidate sha256={candidate_hash}")
        print(f"  rows: {reference} | {candidate}")
    print(f"Parity passed for {len(validated)} authenticated trace(s); "
          "this is a semantic comparison, not a timing measurement.")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("traces", nargs="+", type=Path,
                        help="authenticated Z7 host trace JSON file(s)")
    parser.add_argument("--variant", choices=VARIANTS, default="count-snapshot",
                        help="compare the count cache or source-radius sight spans")
    parser.add_argument("--output-dir", type=Path,
                        default=Path(".local/build/z7-trace-parity"),
                        help="directory for per-update comparison streams")
    args = parser.parse_args(argv)
    try:
        return run(args.traces, args.output_dir, args.variant)
    except (OSError, json.JSONDecodeError, ValueError, RuntimeError) as exc:
        print(f"sc64_z7_trace_parity: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
