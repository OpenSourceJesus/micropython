#!/usr/bin/env python3
"""Batch-translate micropython-lib python-stdlib .py files to objcore C."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from ast2objcore import try_translate_file

MICROPYTHON_TOP = Path(__file__).resolve().parents[2]
DEFAULT_STDLIB = MICROPYTHON_TOP / "lib" / "micropython-lib" / "python-stdlib"
DEFAULT_OUT = Path(__file__).resolve().parent / "stdlib_translated"
DEFAULT_REPORT = Path(__file__).resolve().parent / "stdlib_translate_report.txt"


def iter_stdlib_py_files(stdlib_dir: Path) -> list[Path]:
    return sorted(stdlib_dir.rglob("*.py"))


def relative_slug(stdlib_dir: Path, py_path: Path) -> str:
    rel = py_path.relative_to(stdlib_dir)
    return rel.as_posix().replace("/", "_").removesuffix(".py")


def translate_stdlib(
    stdlib_dir: Path,
    out_dir: Path,
    report_path: Path,
) -> tuple[list[str], list[tuple[str, str]]]:
    ok: list[str] = []
    failed: list[tuple[str, str]] = []

    for py_path in iter_stdlib_py_files(stdlib_dir):
        rel = py_path.relative_to(stdlib_dir).as_posix()
        code, err = try_translate_file(py_path)
        if err is not None:
            failed.append((rel, err))
            continue
        assert code is not None
        out_path = out_dir / f"{relative_slug(stdlib_dir, py_path)}.c"
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(code, encoding="utf-8")
        ok.append(rel)

    lines = [
        "micropython-lib python-stdlib → objcore C translation report",
        f"stdlib: {stdlib_dir}",
        f"output: {out_dir}",
        "",
        f"OK:   {len(ok)}",
        f"FAIL: {len(failed)}",
        "",
    ]
    if ok:
        lines.append("=== translated ===")
        lines.extend(f"  {name}" for name in ok)
        lines.append("")
    if failed:
        lines.append("=== failures ===")
        for name, err in failed:
            lines.append(f"  {name}")
            lines.append(f"    {err}")
        lines.append("")

    report_path.write_text("\n".join(lines), encoding="utf-8")
    return ok, failed


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--stdlib-dir",
        type=Path,
        default=DEFAULT_STDLIB,
        help=f"python-stdlib root (default: {DEFAULT_STDLIB})",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=DEFAULT_OUT,
        help=f"write translated .c files here (default: {DEFAULT_OUT})",
    )
    parser.add_argument(
        "--report",
        type=Path,
        default=DEFAULT_REPORT,
        help=f"write text report here (default: {DEFAULT_REPORT})",
    )
    args = parser.parse_args(argv)

    if not args.stdlib_dir.is_dir():
        print(f"error: stdlib directory not found: {args.stdlib_dir}", file=sys.stderr)
        return 2

    ok, failed = translate_stdlib(args.stdlib_dir, args.out_dir, args.report)
    print(f"translated {len(ok)} file(s), {len(failed)} failed")
    print(f"report: {args.report}")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
