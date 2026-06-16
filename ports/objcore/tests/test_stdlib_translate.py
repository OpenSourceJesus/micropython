#!/usr/bin/env python3
"""Try translating every micropython-lib python-stdlib .py file via tools/py2c.py."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

_MICROPYTHON_TOP = Path(__file__).resolve().parents[3]
_TOOLS = _MICROPYTHON_TOP / "tools"
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))

from py2c import DEFAULT_STDLIB, transpile_file  # noqa: E402
from translate_stdlib import (  # noqa: E402
    iter_stdlib_py_files,
    relative_slug,
    translate_stdlib,
)

_STDLIB_FILES = iter_stdlib_py_files(DEFAULT_STDLIB) if DEFAULT_STDLIB.is_dir() else []


def _safe_test_name(rel_path: str) -> str:
    return "test_" + rel_path.replace("/", "_").replace(".", "_").replace("-", "_")


class TestStdlibTranslate(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if not DEFAULT_STDLIB.is_dir():
            raise unittest.SkipTest(f"python-stdlib not found at {DEFAULT_STDLIB}")

    def test_stdlib_directory_exists(self) -> None:
        self.assertTrue(DEFAULT_STDLIB.is_dir())
        self.assertGreater(len(_STDLIB_FILES), 0)

    def test_translate_summary(self) -> None:
        """Every stdlib file must transpile without stubs."""
        failures: list[tuple[str, str]] = []
        for py_path in _STDLIB_FILES:
            rel = py_path.relative_to(DEFAULT_STDLIB).as_posix()
            out, err = transpile_file(str(py_path), "/tmp", str(DEFAULT_STDLIB))
            if out is None:
                failures.append((rel, err or "unknown error"))
                continue
            code = Path(out).read_text(encoding="utf-8")
            if "unsupported */" in code or "unsupported */" in code.lower():
                failures.append((rel, "output contains unsupported stub"))

        if failures:
            width = max(len(name) for name, _ in failures)
            lines = [f"{len(failures)} of {len(_STDLIB_FILES)} stdlib file(s) failed:"]
            for name, err in failures:
                lines.append(f"  {name.ljust(width)}  {err}")
            self.fail("\n".join(lines))

    def test_batch_translate_writes_report(self) -> None:
        out = Path(__file__).resolve().parents[1] / "build" / "stdlib_test_out"
        report = Path(__file__).resolve().parents[1] / "build" / "stdlib_test_report.txt"
        ok, failed = translate_stdlib(DEFAULT_STDLIB, out, report)
        self.assertTrue(report.is_file())
        text = report.read_text(encoding="utf-8")
        self.assertIn("FAIL: 0", text.replace("FAIL:   0", "FAIL: 0"))
        self.assertEqual(len(ok), len(_STDLIB_FILES))
        self.assertEqual(failed, [])
        for rel in ok:
            c_path = out / f"{relative_slug(DEFAULT_STDLIB, DEFAULT_STDLIB / rel)}.c"
            self.assertTrue(c_path.is_file(), f"missing output for {rel}")
            body = c_path.read_text(encoding="utf-8")
            self.assertIn("#include \"shivyc_rt.h\"", body)
            self.assertIn("#include \"mp_stdlib_bridge.h\"", body)
            self.assertNotIn("unsupported */", body)


def _make_file_test(py_path: Path):
    rel = py_path.relative_to(DEFAULT_STDLIB).as_posix()

    def test(self: TestStdlibTranslate) -> None:
        out, err = transpile_file(str(py_path), "/tmp", str(DEFAULT_STDLIB))
        if out is None:
            self.fail(err or "transpile failed")
        code = Path(out).read_text(encoding="utf-8")
        self.assertNotIn("unsupported */", code)

    test.__doc__ = f"Transpile python-stdlib/{rel} via py2c."
    test.__name__ = _safe_test_name(rel)
    return test


if _STDLIB_FILES:
    for _py in _STDLIB_FILES:
        setattr(TestStdlibTranslate, _safe_test_name(_py.relative_to(DEFAULT_STDLIB).as_posix()), _make_file_test(_py))


if __name__ == "__main__":
    unittest.main()
