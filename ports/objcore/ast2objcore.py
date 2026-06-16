#!/usr/bin/env python3
"""Translate a subset of Python to C that drives the objcore MicroPython runtime."""

from __future__ import annotations

import argparse
import ast
import os
import sys
import warnings
import textwrap
from typing import Iterable


HEADER = textwrap.dedent(
    """\
    #include "script.h"
    #include "py/mphal.h"
    #include "py/obj.h"
    #include "py/objlist.h"
    #include "py/objstr.h"
    #include "py/objtuple.h"
    #include "py/qstr.h"
    #include "py/runtime.h"
    """
)

# micropython-lib manifest.py DSL — no runtime equivalent in objcore.
MANIFEST_FUNCS = frozenset({"metadata", "module", "package", "require", "opt", "include"})

# Names treated as known in permissive (stdlib) mode when not assigned locally.
PERMISSIVE_GLOBALS: dict[str, str] = {
    "True": "mp_const_true",
    "False": "mp_const_false",
    "None": "mp_const_none",
    "__name__": "mp_obj_new_str(\"__main__\", 8)",
    "set": "mp_obj_new_set(0, NULL)",
    "frozenset": "mp_obj_new_set(0, NULL)",
    "Ellipsis": "mp_const_none",
}

BINOPS: dict[type, str] = {
    ast.Add: "MP_BINARY_OP_ADD",
    ast.Sub: "MP_BINARY_OP_SUBTRACT",
    ast.Mult: "MP_BINARY_OP_MULTIPLY",
    ast.Div: "MP_BINARY_OP_TRUE_DIVIDE",
    ast.FloorDiv: "MP_BINARY_OP_FLOOR_DIVIDE",
    ast.Mod: "MP_BINARY_OP_MODULO",
    ast.Pow: "MP_BINARY_OP_POWER",
    ast.BitAnd: "MP_BINARY_OP_AND",
    ast.BitOr: "MP_BINARY_OP_OR",
    ast.BitXor: "MP_BINARY_OP_XOR",
    ast.LShift: "MP_BINARY_OP_LSHIFT",
    ast.RShift: "MP_BINARY_OP_RSHIFT",
}

CMPOPS: dict[type, str] = {
    ast.Eq: "MP_BINARY_OP_EQUAL",
    ast.NotEq: "MP_BINARY_OP_NOT_EQUAL",
    ast.Lt: "MP_BINARY_OP_LESS",
    ast.LtE: "MP_BINARY_OP_LESS_EQUAL",
    ast.Gt: "MP_BINARY_OP_MORE",
    ast.GtE: "MP_BINARY_OP_MORE_EQUAL",
}

UNARYOPS: dict[type, str] = {
    ast.UAdd: "MP_UNARY_OP_POSITIVE",
    ast.USub: "MP_UNARY_OP_NEGATIVE",
    ast.Invert: "MP_UNARY_OP_INVERT",
    ast.Not: "MP_UNARY_OP_NOT",
}


class TranslateError(Exception):
    def __init__(self, msg: str, lineno: int | None = None) -> None:
        super().__init__(msg)
        self.msg = msg
        self.lineno = lineno


def collect_names(node: ast.AST) -> set[str]:
    names: set[str] = set()
    for child in ast.walk(node):
        if isinstance(child, ast.Name) and isinstance(child.ctx, ast.Load):
            names.add(child.id)
    return names


class RefGraph:
    """Track container reachability to reject cyclic object graphs at compile time."""

    def __init__(self) -> None:
        self.reachable: dict[str, set[str]] = {}

    def define_empty(self, var: str) -> None:
        self.reachable[var] = set()

    def assign_copy(self, var: str, refs: set[str]) -> None:
        reach: set[str] = set(refs)
        for name in refs:
            reach |= self.reachable.get(name, set())
        self.reachable[var] = reach

    def check_store(self, container: str, item_refs: set[str], lineno: int | None) -> None:
        for name in item_refs:
            if name == container:
                raise TranslateError(
                    f"cyclic reference: {container!r} cannot contain itself "
                    "(reference counting cannot collect cycles)",
                    lineno,
                )
            if container in self.reachable.get(name, set()):
                raise TranslateError(
                    f"cyclic reference: {container!r} and {name!r} would form a cycle "
                    "(reference counting cannot collect cycles)",
                    lineno,
                )

    def record_store(self, container: str, item_refs: set[str]) -> None:
        self.check_store(container, item_refs, None)
        reach = self.reachable.setdefault(container, set())
        reach |= set(item_refs)
        for name in item_refs:
            reach |= self.reachable.get(name, set())


class Emitter(ast.NodeVisitor):
    def __init__(self, *, permissive: bool = False) -> None:
        self.permissive = permissive
        self.lines: list[str] = []
        self._tmp = 0
        self._label = 0
        self._vars: dict[str, str] = {}
        self._refs = RefGraph()
        self._loop_stack: list[tuple[str, str]] = []  # (break_label, continue_label)

    def emit(self, line: str = "") -> None:
        self.lines.append(line)

    def fresh(self, prefix: str = "_v") -> str:
        self._tmp += 1
        return f"{prefix}{self._tmp}"

    def label(self, prefix: str = "_L") -> str:
        self._label += 1
        return f"{prefix}{self._label}"

    def visit_Module(self, node: ast.Module) -> None:
        for stmt in node.body:
            self._visit_stmt(stmt)

    def _visit_stmt(self, stmt: ast.stmt) -> None:
        try:
            self.visit(stmt)
        except TranslateError:
            if not self.permissive:
                raise

    def visit_Pass(self, node: ast.Pass) -> None:
        return

    def visit_Expr(self, node: ast.Expr) -> None:
        self._emit_expr_stmt(node.value, node.lineno)

    def visit_Assign(self, node: ast.Assign) -> None:
        if len(node.targets) != 1:
            raise TranslateError("only single-target assignment is supported", node.lineno)
        self._emit_assign(node.targets[0], node.value, node.lineno)

    def visit_AugAssign(self, node: ast.AugAssign) -> None:
        if not isinstance(node.target, ast.Name):
            raise TranslateError("augmented assignment to names only", node.lineno)
        if not isinstance(node.op, ast.Add):
            raise TranslateError("only += augmented assignment is supported", node.lineno)
        name = node.target.id
        if name not in self._vars:
            raise TranslateError(f"undefined name {name!r}", node.lineno)
        cname = self._vars[name]
        rhs = self._emit_expr(node.value)
        tmp = self.fresh("_aug")
        self.emit(f"mp_obj_t {tmp} = mp_binary_op(MP_BINARY_OP_INPLACE_ADD, {cname}, {rhs});")
        self.emit(f"{cname} = {tmp};")
        self._refs.assign_copy(name, {name})

    def visit_If(self, node: ast.If) -> None:
        cond = self._emit_expr(node.test)
        else_lbl = self.label("_else")
        end_lbl = self.label("_endif")
        self.emit(f"if (!mp_obj_is_true({cond})) goto {else_lbl};")
        for stmt in node.body:
            self._visit_stmt(stmt)
        self.emit(f"goto {end_lbl};")
        self.emit(f"{else_lbl}:")
        for stmt in node.orelse:
            self._visit_stmt(stmt)
        self.emit(f"{end_lbl}:")

    def visit_While(self, node: ast.While) -> None:
        if node.orelse:
            raise TranslateError("while/else is not supported", node.lineno)
        start = self.label("_while")
        end = self.label("_wend")
        self._loop_stack.append((end, start))
        self.emit(f"{start}:")
        cond = self._emit_expr(node.test)
        self.emit(f"if (!mp_obj_is_true({cond})) goto {end};")
        for stmt in node.body:
            self._visit_stmt(stmt)
        self.emit(f"goto {start};")
        self.emit(f"{end}:")
        self._loop_stack.pop()

    def visit_For(self, node: ast.For) -> None:
        if node.orelse:
            raise TranslateError("for/else is not supported", node.lineno)
        if not isinstance(node.target, ast.Name):
            raise TranslateError("for-loop target must be a simple name", node.lineno)
        if isinstance(node.iter, ast.Call) and isinstance(node.iter.func, ast.Name):
            if node.iter.func.id == "range" and node.iter.keywords == []:
                self._emit_for_range(node)
                return
        self._emit_for_iter(node)

    def visit_Break(self, node: ast.Break) -> None:
        if not self._loop_stack:
            raise TranslateError("break outside loop", node.lineno)
        self.emit(f"goto {self._loop_stack[-1][0]};")

    def visit_Continue(self, node: ast.Continue) -> None:
        if not self._loop_stack:
            raise TranslateError("continue outside loop", node.lineno)
        self.emit(f"goto {self._loop_stack[-1][1]};")

    def visit_FunctionDef(self, node: ast.FunctionDef) -> None:
        if self.permissive:
            return
        self._unsupported_stmt(node)

    def visit_AsyncFunctionDef(self, node: ast.AsyncFunctionDef) -> None:
        if self.permissive:
            return
        self._unsupported_stmt(node)

    def visit_ClassDef(self, node: ast.ClassDef) -> None:
        if self.permissive:
            return
        self._unsupported_stmt(node)

    def visit_AnnAssign(self, node: ast.AnnAssign) -> None:
        if node.value is None:
            if self.permissive:
                return
            raise TranslateError("annotated assignment without value is not supported", node.lineno)
        self._emit_assign(node.target, node.value, node.lineno)

    def visit_Delete(self, node: ast.Delete) -> None:
        if self.permissive:
            return
        self._unsupported_stmt(node)

    def _unsupported_stmt(self, node: ast.AST) -> None:
        if self.permissive:
            return
        raise TranslateError(
            f"unsupported statement: {type(node).__name__}",
            getattr(node, "lineno", None),
        )

    def visit_Import(self, node: ast.Import) -> None:
        if self.permissive:
            return
        raise TranslateError("import statements are not supported", node.lineno)

    def visit_ImportFrom(self, node: ast.ImportFrom) -> None:
        if self.permissive:
            return
        raise TranslateError("import statements are not supported", node.lineno)

    def _emit_for_range(self, node: ast.For) -> None:
        assert isinstance(node.target, ast.Name)
        args = node.iter.args  # type: ignore[union-attr]
        if not (1 <= len(args) <= 3):
            raise TranslateError("range() supports one to three arguments", node.lineno)
        bounds = [self._emit_expr(arg) for arg in args]
        if len(bounds) == 1:
            start, stop, step = "mp_obj_new_int(0)", bounds[0], "mp_obj_new_int(1)"
        elif len(bounds) == 2:
            start, stop, step = bounds[0], bounds[1], "mp_obj_new_int(1)"
        else:
            start, stop, step = bounds

        idx = self.fresh("_ri")
        end_lbl = self.label("_forend")
        loop_lbl = self.label("_forloop")
        self._loop_stack.append((end_lbl, loop_lbl))

        self.emit(f"mp_obj_t {idx} = {start};")
        self.emit(f"goto {loop_lbl};")
        self.emit(f"{loop_lbl}:")
        cmp_tmp = self.fresh("_rcmp")
        self.emit(
            f"mp_obj_t {cmp_tmp} = mp_binary_op(MP_BINARY_OP_LESS, {idx}, {stop});"
        )
        self.emit(f"if (!mp_obj_is_true({cmp_tmp})) goto {end_lbl};")

        var = node.target.id
        cname = self._vars.get(var)
        if cname is None:
            cname = self.fresh("_var")
            self._vars[var] = cname
            self.emit(f"mp_obj_t {cname} = {idx};")
            self._refs.define_empty(var)
        else:
            self.emit(f"{cname} = {idx};")

        for stmt in node.body:
            self._visit_stmt(stmt)

        step_tmp = self.fresh("_rstep")
        self.emit(
            f"mp_obj_t {step_tmp} = mp_binary_op(MP_BINARY_OP_ADD, {idx}, {step});"
        )
        self.emit(f"{idx} = {step_tmp};")
        self.emit(f"goto {loop_lbl};")
        self.emit(f"{end_lbl}:")
        self._loop_stack.pop()

    def _emit_for_iter(self, node: ast.For) -> None:
        assert isinstance(node.target, ast.Name)
        iterable = self._emit_expr(node.iter)
        iter_tmp = self.fresh("_iter")
        buf = self.fresh("_iterbuf")
        end_lbl = self.label("_forend")
        loop_lbl = self.label("_forbody")
        self._loop_stack.append((end_lbl, loop_lbl))

        self.emit(f"mp_obj_iter_buf_t {buf};")
        self.emit(f"mp_obj_t {iter_tmp} = mp_getiter({iterable}, &{buf});")
        self.emit(f"goto {loop_lbl};")
        self.emit(f"{loop_lbl}:")
        item = self.fresh("_item")
        self.emit(f"mp_obj_t {item} = mp_iternext({iter_tmp});")
        self.emit(f"if ({item} == MP_OBJ_STOP_ITERATION) goto {end_lbl};")

        var = node.target.id
        cname = self._vars.get(var)
        if cname is None:
            cname = self.fresh("_var")
            self._vars[var] = cname
            self.emit(f"mp_obj_t {cname} = {item};")
            self._refs.define_empty(var)
        else:
            self.emit(f"{cname} = {item};")

        for stmt in node.body:
            self._visit_stmt(stmt)
        self.emit(f"goto {loop_lbl};")
        self.emit(f"{end_lbl}:")
        self._loop_stack.pop()

    def _emit_qstr(self, name: str) -> str:
        data, length = c_string_literal(name)
        tmp = self.fresh("_qstr")
        self.emit(f"qstr {tmp} = qstr_from_strn({data}, {length});")
        return tmp

    def _emit_assign(self, target: ast.expr, value: ast.expr, lineno: int | None) -> None:
        if isinstance(target, ast.Name):
            name = target.id
            if isinstance(value, (ast.List, ast.Dict, ast.Tuple)):
                cval = self._emit_container(value, owner_name=name, lineno=lineno)
            else:
                cval = self._emit_expr(value)
                refs = collect_names(value)
                cname = self._vars.get(name)
                if cname is None:
                    cname = self.fresh("_var")
                    self._vars[name] = cname
                    self.emit(f"mp_obj_t {cname} = {cval};")
                else:
                    self.emit(f"{cname} = {cval};")
                self._refs.assign_copy(name, refs)
                return

            cname = self._vars.get(name)
            if cname is None:
                cname = self.fresh("_var")
                self._vars[name] = cname
                self.emit(f"mp_obj_t {cname} = {cval};")
            else:
                self.emit(f"{cname} = {cval};")
            return

        if isinstance(target, ast.Subscript):
            base = self._emit_expr(target.value)
            index = self._emit_expr(target.slice)
            val = self._emit_expr(value)
            base_name = self._name_if_simple(target.value)
            item_refs = collect_names(value) | collect_names(target.slice)
            if base_name is not None:
                self._refs.check_store(base_name, item_refs, lineno)
                self._refs.record_store(base_name, item_refs)
            self.emit(f"mp_obj_subscr({base}, {index}, {val});")
            return

        raise TranslateError("unsupported assignment target", lineno)

    def _name_if_simple(self, node: ast.expr) -> str | None:
        if isinstance(node, ast.Name):
            return node.id
        return None

    def _emit_expr_stmt(self, node: ast.expr, lineno: int | None) -> None:
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
            if node.func.id in MANIFEST_FUNCS and self.permissive:
                return
        if isinstance(node, ast.Call):
            if isinstance(node.func, ast.Name) and node.func.id == "print":
                self._emit_print(node)
                return
            if isinstance(node.func, ast.Name) and node.func.id == "len":
                self._emit_len_call(node)
                return
            if isinstance(node.func, ast.Attribute) and node.func.attr == "append":
                self._emit_list_append(node, lineno)
                return
        cval = self._emit_expr(node)
        self.emit(f"(void){cval};")

    def _emit_len_call(self, node: ast.Call) -> None:
        if len(node.args) != 1 or node.keywords:
            raise TranslateError("len() takes exactly one argument", node.lineno)
        arg = self._emit_expr(node.args[0])
        tmp = self.fresh("_len")
        self.emit(f"mp_obj_t {tmp} = mp_obj_len({arg});")
        self.emit(f"mp_obj_print_helper(&mp_plat_print, {tmp}, PRINT_STR);")
        self.emit("mp_hal_stdout_tx_strn(\"\\n\", 1);")

    def _emit_list_append(self, node: ast.Call, lineno: int | None) -> None:
        if len(node.args) != 1 or node.keywords:
            raise TranslateError("list.append() takes exactly one argument", node.lineno)
        if not isinstance(node.func, ast.Attribute):
            raise TranslateError("invalid append() call", node.lineno)
        container_name = self._name_if_simple(node.func.value)
        container = self._emit_expr(node.func.value)
        item_refs = collect_names(node.args[0])
        if container_name is not None:
            self._refs.check_store(container_name, item_refs, lineno)
            self._refs.record_store(container_name, item_refs)
        item = self._emit_expr(node.args[0])
        self.emit(f"mp_obj_list_append({container}, {item});")

    def _emit_print(self, node: ast.Call) -> None:
        sep = " "
        end = "\n"
        for kw in node.keywords:
            if kw.arg == "sep" and isinstance(kw.value, ast.Constant) and isinstance(kw.value.value, str):
                sep = kw.value.value
            elif kw.arg == "end" and isinstance(kw.value, ast.Constant) and isinstance(kw.value.value, str):
                end = kw.value.value
            else:
                raise TranslateError("print() only supports constant sep=/end=", node.lineno)

        if not node.args:
            if end:
                self._emit_write_cstr(end)
            return

        for i, arg in enumerate(node.args):
            if i:
                self._emit_write_cstr(sep)
            val = self._emit_expr(arg)
            tmp = self.fresh("_p")
            self.emit(f"mp_obj_t {tmp} = {val};")
            self.emit(f"mp_obj_print_helper(&mp_plat_print, {tmp}, PRINT_STR);")
        if end:
            self._emit_write_cstr(end)

    def _emit_write_cstr(self, s: str) -> None:
        data, length = c_string_literal(s)
        self.emit(f"mp_hal_stdout_tx_strn({data}, {length});")

    def _emit_container(
        self,
        node: ast.List | ast.Dict | ast.Tuple,
        owner_name: str,
        lineno: int | None,
    ) -> str:
        cvar = self.fresh("_ctr")
        if isinstance(node, ast.List):
            self._refs.define_empty(owner_name)
            self.emit(f"mp_obj_t {cvar} = mp_obj_new_list(0, NULL);")
            for elt in node.elts:
                refs = collect_names(elt)
                self._refs.check_store(owner_name, refs, lineno)
                val = self._emit_expr(elt)
                self.emit(f"mp_obj_list_append({cvar}, {val});")
                self._refs.record_store(owner_name, refs)
            return cvar

        if isinstance(node, ast.Tuple):
            if not node.elts:
                self.emit(f"mp_obj_t {cvar} = mp_obj_new_tuple(0, NULL);")
                self._refs.define_empty(owner_name)
                return cvar
            vals = [self._emit_expr(elt) for elt in node.elts]
            arr = self.fresh("_titems")
            self.emit(f"mp_obj_t {arr}[] = {{{', '.join(vals)}}};")
            self.emit(f"mp_obj_t {cvar} = mp_obj_new_tuple({len(vals)}, {arr});")
            refs: set[str] = set()
            for elt in node.elts:
                refs |= collect_names(elt)
            self._refs.assign_copy(owner_name, refs)
            return cvar

        assert isinstance(node, ast.Dict)
        self._refs.define_empty(owner_name)
        self.emit(f"mp_obj_t {cvar} = mp_obj_new_dict(0);")
        for key_node, val_node in zip(node.keys, node.values):
            if key_node is None:
                raise TranslateError("dict unpacking is not supported", lineno)
            key_refs = collect_names(key_node)
            val_refs = collect_names(val_node)
            self._refs.check_store(owner_name, key_refs | val_refs, lineno)
            key = self._emit_expr(key_node)
            val = self._emit_expr(val_node)
            self.emit(f"mp_obj_dict_store({cvar}, {key}, {val});")
            self._refs.record_store(owner_name, key_refs | val_refs)
        return cvar

    def _emit_expr(self, node: ast.expr) -> str:
        if self.permissive:
            try:
                return self._emit_expr_inner(node)
            except TranslateError:
                return "mp_const_none"
        return self._emit_expr_inner(node)

    def _emit_expr_inner(self, node: ast.expr) -> str:
        if isinstance(node, ast.Constant):
            return self._const(node)
        if isinstance(node, ast.Name):
            if node.id in self._vars:
                return self._vars[node.id]
            if node.id in PERMISSIVE_GLOBALS:
                return PERMISSIVE_GLOBALS[node.id]
            if node.id == "True":
                return "mp_const_true"
            if node.id == "False":
                return "mp_const_false"
            if node.id == "None":
                return "mp_const_none"
            if self.permissive:
                return "mp_const_none"
            raise TranslateError(f"undefined name {node.id!r}", node.lineno)
        if isinstance(node, ast.Attribute):
            base = self._emit_expr_inner(node.value)
            q = self._emit_qstr(node.attr)
            tmp = self.fresh("_attr")
            self.emit(f"mp_obj_t {tmp} = mp_load_attr({base}, {q});")
            return tmp
        if isinstance(node, ast.UnaryOp):
            return self._emit_unary(node)
        if isinstance(node, ast.BinOp):
            return self._emit_binop(node)
        if isinstance(node, ast.Compare):
            return self._emit_compare(node)
        if isinstance(node, ast.BoolOp):
            return self._emit_boolop(node)
        if isinstance(node, ast.IfExp):
            return self._emit_ifexp(node)
        if isinstance(node, ast.Subscript):
            if isinstance(node.slice, ast.Slice):
                if self.permissive:
                    return "mp_const_none"
                raise TranslateError("slice subscripts are not supported", node.lineno)
            base = self._emit_expr_inner(node.value)
            index = self._emit_expr_inner(node.slice)
            tmp = self.fresh("_sub")
            self.emit(f"mp_obj_t {tmp} = mp_obj_subscr({base}, {index}, MP_OBJ_SENTINEL);")
            return tmp
        if isinstance(node, ast.Set):
            if not node.elts:
                return "mp_obj_new_set(0, NULL)"
            vals = [self._emit_expr_inner(elt) for elt in node.elts]
            arr = self.fresh("_sitems")
            self.emit(f"mp_obj_t {arr}[] = {{{', '.join(vals)}}};")
            tmp = self.fresh("_set")
            self.emit(f"mp_obj_t {tmp} = mp_obj_new_set({len(vals)}, {arr});")
            return tmp
        if isinstance(node, ast.List):
            tmp = self.fresh("_lst")
            owner = tmp
            self._refs.define_empty(owner)
            self.emit(f"mp_obj_t {tmp} = mp_obj_new_list(0, NULL);")
            for elt in node.elts:
                refs = collect_names(elt)
                self._refs.check_store(owner, refs, node.lineno)
                val = self._emit_expr(elt)
                self.emit(f"mp_obj_list_append({tmp}, {val});")
                self._refs.record_store(owner, refs)
            return tmp
        if isinstance(node, ast.Dict):
            tmp = self.fresh("_dct")
            owner = tmp
            self._refs.define_empty(owner)
            self.emit(f"mp_obj_t {tmp} = mp_obj_new_dict(0);")
            for key_node, val_node in zip(node.keys, node.values):
                if key_node is None:
                    raise TranslateError("dict unpacking is not supported", node.lineno)
                refs = collect_names(key_node) | collect_names(val_node)
                self._refs.check_store(owner, refs, node.lineno)
                key = self._emit_expr(key_node)
                val = self._emit_expr(val_node)
                self.emit(f"mp_obj_dict_store({tmp}, {key}, {val});")
                self._refs.record_store(owner, refs)
            return tmp
        if isinstance(node, ast.Tuple):
            if not node.elts:
                return "mp_obj_new_tuple(0, NULL)"
            vals = [self._emit_expr(elt) for elt in node.elts]
            arr = self.fresh("_titems")
            self.emit(f"mp_obj_t {arr}[] = {{{', '.join(vals)}}};")
            tmp = self.fresh("_tpl")
            self.emit(f"mp_obj_t {tmp} = mp_obj_new_tuple({len(vals)}, {arr});")
            return tmp
        if isinstance(node, ast.Call):
            return self._emit_call_expr(node)
        if isinstance(node, ast.JoinedStr):
            raise TranslateError("f-strings are not supported", node.lineno)
        raise TranslateError(f"unsupported expression: {type(node).__name__}", node.lineno)

    def _emit_call_expr(self, node: ast.Call) -> str:
        if isinstance(node.func, ast.Name):
            fname = node.func.id
            if fname in MANIFEST_FUNCS and self.permissive:
                return "mp_const_none"
            if fname == "const":
                if len(node.args) != 1 or node.keywords:
                    raise TranslateError("const() takes exactly one argument", node.lineno)
                return self._emit_expr(node.args[0])
            if fname == "int" and len(node.args) == 1 and not node.keywords:
                arg = self._emit_expr(node.args[0])
                tmp = self.fresh("_int")
                self.emit(f"mp_obj_t {tmp} = mp_unary_op(MP_UNARY_OP_INT_MAYBE, {arg});")
                return tmp
            if fname == "len" and len(node.args) == 1 and not node.keywords:
                arg = self._emit_expr(node.args[0])
                tmp = self.fresh("_len")
                self.emit(f"mp_obj_t {tmp} = mp_obj_len({arg});")
                return tmp
            if fname == "ord" and len(node.args) == 1 and not node.keywords:
                arg = self._emit_expr(node.args[0])
                tmp = self.fresh("_ord")
                self.emit(f"mp_obj_t {tmp} = mp_unary_op(MP_UNARY_OP_INT_MAYBE, {arg});")
                return tmp
            if fname == "hasattr" and self.permissive:
                return "mp_const_false"
            func = self._emit_expr(node.func)
            return self._emit_generic_call(func, node)
        if isinstance(node.func, ast.Attribute):
            return self._emit_method_call(node)
        func = self._emit_expr(node.func)
        return self._emit_generic_call(func, node)

    def _emit_generic_call(self, func_c: str, node: ast.Call) -> str:
        if node.keywords and not self.permissive:
            raise TranslateError("keyword call arguments are not supported", node.lineno)
        nargs = len(node.args)
        tmp = self.fresh("_call")
        if nargs == 0:
            self.emit(f"mp_obj_t {tmp} = mp_call_function_0({func_c});")
        elif nargs == 1 and not node.keywords:
            arg = self._emit_expr(node.args[0])
            self.emit(f"mp_obj_t {tmp} = mp_call_function_1({func_c}, {arg});")
        elif nargs == 2 and not node.keywords:
            a1 = self._emit_expr(node.args[0])
            a2 = self._emit_expr(node.args[1])
            self.emit(f"mp_obj_t {tmp} = mp_call_function_2({func_c}, {a1}, {a2});")
        else:
            arr = self.fresh("_cargs")
            args_c = [self._emit_expr(a) for a in node.args]
            self.emit(f"mp_obj_t {arr}[] = {{{', '.join(args_c)}}};")
            self.emit(f"mp_obj_t {tmp} = mp_call_function_n_kw({func_c}, {nargs}, 0, {arr});")
        return tmp

    def _emit_method_call(self, node: ast.Call) -> str:
        assert isinstance(node.func, ast.Attribute)
        if node.keywords and not self.permissive:
            raise TranslateError("keyword call arguments are not supported", node.lineno)
        obj = self._emit_expr(node.func.value)
        q = self._emit_qstr(node.func.attr)
        dest = self.fresh("_meth")
        self.emit(f"mp_obj_t {dest}[4];")
        self.emit(f"mp_load_method({obj}, {q}, {dest});")
        nargs = len(node.args)
        tmp = self.fresh("_mcall")
        if nargs == 0:
            self.emit(f"mp_obj_t {tmp} = mp_call_method_n_kw(0, 0, NULL);")
        else:
            arr = self.fresh("_margs")
            args_c = [self._emit_expr(a) for a in node.args]
            self.emit(f"mp_obj_t {arr}[] = {{{', '.join(args_c)}}};")
            self.emit(f"mp_obj_t {tmp} = mp_call_method_n_kw({nargs}, 0, {arr});")
        return tmp

    def _emit_unary(self, node: ast.UnaryOp) -> str:
        op = UNARYOPS.get(type(node.op))
        if op is None:
            raise TranslateError("unsupported unary operator", node.lineno)
        arg = self._emit_expr(node.operand)
        tmp = self.fresh("_un")
        self.emit(f"mp_obj_t {tmp} = mp_unary_op({op}, {arg});")
        return tmp

    def _emit_binop(self, node: ast.BinOp) -> str:
        op = BINOPS.get(type(node.op))
        if op is None:
            raise TranslateError("unsupported binary operator", node.lineno)
        left = self._emit_expr(node.left)
        right = self._emit_expr(node.right)
        tmp = self.fresh("_bin")
        self.emit(f"mp_obj_t {tmp} = mp_binary_op({op}, {left}, {right});")
        return tmp

    def _emit_compare(self, node: ast.Compare) -> str:
        if len(node.ops) != 1 or len(node.comparators) != 1:
            raise TranslateError("chained comparisons are not supported", node.lineno)
        op = CMPOPS.get(type(node.ops[0]))
        if op is None:
            raise TranslateError("unsupported comparison operator", node.lineno)
        left = self._emit_expr(node.left)
        right = self._emit_expr(node.comparators[0])
        tmp = self.fresh("_cmp")
        self.emit(f"mp_obj_t {tmp} = mp_binary_op({op}, {left}, {right});")
        return tmp

    def _emit_boolop(self, node: ast.BoolOp) -> str:
        vals = [self._emit_expr(v) for v in node.values]
        tmp = self.fresh("_bool")
        if isinstance(node.op, ast.And):
            self.emit(f"mp_obj_t {tmp} = mp_obj_is_true({vals[0]}) ? {vals[1]} : {vals[0]};")
        elif isinstance(node.op, ast.Or):
            self.emit(f"mp_obj_t {tmp} = mp_obj_is_true({vals[0]}) ? {vals[0]} : {vals[1]};")
        else:
            raise TranslateError("unsupported boolean operator", node.lineno)
        return tmp

    def _emit_ifexp(self, node: ast.IfExp) -> str:
        cond = self._emit_expr(node.test)
        body = self._emit_expr(node.body)
        orelse = self._emit_expr(node.orelse)
        tmp = self.fresh("_ifexp")
        self.emit(f"mp_obj_t {tmp} = mp_obj_is_true({cond}) ? {body} : {orelse};")
        return tmp

    def _const(self, node: ast.Constant) -> str:
        val = node.value
        if isinstance(val, str):
            data, length = c_string_literal(val)
            return f"mp_obj_new_str({data}, {length})"
        if isinstance(val, bool):
            return "mp_const_true" if val else "mp_const_false"
        if isinstance(val, int):
            return f"mp_obj_new_int({val})"
        if isinstance(val, float):
            return f"mp_obj_new_float({repr(val)})"
        if isinstance(val, bytes):
            data, length = c_bytes_literal(val)
            return f"mp_obj_new_bytes({data}, {length})"
        if val is None:
            return "mp_const_none"
        raise TranslateError(f"unsupported constant {type(val).__name__}", node.lineno)


_UNSUPPORTED_STMTS = (
    "Try",
    "TryStar",
    "With",
    "Raise",
    "Return",
    "Global",
    "Nonlocal",
    "Assert",
    "Match",
    "TypeAlias",
)
for _stmt_name in _UNSUPPORTED_STMTS:
    if hasattr(ast, _stmt_name):
        setattr(Emitter, f"visit_{_stmt_name}", Emitter._unsupported_stmt)


def c_string_literal(s: str) -> tuple[str, int]:
    out: list[str] = []
    for ch in s:
        o = ord(ch)
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\r":
            out.append("\\r")
        elif ch == "\t":
            out.append("\\t")
        elif 32 <= o <= 126:
            out.append(ch)
        else:
            out.append(f"\\x{o:02x}")
    encoded = s.encode("utf-8")
    return f"\"{''.join(out)}\"", len(encoded)


def c_bytes_literal(data: bytes) -> tuple[str, int]:
    if not data:
        return "(const byte *)\"\"", 0
    body = ", ".join(f"0x{b:02x}" for b in data)
    return f"(const byte[]){{{body}}}", len(data)


def translate(source: str, filename: str = "<stdin>", *, permissive: bool = False) -> str:
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", SyntaxWarning)
        tree = ast.parse(source, filename=filename, mode="exec")
    emitter = Emitter(permissive=permissive)
    emitter.visit(tree)
    body = "\n".join(f"    {line}" if line else "" for line in emitter.lines)
    return (
        HEADER
        + "\nvoid script_run(void) {\n"
        + body
        + "\n}\n"
    )


def translate_file(path: str | os.PathLike[str], *, permissive: bool = True) -> str:
    path = os.fspath(path)
    with open(path, encoding="utf-8") as f:
        source = f.read()
    return translate(source, filename=path, permissive=permissive)


def try_translate_file(path: str | os.PathLike[str]) -> tuple[str | None, str | None]:
    """Return (c_source, error_message). On success error_message is None."""
    path = os.fspath(path)
    try:
        return translate_file(path), None
    except TranslateError as exc:
        loc = f"{path}:{exc.lineno}" if exc.lineno else path
        return None, f"{loc}: {exc.msg}"
    except SyntaxError as exc:
        return None, f"{path}:{exc.lineno or '?'}: syntax error: {exc.msg}"


def translate_checked(source: str, filename: str = "<stdin>") -> str:
    return translate(source, filename=filename)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", help="Python source file")
    parser.add_argument("-o", "--output", required=True, help="Generated C output path")
    args = parser.parse_args(argv)

    with open(args.input, encoding="utf-8") as f:
        source = f.read()

    try:
        code = translate(source, filename=args.input)
    except TranslateError as exc:
        print(f"{args.input}:{exc.lineno or '?'}: error: {exc.msg}", file=sys.stderr)
        return 1

    with open(args.output, "w", encoding="utf-8") as f:
        f.write(code)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
