# MicroPython-compatible subset of CPython's cffi package.
# Built on top of the built-in `ffi` module (libffi).

try:
    import ffi as _ffi
except ImportError:
    _ffi = None

try:
    import uctypes
except ImportError:
    uctypes = None


def _require_ffi():
    if _ffi is None:
        raise ImportError("ffi module is not available (build with MICROPY_PY_FFI=1)")


# Map a C type name to MicroPython ffi typecode(s).
def _ret_typecode(ctype):
    ctype = ctype.strip()
    if ctype == "void":
        return "v"
    if ctype in ("int", "bool", "DType", "DeviceType", "LogLevel"):
        return "i"
    if ctype in ("unsigned int", "uint32_t"):
        return "I"
    if ctype in ("long", "unsigned long"):
        return "L"
    if ctype in ("uint64_t", "size_t", "uintptr_t"):
        return "Q"
    if ctype == "float":
        return "f"
    if ctype == "double":
        return "d"
    if "const char" in ctype:
        return "s"
    return "p"


def _arg_typecode(ctype):
    ctype = ctype.strip()
    if ctype == "void":
        return None
    if ctype in ("int", "bool", "DType", "DeviceType", "LogLevel"):
        return "i"
    if ctype in ("unsigned int", "uint32_t"):
        return "I"
    if ctype in ("long", "unsigned long"):
        return "L"
    if ctype in ("uint64_t", "size_t", "uintptr_t"):
        return "Q"
    if ctype == "float":
        return "f"
    if ctype == "double":
        return "d"
    if ctype.startswith("const ") and ctype.endswith("*"):
        return "P"
    if ctype.endswith("*") or ctype in ("Tensor", "Module", "Optimizer"):
        return "p"
    if ctype == "char*":
        return "s"
    # Structs passed by value are not supported; pass a pointer instead.
    return "p"


def _parse_args(args_str):
    if not args_str or args_str.strip() in ("void", ""):
        return ""
    parts = []
    depth = 0
    cur = []
    for ch in args_str:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        elif ch == "," and depth == 0:
            parts.append("".join(cur).strip())
            cur = []
            continue
        cur.append(ch)
    tail = "".join(cur).strip()
    if tail:
        parts.append(tail)
    codes = []
    for part in parts:
        if not part:
            continue
        # Drop parameter names, keep the type.
        tokens = part.replace("*", " *").split()
        if len(tokens) >= 2 and not tokens[-1].startswith("*"):
            ctype = " ".join(tokens[:-1])
        else:
            ctype = part
        code = _arg_typecode(ctype)
        if code is not None:
            codes.append(code)
    return "".join(codes)


def _parse_func(line):
    line = line.strip()
    if not line or line.startswith("#") or line.startswith("typedef"):
        return None
    if line.startswith("struct ") or line.startswith("enum "):
        return None
    if "(" not in line:
        return None
    line = line.rstrip(";")
    name_start = line.rfind(" ", 0, line.index("("))
    if name_start < 0:
        return None
    ret = line[:name_start].strip()
    rest = line[name_start + 1 :].strip()
    name_end = rest.index("(")
    name = rest[:name_end].strip()
    args = rest[name_end + 1 : rest.rindex(")")]
    return name, _ret_typecode(ret), _parse_args(args)


class _IntPtr:
    __slots__ = ("_buf", "_arr")

    def __init__(self, buf):
        self._buf = buf
        self._arr = uctypes.array(uctypes.INT32, buf)

    def __getitem__(self, i):
        return self._arr[i]

    def __setitem__(self, i, v):
        self._arr[i] = v


class _LibProxy:
    __slots__ = ("_mod", "_cache")

    def __init__(self, mod):
        self._mod = mod
        self._cache = {}

    def __getattr__(self, name):
        fn = self._cache.get(name)
        if fn is None:
            raise AttributeError(name)
        return fn

    def _bind(self, name, ret, args):
        self._cache[name] = self._mod.func(ret, name, args)


class FFI:
    NULL = 0

    def __init__(self):
        _require_ffi()
        self._lib = None
        self._lib_proxy = None
        self._source_lib = None

    def cdef(self, source, override=False):
        if self._lib is None:
            raise RuntimeError("call dlopen() or set_source() before cdef()")
        for line in source.splitlines():
            parsed = _parse_func(line)
            if parsed is None:
                continue
            name, ret, args = parsed
            if name in self._lib_proxy._cache and not override:
                continue
            self._lib_proxy._bind(name, ret, args)

    def dlopen(self, names):
        if isinstance(names, str):
            names = (names,)
        err = None
        for name in names:
            try:
                handle = None if name in (None, "") else name
                self._lib = _ffi.open(handle)
                self._lib_proxy = _LibProxy(self._lib)
                return self._lib_proxy
            except OSError as e:
                err = e
        raise err

    def set_source(self, module_name, source, libraries=(), **kw):
        # Record library names for a subsequent compile()/dlopen().
        self._source_lib = libraries[0] if libraries else None

    def compile(self, verbose=False):
        if self._source_lib:
            return self.dlopen((f"lib{self._source_lib}.so", f"lib{self._source_lib}.so.0"))
        return self.dlopen((None,))

    def cast(self, ctype, ptr):
        return ptr

    def new(self, ctype, init=None):
        ctype = ctype.strip()
        if ctype.endswith("[]"):
            elem = ctype[:-2].strip()
            if init is None:
                init = []
            if elem == "int":
                n = len(init)
                buf = bytearray(4 * n)
                if uctypes:
                    arr = uctypes.array(uctypes.INT32, buf)
                    for i, v in enumerate(init):
                        arr[i] = v
                else:
                    for i, v in enumerate(init):
                        buf[4 * i] = v & 0xFF
                return buf
            if elem == "float":
                n = len(init)
                buf = bytearray(4 * n)
                if uctypes:
                    arr = uctypes.array(uctypes.FLOAT32, buf)
                    for i, v in enumerate(init):
                        arr[i] = v
                return buf
            if elem == "char":
                if isinstance(init, (bytes, bytearray)):
                    return init
                return bytes(init)
            raise TypeError("unsupported array element: " + elem)
        if ctype.endswith("*"):
            inner = ctype[:-1].strip()
            if inner == "int":
                buf = bytearray(4)
                if init is not None:
                    if uctypes:
                        uctypes.array(uctypes.INT32, buf)[0] = init
                    else:
                        v = int(init)
                        buf[0] = v & 0xFF
                        buf[1] = (v >> 8) & 0xFF
                        buf[2] = (v >> 16) & 0xFF
                        buf[3] = (v >> 24) & 0xFF
                if uctypes:
                    return _IntPtr(buf)
                return buf
            if inner == "float":
                buf = bytearray(4)
                if init is not None and uctypes:
                    uctypes.array(uctypes.FLOAT32, buf)[0] = init
                return buf
            if inner == "const char":
                buf = bytearray(8)
                return buf
            # Opaque struct pointer: allocate a buffer of reasonable size.
            size = 64
            if init is not None:
                buf = bytearray(size)
                if isinstance(init, (bytes, bytearray)):
                    buf[: len(init)] = init
                return buf
            return bytearray(size)
        raise TypeError("unsupported type: " + ctype)

    def addressof(self, buf):
        if uctypes:
            return uctypes.addressof(buf)
        raise RuntimeError("uctypes is required for addressof()")

    @property
    def lib(self):
        if self._lib_proxy is None:
            raise RuntimeError("library not loaded")
        return self._lib_proxy
