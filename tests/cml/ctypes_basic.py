# Test MicroPython ctypes compatibility shim.

try:
    import ffi
except ImportError:
    print("SKIP: ctypes requires ffi module")
    raise SystemExit

import ctypes
from ctypes import (
    CDLL,
    CFUNCTYPE,
    POINTER,
    Structure,
    addressof,
    byref,
    c_bool,
    c_int,
    c_void_p,
    cdll,
    sizeof,
)

assert sizeof(c_int) == 4
assert sizeof(c_void_p) == 8

# Structure layout
class Point(Structure):
    _fields_ = [("x", c_int), ("y", c_int)]


pt = Point(3, 4)
print("point:", pt.x, pt.y)
assert pt.x == 3 and pt.y == 4
assert sizeof(Point) == 8

# libc via cdll
libc = cdll.LoadLibrary("libc")
libc.getpid.argtypes = []
libc.getpid.restype = c_int
pid = libc.getpid()
print("getpid:", pid)
assert pid > 0

# argtypes/restype pattern (as used by C-ML distributed.py)
lib = CDLL(None)
lib.cml_is_initialized.argtypes = []
lib.cml_is_initialized.restype = c_bool
print("cml_is_initialized:", lib.cml_is_initialized())

# CFUNCTYPE + bytearray_at pattern from ffi examples
import uctypes

libc.qsort = CDLL("libc").qsort
libc.qsort.argtypes = [c_void_p, c_int, c_int, c_void_p]
libc.qsort.restype = None

CMP = CFUNCTYPE(c_int, c_void_p, c_void_p)


def cmp(a, b):
    x = uctypes.bytearray_at(a, 1)[0]
    y = uctypes.bytearray_at(b, 1)[0]
    return x - y


cb = CMP(cmp)
data = bytearray(b"dcba")
libc.qsort(data, len(data), 1, cb)
print("qsort:", bytes(data))
assert bytes(data) == b"abcd"

# byref / addressof
n = c_int(42)
print("addressof:", addressof(n))
assert n.value == 42

print("ctypes ok")
