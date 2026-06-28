def attrgetter(attr):
    assert "." not in attr

    def _attrgetter(obj):
        return getattr(obj, attr)

    return _attrgetter


def lt(a, b) -> "bool":
    return a < b


def le(a, b) -> "bool":
    return a <= b


def gt(a, b) -> "bool":
    return a > b


def ge(a, b) -> "bool":
    return a >= b


def eq(a, b) -> "bool":
    return a == b


def ne(a, b) -> "bool":
    return a != b


def mod(a, b):
    return a % b


def truediv(a, b):
    return a / b


def floordiv(a, b):
    return a // b
