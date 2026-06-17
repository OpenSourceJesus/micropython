#!/usr/bin/env python3
"""Compile regression tests for transpiled python-stdlib modules.

Each entry in REGRESSION_MODULES must transpile and compile with gcc. Add a
module here whenever a previously-working translation is fixed so it cannot
regress silently when py2c.py changes.
"""

from __future__ import annotations

import subprocess
import sys
import unittest
from pathlib import Path

_MICROPYTHON_TOP = Path(__file__).resolve().parents[3]
_TOOLS = _MICROPYTHON_TOP / "tools"
_OBJCORE = Path(__file__).resolve().parents[1]
_STDLIB_DIR = _OBJCORE / "stdlib_translated"
_BUILD = _OBJCORE / "build"
_GENHDR = _BUILD / "genhdr"

if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))

from py2c import DEFAULT_STDLIB, transpile_file  # noqa: E402
from translate_stdlib import relative_slug  # noqa: E402

# python-stdlib relative paths (as reported by translate_stdlib).
REGRESSION_MODULES = (
    "datetime/datetime.py",
    "struct/struct.py",
    "types/types.py",
    "warnings/warnings.py",
    "pickle/pickle.py",
    "threading/threading.py",
    "collections-defaultdict/collections/defaultdict.py",
    "keyword/keyword.py",
    "os-path/os/path.py",
    "hashlib-sha224/hashlib/_sha224.py",
    "ssl/ssl.py",
    # gcc compile regressions (add a module here when fixing py2c for it):
    "pathlib/pathlib.py",
    "copy/copy.py",
    "quopri/quopri.py",
    "tempfile/tempfile.py",
    "binascii/binascii.py",
)

GCC_FLAGS = (
    "-std=c99",
    "-fPIC",
    "-Os",
    "-DNDEBUG",
    "-Wall",
    "-I.",
    f"-I{_MICROPYTHON_TOP}",
    f"-I{_BUILD}",
    f"-I{_GENHDR}",
    f"-I{_STDLIB_DIR}",
)


def _slug_for(rel_py: str) -> str:
    return relative_slug(DEFAULT_STDLIB, DEFAULT_STDLIB / rel_py)


def _transpile(rel_py: str, out_dir: Path) -> Path:
    py_path = DEFAULT_STDLIB / rel_py
    if not py_path.is_file():
        raise unittest.SkipTest(f"missing source: {py_path}")
    out_dir.mkdir(parents=True, exist_ok=True)
    out, err = transpile_file(str(py_path), str(out_dir), str(DEFAULT_STDLIB))
    if out is None:
        raise AssertionError(err or "transpile failed")
    return Path(out)


def _compile(c_path: Path, obj_dir: Path) -> str:
    obj_dir.mkdir(parents=True, exist_ok=True)
    obj = obj_dir / (c_path.stem + ".o")
    proc = subprocess.run(
        ["gcc", *GCC_FLAGS, "-c", str(c_path), "-o", str(obj)],
        cwd=_OBJCORE,
        capture_output=True,
        text=True,
    )
    if proc.returncode != 0:
        return proc.stderr
    return ""


class TestStdlibCompileRegression(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if not DEFAULT_STDLIB.is_dir():
            raise unittest.SkipTest(f"python-stdlib not found at {DEFAULT_STDLIB}")

    def test_regression_modules_compile(self) -> None:
        out_dir = _BUILD / "stdlib_compile_test"
        obj_dir = out_dir / "obj"
        failures: list[str] = []
        for rel in REGRESSION_MODULES:
            slug = _slug_for(rel)
            try:
                c_path = _transpile(rel, out_dir)
            except unittest.SkipTest as exc:
                failures.append(f"{slug}: {exc}")
                continue
            err = _compile(c_path, obj_dir)
            if err:
                failures.append(f"{slug}:\n{err.rstrip()}")
        if failures:
            self.fail(
                f"{len(failures)} regression module(s) failed to compile:\n\n"
                + "\n\n".join(failures)
            )


if __name__ == "__main__":
    unittest.main()
