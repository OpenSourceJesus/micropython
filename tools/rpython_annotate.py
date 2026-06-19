#!/usr/bin/env python3
"""Add rpython type annotations to python-stdlib sources for tools/py2c.py.

Follows the conventions in ~/ShivyC/examples/rpython2c/RPYTHON.md:
  * scalar loop/index variables -> int (quoted annotation when name inference fails)
  * function return types -> int / bool / str / float
  * homogeneous list/dict literals -> list[T] / dict[K,V] (unboxed in C)

Reuses py2c's name-based inference and only adds annotations where py2c would
otherwise fall back to the boxed ``obj`` type.
"""

from __future__ import annotations

import argparse
import ast
import re
import sys
from pathlib import Path

MICROPYTHON_TOP = Path(__file__).resolve().parents[1]
DEFAULT_STDLIB = MICROPYTHON_TOP / "lib" / "micropython-lib" / "python-stdlib"

if str(MICROPYTHON_TOP / "tools") not in sys.path:
    sys.path.insert(0, str(MICROPYTHON_TOP / "tools"))

from py2c import (  # noqa: E402
    OBJ,
    ann_dict_kv,
    ann_elem_ctype,
    ann_text_to_ctype,
    ann_to_ctype,
    arg_ctype,
    infer_from_name,
    optional_param_names,
    _param_assigned_from_call,
    _param_used_as_container,
    _param_used_as_object,
    _param_used_in_isinstance,
    _param_used_in_str_compare,
)

# Extra names common in micropython-lib stdlib but absent from py2c INT_NAMES.
EXTRA_INT_NAMES = frozenset({
    "lo", "hi", "mid", "childpos", "parentpos", "endpos", "startpos",
    "rightpos", "quad_pos", "leftbits", "leftchar", "shift", "mask", "sig",
    "crc", "acc", "val", "old", "new", "tmp", "rem", "quot", "digit", "base",
    "radix", "prec", "precision", "min", "max", "argc", "opcode",
    "ch", "cp", "ordval", "pad", "fill", "tab", "tabsize", "indent", "dedent",
    "margin", "maxlen", "minlen", "chunk_size", "block_size", "digest_size",
    "bufsize", "buflen", "outlen", "inlen", "nbytes", "nc", "nr", "nw",
    "parentpos", "childpos", "rightpos", "startpos", "endpos",
})

# Parameters that are almost always generic objects in stdlib APIs.
GENERIC_PARAM_NAMES = frozenset({
    "a", "b", "x", "y", "z", "key", "value", "item", "obj", "arg", "args",
    "kwargs", "self", "cls", "path", "name", "data", "s", "iterable", "seq",
    "heap", "resource", "line", "lineEnd", "ascii", "buf", "src", "dst",
    "default_factory", "format", "mode", "flags", "type", "val", "vals",
})

NUMERIC_BINOPS = (
    ast.Add, ast.Sub, ast.Mult, ast.FloorDiv, ast.Mod,
    ast.LShift, ast.RShift, ast.BitAnd, ast.BitOr, ast.BitXor,
)
ORDER_CMP = (ast.Lt, ast.LtE, ast.Gt, ast.GtE)
RPYTHON_SCALAR = {"int", "bool", "str", "float", "double", "char*"}


def _ctype_to_ann(ct: str | None) -> str | None:
    if not ct or ct == OBJ:
        return None
    if ct == "char*":
        return "str"
    if ct == "double":
        return "float"
    if ct in ("int", "bool", "float"):
        return ct
    return None


def _ann_node(text: str) -> ast.expr:
    return ast.Constant(value=text)


def _existing_ann_text(node: ast.expr | None) -> str | None:
    if node is None:
        return None
    if isinstance(node, ast.Constant) and isinstance(node.value, str):
        return node.value
    try:
        return ast.unparse(node).strip().strip("'\"")
    except Exception:
        return None


def _guess_name(name: str) -> str | None:
    if name in EXTRA_INT_NAMES:
        return "int"
    return _ctype_to_ann(infer_from_name(name))


def _is_numeric_name(name: str, fn: ast.FunctionDef) -> bool:
    """True when ``name`` is used only in numeric contexts inside ``fn``."""
    if _param_used_as_container(fn, name):
        return False
    if _param_used_as_object(fn, name):
        return False
    if _param_used_in_isinstance(fn, name):
        return False
    if _param_used_in_str_compare(fn, name):
        return False

    saw_use = False
    for node in ast.walk(fn):
        if isinstance(node, ast.Name) and node.id == name:
            ctx = node.ctx
            if isinstance(ctx, ast.Store):
                parent = None
                # assigned from non-numeric -> not int
                for sub in ast.walk(fn):
                    if isinstance(sub, ast.Assign):
                        for t in sub.targets:
                            if t is node:
                                if not _expr_numeric(sub.value, fn, name):
                                    return False
                    elif isinstance(sub, ast.AnnAssign) and sub.target is node:
                        pass
                    elif isinstance(sub, ast.AugAssign) and sub.target is node:
                        if not _expr_numeric(sub.value, fn, name):
                            return False
            elif isinstance(ctx, ast.Load):
                saw_use = True
                if not _name_use_numeric(node, fn):
                    return False
    return saw_use


def _expr_numeric(node: ast.AST | None, fn: ast.FunctionDef, param: str) -> bool:
    if node is None:
        return True
    if isinstance(node, ast.Constant):
        return isinstance(node.value, (int, float, bool)) or node.value is None
    if isinstance(node, ast.Name):
        if node.id == param:
            return True
        g = _guess_name(node.id)
        return g in ("int", "float", "bool") if g else False
    if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
        return _expr_numeric(node.operand, fn, param)
    if isinstance(node, ast.BinOp) and isinstance(node.op, NUMERIC_BINOPS):
        return (_expr_numeric(node.left, fn, param) and
                _expr_numeric(node.right, fn, param))
    if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
        if node.func.id in ("len", "ord", "int", "abs", "min", "max", "hash"):
            return True
    if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute):
        if node.func.attr in ("find", "rfind", "index", "count", "get"):
            return True
    if isinstance(node, ast.IfExp):
        return (_expr_numeric(node.body, fn, param) and
                _expr_numeric(node.orelse, fn, param))
    if isinstance(node, ast.Compare):
        return all(_expr_numeric(c, fn, param) for c in node.comparators)
    return False


def _name_use_numeric(name_node: ast.Name, fn: ast.FunctionDef) -> bool:
    # Walk parents by scanning — good enough for our patterns.
    name = name_node.id
    for parent in ast.walk(fn):
        if isinstance(parent, ast.BinOp) and isinstance(parent.op, NUMERIC_BINOPS):
            if parent.left is name_node or parent.right is name_node:
                return True
        if isinstance(parent, ast.Compare):
            if name_node in (parent.left, *parent.comparators):
                if all(isinstance(op, ORDER_CMP + (ast.Eq, ast.NotEq))
                       for op in parent.ops):
                    other = (parent.comparators[0]
                             if parent.left is name_node else parent.left)
                    return _expr_numeric(other, fn, name)
        if isinstance(parent, ast.Subscript):
            if parent.slice is name_node:
                return True
        if isinstance(parent, ast.Call) and isinstance(parent.func, ast.Name):
            if parent.func.id == "range" and name_node in parent.args:
                return True
    return False


def _param_desired_ann(fn: ast.FunctionDef, arg: ast.arg) -> str | None:
    if arg.arg in GENERIC_PARAM_NAMES:
        return None
    if arg.annotation is not None:
        return None
    if arg.arg in optional_param_names(fn):
        return None
    if _param_assigned_from_call(fn, arg.arg, {"input", "raw_input"}):
        return None

    ct = arg_ctype(fn, arg)
    if ct != OBJ:
        return None

    guess = _guess_name(arg.arg)
    if not guess:
        return None
    if guess in ("int", "bool") and _param_used_as_container(fn, arg.arg):
        return None
    if guess in ("bool", "int", "char*") and _param_used_as_object(fn, arg.arg):
        return None
    if guess == "int" and _param_used_in_str_compare(fn, arg.arg):
        return None
    return guess


def _infer_return_ann(fn: ast.FunctionDef) -> str | None:
    if fn.returns is not None:
        return None
    rets = [n.value for n in ast.walk(fn) if isinstance(n, ast.Return)]
    if not rets:
        return None
    values = [r for r in rets if r is not None]
    if not values:
        return None

    types: set[str] = set()
    for val in values:
        t = _expr_ann(val)
        if t is None:
            return None
        types.add(t)
    if len(types) != 1:
        return None
    return types.pop()


def _expr_ann(node: ast.AST | None) -> str | None:
    if node is None:
        return None
    if isinstance(node, ast.Constant):
        if isinstance(node.value, bool):
            return "bool"
        if isinstance(node.value, int):
            return "int"
        if isinstance(node.value, float):
            return "float"
        if isinstance(node.value, str):
            return "str"
        return None
    if isinstance(node, ast.Name):
        return _guess_name(node.id)
    if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
        return _expr_ann(node.operand)
    if isinstance(node, ast.Compare) and len(node.ops) == 1:
        return "bool"
    if isinstance(node, ast.BoolOp):
        return "bool"
    if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
        if node.func.id == "len":
            return "int"
    if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute):
        if node.func.attr in ("find", "rfind", "index", "count"):
            return "int"
    if isinstance(node, ast.IfExp):
        tb, to = _expr_ann(node.body), _expr_ann(node.orelse)
        return tb if tb and tb == to else None
    if isinstance(node, ast.BinOp) and isinstance(node.op, NUMERIC_BINOPS):
        tl, tr = _expr_ann(node.left), _expr_ann(node.right)
        if tl == "float" or tr == "float":
            return "float"
        if tl == "int" and tr == "int":
            return "int"
    return None


def _list_ann(node: ast.List) -> str | None:
    if not node.elts:
        return None
    types: set[str] = set()
    for elt in node.elts:
        t = _expr_ann(elt)
        if t is None:
            return None
        types.add(t)
    if len(types) != 1:
        return None
    return "list[%s]" % types.pop()


def _dict_ann(node: ast.Dict) -> str | None:
    if not node.keys or any(k is None for k in node.keys):
        return None
    ktypes: set[str] = set()
    vtypes: set[str] = set()
    for k, v in zip(node.keys, node.values):
        kt, vt = _expr_ann(k), _expr_ann(v)
        if kt is None or vt is None:
            return None
        ktypes.add(kt)
        vtypes.add(vt)
    if len(ktypes) != 1 or len(vtypes) != 1:
        return None
    return "dict[%s,%s]" % (ktypes.pop(), vtypes.pop())


def _reassigned_names(tree: ast.AST) -> set[str]:
    """Names assigned more than once in the same scope (module or function)."""
    out: set[str] = set()

    def scan(body: list[ast.stmt]):
        counts: dict[str, int] = {}
        for node in body:
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                scan(node.body)
                continue
            for sub in ast.walk(node):
                if isinstance(sub, (ast.FunctionDef, ast.AsyncFunctionDef)):
                    continue
                if isinstance(sub, ast.Assign):
                    for t in sub.targets:
                        if isinstance(t, ast.Name):
                            counts[t.id] = counts.get(t.id, 0) + 1
                elif isinstance(sub, ast.AnnAssign) and isinstance(sub.target, ast.Name):
                    counts[sub.target.id] = counts.get(sub.target.id, 0) + 1
                elif isinstance(sub, ast.AugAssign) and isinstance(sub.target, ast.Name):
                    counts[sub.target.id] = counts.get(sub.target.id, 0) + 1
        out.update(n for n, c in counts.items() if c > 1)

    if isinstance(tree, ast.Module):
        scan(tree.body)
    elif isinstance(tree, (ast.FunctionDef, ast.AsyncFunctionDef)):
        scan(tree.body)
    return out


def _assign_desired_ann(
    node: ast.Assign,
    reassigned: set[str],
) -> str | None:
    if len(node.targets) != 1 or not isinstance(node.targets[0], ast.Name):
        return None
    name = node.targets[0].id
    if name in reassigned or name in SKIP_ASSIGN_NAMES or name in SKIP_LOCAL_NAMES:
        return None
    if isinstance(node.value, ast.List):
        ann = _list_ann(node.value)
        if ann:
            elem = ann[5:-1]  # list[T] -> T
            if elem not in _SCALAR_LIST_elems:
                return None
        return ann
    if isinstance(node.value, ast.Dict):
        return _dict_ann(node.value)
    local = _local_int_ann(node)
    if local:
        if isinstance(node.value, ast.Constant) and isinstance(node.value.value, bool):
            return None
        return local
    return None


def _local_int_ann(node: ast.Assign) -> str | None:
    if len(node.targets) != 1 or not isinstance(node.targets[0], ast.Name):
        return None
    if isinstance(node.value, ast.Call):
        return None
    name = node.targets[0].id
    guess = _guess_name(name)
    if not guess:
        return None
    # Do not tag numeric temporaries with str-like names (e.g. timedelta ``s``).
    if guess == "str" and _expr_is_intish(node.value):
        return None
    return guess


def _expr_is_intish(node: ast.AST | None) -> bool:
    if node is None:
        return False
    if isinstance(node, ast.Constant) and isinstance(node.value, int):
        return True
    if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
        return _expr_is_intish(node.operand)
    if isinstance(node, ast.BinOp) and isinstance(node.op, NUMERIC_BINOPS):
        return _expr_is_intish(node.left) or _expr_is_intish(node.right)
    if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
        return node.func.id in ("len", "ord", "int", "abs")
    return False


def _would_help(ann_text: str) -> bool:
    ct = ann_text_to_ctype(ann_text)
    if ct is not None and ct != OBJ:
        if ct.startswith("_tlist_") or ct.startswith("_tdict_"):
            return True
        if ct in RPYTHON_SCALAR:
            return True
    # list[T] / dict[K,V] map to OBJ in ann_text_to_ctype but still help py2c.
    bare = ann_text.strip().strip("'\"")
    if bare.startswith("list[") or bare.startswith("dict["):
        return True
    return False


# Single-letter names are too ambiguous for automatic local annotations.
SKIP_LOCAL_NAMES = frozenset({"s", "l", "n", "m", "t", "v", "f", "r", "c", "d", "e", "g", "h", "p", "w"})

SKIP_ASSIGN_NAMES = frozenset({
    "__all__", "__doc__", "__slots__", "__version__", "__name__",
})

# Typed lists only help py2c for numeric/scalar elements, not str objects.
_SCALAR_LIST_elems = frozenset({"int", "float", "bool"})


class Annotator(ast.NodeVisitor):
    def __init__(self):
        self.func_edits: list[tuple[ast.FunctionDef, dict]] = []
        self.assign_edits: list[tuple[ast.Assign, str]] = []
        self._scope_reassigned: set[str] = set()
        self._func_params: set[str] = set()
        self._in_function = 0

    def _enter_scope(self, body: list[ast.stmt]):
        saved = self._scope_reassigned
        counts: dict[str, int] = {}
        for node in body:
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                continue
            for sub in ast.walk(node):
                if isinstance(sub, (ast.FunctionDef, ast.AsyncFunctionDef)):
                    continue
                if isinstance(sub, ast.Assign):
                    for t in sub.targets:
                        if isinstance(t, ast.Name):
                            counts[t.id] = counts.get(t.id, 0) + 1
                elif isinstance(sub, ast.AnnAssign) and isinstance(sub.target, ast.Name):
                    counts[sub.target.id] = counts.get(sub.target.id, 0) + 1
                elif isinstance(sub, ast.AugAssign) and isinstance(sub.target, ast.Name):
                    counts[sub.target.id] = counts.get(sub.target.id, 0) + 1
        self._scope_reassigned = {n for n, c in counts.items() if c > 1}
        return saved

    def visit_Module(self, node: ast.Module):
        saved = self._enter_scope(node.body)
        self.generic_visit(node)
        self._scope_reassigned = saved

    def visit_FunctionDef(self, node: ast.FunctionDef):
        param_anns = {}
        for arg in node.args.args + node.args.kwonlyargs:
            ann = _param_desired_ann(node, arg)
            if ann and _would_help('"%s"' % ann):
                param_anns[arg.arg] = ann
        ret_ann = _infer_return_ann(node)
        if not (ret_ann and _would_help('"%s"' % ret_ann)):
            ret_ann = None
        if param_anns or ret_ann:
            self.func_edits.append((node, {"params": param_anns, "return": ret_ann}))
        saved = self._enter_scope(node.body)
        saved_params = self._func_params
        self._func_params = {a.arg for a in node.args.args + node.args.kwonlyargs}
        self._in_function += 1
        self.generic_visit(node)
        self._in_function -= 1
        self._func_params = saved_params
        self._scope_reassigned = saved

    visit_AsyncFunctionDef = visit_FunctionDef

    def visit_Assign(self, node: ast.Assign):
        if self._in_function:
            if len(node.targets) == 1 and isinstance(node.targets[0], ast.Name):
                name = node.targets[0].id
                if name in self._func_params:
                    self.generic_visit(node)
                    return
            ann = _assign_desired_ann(node, self._scope_reassigned)
            if ann and _would_help('"%s"' % ann):
                self.assign_edits.append((node, ann))
        self.generic_visit(node)


def _insert_param_anns(def_line: str, param_anns: dict[str, str]) -> str:
    for name, ann in sorted(param_anns.items(), key=lambda x: -len(x[0])):
        repl = '%s: "%s"' % (name, ann)
        pat = r'\b' + re.escape(name) + r'(?=\s*(?:[,)=:]|=))'
        new_line, n = re.subn(pat, repl, def_line, count=1)
        if n:
            def_line = new_line
    return def_line


def _insert_return_ann(def_line: str, ret_ann: str) -> str:
    if "->" in def_line:
        return def_line
    return re.sub(
        r'\)\s*:',
        ') -> "%s":' % ret_ann,
        def_line,
        count=1,
    )


def apply_annotations(source: str, tree: ast.AST) -> tuple[str, int]:
    ann = Annotator()
    ann.visit(tree)
    if not ann.func_edits and not ann.assign_edits:
        return source, 0

    lines = source.splitlines(keepends=True)
    changed = 0

    for fn, edits in ann.func_edits:
        if fn.lineno <= 0 or fn.lineno > len(lines):
            continue
        idx = fn.lineno - 1
        # Multi-line def: join until colon at paren depth 0.
        def_lines = [lines[idx]]
        pos = 0
        while pos < len(def_lines[-1]) or def_lines[-1].count("(") > def_lines[-1].count(")"):
            if pos >= len(def_lines[-1]) and idx + len(def_lines) < len(lines):
                def_lines.append(lines[idx + len(def_lines)])
            pos = sum(len(l) for l in def_lines)
            if len(def_lines) > 20:
                break
        joined = "".join(def_lines)
        if ":" not in joined:
            continue
        new_joined = joined
        if edits.get("params"):
            new_joined = _insert_param_anns(new_joined, edits["params"])
        if edits.get("return"):
            new_joined = _insert_return_ann(new_joined, edits["return"])
        new_joined = re.sub(r'"\s*(?==)', '" ', new_joined)
        new_joined = re.sub(r'(")\s*=\s*(\d)', r'\1 = \2', new_joined)
        if new_joined != joined:
            # Replace first len(def_lines) lines with new_joined split
            new_parts = new_joined.splitlines(keepends=True)
            if new_parts and not new_parts[-1].endswith(("\n", "\r")):
                new_parts[-1] += lines[idx + len(def_lines) - 1][-1:] \
                    if lines[idx + len(def_lines) - 1].endswith("\n") else "\n"
            for off, part in enumerate(new_parts):
                lines[idx + off] = part
            for off in range(len(new_parts), len(def_lines)):
                lines[idx + off] = ""
            changed += 1

    for assign, ann_text in ann.assign_edits:
        if assign.lineno <= 0 or assign.lineno > len(lines):
            continue
        idx = assign.lineno - 1
        line = lines[idx]
        m = re.match(r'(\s*)([A-Za-z_]\w*)\s*=', line)
        if not m:
            continue
        name = m.group(2)
        new_line = re.sub(
            r'^(\s*)' + re.escape(name) + r'\s*=',
            r'\1%s: "%s" =' % (name, ann_text),
            line,
            count=1,
        )
        if new_line != line:
            lines[idx] = new_line
            changed += 1

    out = "".join(lines)
    return out, changed


def is_library_py(path: Path, stdlib_root: Path) -> bool:
    rel = path.relative_to(stdlib_root)
    parts = rel.parts
    if path.name == "manifest.py" or path.name == "tests.py":
        return False
    if path.name.startswith("test_") or parts[0].startswith("test"):
        return False
    if "tests" in parts or "examples" in parts:
        return False
    return True


def iter_library_files(stdlib_root: Path) -> list[Path]:
    return sorted(
        p for p in stdlib_root.rglob("*.py")
        if is_library_py(p, stdlib_root)
    )


def strip_annotations(source: str) -> tuple[str, int]:
    """Remove rpython : \"type\" annotations previously added by this tool."""
    patterns = [
        (r'(\b[A-Za-z_]\w*): "(?:int|bool|str|float|list\[[^"]+\]|dict\[[^"]+\])"\s*=',
         r'\1 ='),
        (r'(\b[A-Za-z_]\w*): "(?:int|bool|str|float)"(?=\s*[,)=])',
         r'\1'),
    ]
    out = source
    n = 0
    for pat, repl in patterns:
        out, c = re.subn(pat, repl, out)
        n += c
    return out, n


def strip_file(path: Path, dry_run: bool = False) -> int:
    source = path.read_text(encoding="utf-8")
    new_source, n = strip_annotations(source)
    if n and not dry_run:
        path.write_text(new_source, encoding="utf-8")
    return n


def annotate_file(path: Path, dry_run: bool = False) -> int:
    source = path.read_text(encoding="utf-8")
    try:
        tree = ast.parse(source, filename=str(path))
    except SyntaxError as e:
        print("  skip %s: %s" % (path, e), file=sys.stderr)
        return 0
    new_source, n = apply_annotations(source, tree)
    if n and not dry_run:
        path.write_text(new_source, encoding="utf-8")
    return n


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--stdlib-dir",
        type=Path,
        default=DEFAULT_STDLIB,
        help="python-stdlib root (default: %(default)s)",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="include tests and examples (default: library modules only)",
    )
    parser.add_argument(
        "-n", "--dry-run",
        action="store_true",
        help="report changes without writing files",
    )
    parser.add_argument(
        "--strip",
        action="store_true",
        help="remove rpython annotations instead of adding them",
    )
    parser.add_argument(
        "files",
        nargs="*",
        type=Path,
        help="specific .py files to annotate (default: all library modules)",
    )
    args = parser.parse_args(argv)

    if args.files:
        files = [p.resolve() for p in args.files]
    elif args.all:
        files = sorted(args.stdlib_dir.rglob("*.py"))
    else:
        files = iter_library_files(args.stdlib_dir)

    total_files = 0
    total_edits = 0
    action = strip_file if args.strip else annotate_file
    for path in files:
        if not path.is_file():
            print("  missing: %s" % path, file=sys.stderr)
            continue
        n = action(path, dry_run=args.dry_run)
        if n:
            total_files += 1
            total_edits += n
            rel = path.relative_to(args.stdlib_dir) if path.is_relative_to(args.stdlib_dir) else path
            verb = "stripped" if args.strip else "annotated"
            print("%s: %d edit(s)%s" % (rel, n, " (dry-run)" if args.dry_run else ""))

    verb = "stripped" if args.strip else "annotated"
    print("%s %d file(s), %d edit(s)" % (verb, total_files, total_edits))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
