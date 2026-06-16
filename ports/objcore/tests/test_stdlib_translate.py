#!/usr/bin/env python3
"""Try translating every micropython-lib python-stdlib .py file to objcore C."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

# Import from ports/objcore/
_OBJCORE = Path(__file__).resolve().parents[1]
if str(_OBJCORE) not in sys.path:
    sys.path.insert(0, str(_OBJCORE))

from ast2objcore import try_translate_file  # noqa: E402
from translate_stdlib import (  # noqa: E402
    DEFAULT_STDLIB,
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
        """Single failure listing every file that does not transpile."""
        failures: list[tuple[str, str]] = []
        successes: list[str] = []
        for py_path in _STDLIB_FILES:
            rel = py_path.relative_to(DEFAULT_STDLIB).as_posix()
            _code, err = try_translate_file(py_path)
            if err is None:
                successes.append(rel)
            else:
                failures.append((rel, err))

        if failures:
            width = max(len(name) for name, _ in failures)
            lines = [f"{len(failures)} of {len(_STDLIB_FILES)} stdlib file(s) failed to translate:"]
            for name, err in failures:
                lines.append(f"  {name.ljust(width)}  {err}")
            if successes:
                lines.append("")
                lines.append(f"{len(successes)} succeeded:")
                for name in successes:
                    lines.append(f"  {name}")
            self.fail("\n".join(lines))

    def test_batch_translate_writes_report(self) -> None:
        out = _OBJCORE / "build" / "stdlib_test_out"
        report = _OBJCORE / "build" / "stdlib_test_report.txt"
        ok, failed = translate_stdlib(DEFAULT_STDLIB, out, report)
        self.assertTrue(report.is_file())
        text = report.read_text(encoding="utf-8")
        self.assertIn("FAIL:", text)
        self.assertEqual(len(ok) + len(failed), len(_STDLIB_FILES))
        for rel in ok:
            c_path = out / f"{relative_slug(DEFAULT_STDLIB, DEFAULT_STDLIB / rel)}.c"
            self.assertTrue(c_path.is_file(), f"missing output for {rel}")
            self.assertIn("void script_run(void)", c_path.read_text(encoding="utf-8"))


def _make_file_test(py_path: Path):
    rel = py_path.relative_to(DEFAULT_STDLIB).as_posix()

    def test(self: TestStdlibTranslate) -> None:
        code, err = try_translate_file(py_path)
        if err is not None:
            self.fail(err)
        self.assertIn("void script_run(void)", code or "")

    test.__doc__ = f"Transpile python-stdlib/{rel} to objcore C."
    test.__name__ = _safe_test_name(rel)
    return test


if _STDLIB_FILES:
    for _py in _STDLIB_FILES:
        _rel = _py.relative_to(DEFAULT_STDLIB).as_posix()
        setattr(TestStdlibTranslate, _safe_test_name(_rel), _make_file_test(_py))


if __name__ == "__main__":
    unittest.main()
