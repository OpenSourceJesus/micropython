#!/usr/bin/env python3
"""Tests for ast2objcore.py — Python AST to MicroPython C transpilation."""

from __future__ import annotations

import unittest

from ast2objcore import TranslateError, translate


def transpile(source: str) -> str:
    return translate(source, filename="<test>")


def expect_ok(source: str, *fragments: str) -> str:
    code = transpile(source)
    for frag in fragments:
        assert frag in code, f"missing {frag!r} in:\n{code}"
    return code


def expect_error(source: str, msg_part: str) -> None:
    with unittest.TestCase().assertRaises(TranslateError) as ctx:
        transpile(source)
    assert msg_part in ctx.exception.msg, ctx.exception.msg


class TestLiterals(unittest.TestCase):
    def test_print_string(self) -> None:
        expect_ok('print("hi")', 'mp_obj_new_str("hi", 2)', "mp_obj_print_helper")

    def test_print_int(self) -> None:
        expect_ok("print(42)", "mp_obj_new_int(42)")

    def test_print_float(self) -> None:
        expect_ok("print(3.5)", "mp_obj_new_float(3.5)")

    def test_print_bool(self) -> None:
        expect_ok("print(True)", "mp_const_true")
        expect_ok("print(False)", "mp_const_false")

    def test_print_none(self) -> None:
        expect_ok("print(None)", "mp_const_none")

    def test_print_multiple(self) -> None:
        code = expect_ok('print("a", 1)', 'mp_obj_new_str("a", 1)')
        assert code.count("mp_obj_print_helper") == 2

    def test_print_sep_end(self) -> None:
        expect_ok('print(1, 2, sep=",", end="!")', 'mp_hal_stdout_tx_strn(",", 1)')
        expect_ok('print(1, 2, sep=",", end="!")', 'mp_hal_stdout_tx_strn("!", 1)')


class TestOperators(unittest.TestCase):
    def test_arithmetic(self) -> None:
        expect_ok("print(1 + 2)", "MP_BINARY_OP_ADD")
        expect_ok("print(5 - 1)", "MP_BINARY_OP_SUBTRACT")
        expect_ok("print(3 * 4)", "MP_BINARY_OP_MULTIPLY")
        expect_ok("print(7 / 2)", "MP_BINARY_OP_TRUE_DIVIDE")
        expect_ok("print(7 // 2)", "MP_BINARY_OP_FLOOR_DIVIDE")
        expect_ok("print(7 % 3)", "MP_BINARY_OP_MODULO")
        expect_ok("print(2 ** 8)", "MP_BINARY_OP_POWER")

    def test_bitwise(self) -> None:
        expect_ok("print(1 & 3)", "MP_BINARY_OP_AND")
        expect_ok("print(1 | 2)", "MP_BINARY_OP_OR")
        expect_ok("print(3 ^ 1)", "MP_BINARY_OP_XOR")
        expect_ok("print(1 << 4)", "MP_BINARY_OP_LSHIFT")
        expect_ok("print(8 >> 2)", "MP_BINARY_OP_RSHIFT")

    def test_unary(self) -> None:
        expect_ok("print(-1)", "MP_UNARY_OP_NEGATIVE")
        expect_ok("print(+1)", "MP_UNARY_OP_POSITIVE")
        expect_ok("print(not True)", "MP_UNARY_OP_NOT")
        expect_ok("print(~0)", "MP_UNARY_OP_INVERT")

    def test_compare(self) -> None:
        expect_ok("print(1 < 2)", "MP_BINARY_OP_LESS")
        expect_ok("print(1 <= 2)", "MP_BINARY_OP_LESS_EQUAL")
        expect_ok("print(2 > 1)", "MP_BINARY_OP_MORE")
        expect_ok("print(2 >= 2)", "MP_BINARY_OP_MORE_EQUAL")
        expect_ok("print(1 == 1)", "MP_BINARY_OP_EQUAL")
        expect_ok("print(1 != 2)", "MP_BINARY_OP_NOT_EQUAL")

    def test_bool_and_or_ifexp(self) -> None:
        expect_ok("print(True and False)", "mp_obj_is_true")
        expect_ok("print(True or False)", "mp_obj_is_true")
        expect_ok("print(1 if True else 2)", "mp_obj_is_true")


class TestVariables(unittest.TestCase):
    def test_assign_and_use(self) -> None:
        expect_ok("a = 1\nprint(a)", "mp_obj_new_int(1)")

    def test_augassign(self) -> None:
        expect_ok("a = 1\na += 2", "MP_BINARY_OP_INPLACE_ADD")

    def test_undefined_name(self) -> None:
        expect_error("print(x)", "undefined name 'x'")


class TestContainers(unittest.TestCase):
    def test_empty_list(self) -> None:
        expect_ok("a = []\nprint(a)", "mp_obj_new_list(0, NULL)")

    def test_list_literal(self) -> None:
        expect_ok("a = [1, 2]", "mp_obj_list_append")

    def test_list_append(self) -> None:
        expect_ok("a = []\na.append(1)", "mp_obj_list_append")

    def test_dict_literal(self) -> None:
        expect_ok('d = {"k": 1}', "mp_obj_new_dict", "mp_obj_dict_store")

    def test_tuple_literal(self) -> None:
        expect_ok("t = (1, 2)", "mp_obj_new_tuple(2")

    def test_subscript_load(self) -> None:
        expect_ok("a = [1, 2]\nprint(a[0])", "MP_OBJ_SENTINEL")

    def test_subscript_store(self) -> None:
        expect_ok("a = [0, 0]\na[1] = 9", "mp_obj_subscr")

    def test_len_as_stmt(self) -> None:
        expect_ok("a = [1, 2, 3]\nlen(a)", "mp_obj_len")


class TestControlFlow(unittest.TestCase):
    def test_if(self) -> None:
        expect_ok("if True:\n    print(1)", "if (!mp_obj_is_true", "goto ")

    def test_if_else(self) -> None:
        code = expect_ok("if False:\n    print(1)\nelse:\n    print(2)")
        assert code.count("goto ") >= 2

    def test_while(self) -> None:
        expect_ok("while False:\n    pass", "goto ")

    def test_for_range(self) -> None:
        expect_ok("for i in range(3):\n    print(i)", "MP_BINARY_OP_LESS")

    def test_for_iter(self) -> None:
        expect_ok("for x in [1, 2]:\n    print(x)", "mp_getiter", "mp_iternext")

    def test_break_continue(self) -> None:
        expect_ok("while True:\n    break", "goto ")
        expect_ok("while True:\n    continue", "goto ")

    def test_break_outside_loop(self) -> None:
        expect_error("break", "break outside loop")


class TestCycles(unittest.TestCase):
    def test_list_append_self(self) -> None:
        expect_error("a = []\na.append(a)", "cyclic reference")

    def test_list_literal_self(self) -> None:
        expect_error("a = [a]", "cyclic reference")

    def test_dict_self_value(self) -> None:
        expect_error('d = {}\nd["k"] = d', "cyclic reference")

    def test_dict_self_key(self) -> None:
        expect_error("d = {}\nd[d] = 1", "cyclic reference")

    def test_subscript_self(self) -> None:
        expect_error("a = [0]\na[0] = a", "cyclic reference")

    def test_indirect_two_var_cycle(self) -> None:
        expect_error("a = []\nb = [a]\na.append(b)", "cyclic reference")

    def test_literal_indirect_cycle(self) -> None:
        expect_error("a = []\nb = []\na = [b]\nb = [a]", "cyclic reference")

    def test_no_cycle_chain(self) -> None:
        expect_ok("a = []\nb = [a]\nprint(b)", "mp_obj_list_append")


class TestUnsupported(unittest.TestCase):
    def test_fstring(self) -> None:
        expect_error('print(f"x")', "f-strings")

    def test_chained_compare(self) -> None:
        expect_error("print(1 < 2 < 3)", "chained comparisons")

    def test_while_else(self) -> None:
        expect_error("while False:\n    pass\nelse:\n    pass", "while/else")

    def test_import(self) -> None:
        expect_error("import os", "import statements")


class TestFunctions(unittest.TestCase):
    def test_simple_function_return(self) -> None:
        src = """
def add(a, b):
    return a + b
"""
        code = translate(src, permissive=True)
        assert "static mp_obj_t fn_add" in code
        assert "return" in code
        assert "MP_BINARY_OP_ADD" in code

    def test_function_raise_valueerror(self) -> None:
        src = """
def check(x):
    if x < 0:
        raise ValueError("bad")
    return x
"""
        code = translate(src, permissive=True)
        assert "mp_raise_ValueError" in code
        assert "MP_BINARY_OP_LESS" in code


class TestIntegrationSnippet(unittest.TestCase):
    def test_hello_world(self) -> None:
        expect_ok('print("hello world")', 'mp_obj_new_str("hello world", 11)')

    def test_small_program(self) -> None:
        src = """
a = 40
b = 2
print(a + b)
items = [1, 2, 3]
for x in items:
    print(x)
"""
        code = transpile(src)
        assert "mp_obj_new_list" in code
        assert "mp_getiter" in code
        assert "MP_BINARY_OP_ADD" in code


if __name__ == "__main__":
    unittest.main()
