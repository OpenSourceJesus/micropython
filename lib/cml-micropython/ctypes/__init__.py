# MicroPython-compatible subset of CPython's ctypes package.
# Built on the built-in `ffi` and `uctypes` modules.

try:
    import ffi as _ffi
except ImportError:
    _ffi = None

try:
    import uctypes
except ImportError:
    uctypes = None

try:
    import ffilib
except ImportError:
    ffilib = None

_LAYOUT = uctypes.NATIVE if uctypes else 0
_PTR_SIZE = 8


def _require_ffi():
    if _ffi is None:
        raise ImportError("ffi module is not available (build with MICROPY_PY_FFI=1)")


def _require_uctypes():
    if uctypes is None:
        raise ImportError("uctypes module is not available")


def _ffi_code(typ):
    if typ is None:
        return "v"
    if isinstance(typ, _PointerType):
        return "p"
    if isinstance(typ, type) and issubclass(typ, _CType):
        return typ._code
    raise TypeError("unsupported ctype")


class _CType:
    _code = "i"
    _size = 4

    def __init__(self, value=0):
        _require_uctypes()
        self._buf = bytearray(self._size)
        self.value = value

    def _desc(self):
        return {"v": _scalar_uctype(self._code) | 0}

    @property
    def value(self):
        if self._size == 1:
            return self._buf[0]
        s = uctypes.struct(uctypes.addressof(self._buf), self._desc(), _LAYOUT)
        return s.v

    @value.setter
    def value(self, v):
        if self._size == 1:
            self._buf[0] = v & 0xFF
            return
        s = uctypes.struct(uctypes.addressof(self._buf), self._desc(), _LAYOUT)
        s.v = v


def _scalar_uctype(code):
    _require_uctypes()
    return {
        "b": uctypes.INT8,
        "B": uctypes.UINT8,
        "h": uctypes.INT16,
        "H": uctypes.UINT16,
        "i": uctypes.INT32,
        "I": uctypes.UINT32,
        "l": uctypes.INT32,
        "L": uctypes.UINT32,
        "q": uctypes.INT64,
        "Q": uctypes.UINT64,
        "f": uctypes.FLOAT32,
        "d": uctypes.FLOAT64,
        "p": uctypes.UINT64 if _PTR_SIZE == 8 else uctypes.UINT32,
        "s": uctypes.UINT64 if _PTR_SIZE == 8 else uctypes.UINT32,
    }[code]


class c_byte(_CType):
    _code = "b"
    _size = 1


class c_char(c_byte):
    pass


class c_ubyte(_CType):
    _code = "B"
    _size = 1


class c_short(_CType):
    _code = "h"
    _size = 2


class c_ushort(_CType):
    _code = "H"
    _size = 2


class c_int(_CType):
    _code = "i"
    _size = 4


class c_uint(_CType):
    _code = "I"
    _size = 4


class c_long(_CType):
    _code = "l"
    _size = 4


class c_ulong(_CType):
    _code = "L"
    _size = 4


class c_longlong(_CType):
    _code = "q"
    _size = 8


class c_ulonglong(_CType):
    _code = "Q"
    _size = 8


class c_float(_CType):
    _code = "f"
    _size = 4


class c_double(_CType):
    _code = "d"
    _size = 8


class c_bool(_CType):
    _code = "i"
    _size = 4


class c_void_p(_CType):
    _code = "p"
    _size = _PTR_SIZE


class c_char_p(_CType):
    _code = "s"
    _size = _PTR_SIZE


class c_size_t(_CType):
    _code = "Q" if _PTR_SIZE == 8 else "L"
    _size = _PTR_SIZE


class _PointerType:
    __slots__ = ("_base",)

    def __init__(self, base):
        self._base = base


def POINTER(typ):
    return _PointerType(typ)


def _type_size(typ):
    if isinstance(typ, _PointerType):
        return _PTR_SIZE
    if isinstance(typ, type):
        if issubclass(typ, _CType):
            return typ._size
        if issubclass(typ, Structure):
            return typ._size_
    raise TypeError("unsupported type")


def _build_fields(fields):
    _require_uctypes()
    desc = {}
    offset = 0
    for name, typ in fields:
        if isinstance(typ, _PointerType):
            ut = _scalar_uctype("p")
            size = _PTR_SIZE
        elif isinstance(typ, type) and issubclass(typ, Structure):
            ut = typ._uctypes_desc_
            size = typ._size_
        elif isinstance(typ, type) and issubclass(typ, _CType):
            ut = _scalar_uctype(typ._code)
            size = typ._size
        else:
            raise TypeError("unsupported field type")
        desc[name] = ut | offset
        offset += size
    return desc, offset


def sizeof(obj):
    if isinstance(obj, type):
        if issubclass(obj, Structure):
            obj._ensure_layout()
            return obj._size_
        if issubclass(obj, _CType):
            return obj._size
    if isinstance(obj, _PointerType):
        return _PTR_SIZE
    if isinstance(obj, (_CType, Structure, Array)):
        return len(obj._buf)
    raise TypeError("sizeof() argument must be a ctype")


class Structure:
    _fields_ = ()
    _size_ = 0
    _uctypes_desc_ = {}

    @classmethod
    def _ensure_layout(cls):
        if cls._size_ == 0 and cls._fields_:
            cls._uctypes_desc_, cls._size_ = _build_fields(cls._fields_)

    def __init__(self, *args, **kw):
        _require_uctypes()
        self.__class__._ensure_layout()
        self._buf = bytearray(self._size_)
        self._s = uctypes.struct(
            uctypes.addressof(self._buf), self._uctypes_desc_, _LAYOUT
        )
        names = [n for n, _ in self._fields_]
        for i, val in enumerate(args):
            setattr(self, names[i], val)
        for k, v in kw.items():
            setattr(self, k, v)

    def __getattr__(self, name):
        if name.startswith("_"):
            raise AttributeError(name)
        return getattr(self._s, name)

    def __setattr__(self, name, val):
        if name.startswith("_"):
            super().__setattr__(name, val)
        else:
            setattr(self._s, name, val)


class Array:
    def __init__(self, typ, init):
        _require_uctypes()
        if not isinstance(init, (list, tuple)):
            raise TypeError("array init must be a list or tuple")
        if isinstance(typ, _PointerType):
            elem = _scalar_uctype("p")
            esize = _PTR_SIZE
        elif isinstance(typ, type) and issubclass(typ, _CType):
            elem = _scalar_uctype(typ._code)
            esize = typ._size
        else:
            raise TypeError("unsupported array element type")
        self._buf = bytearray(esize * len(init))
        desc = {"a": (uctypes.ARRAY | 0, elem | len(init))}
        arr = uctypes.struct(uctypes.addressof(self._buf), desc, _LAYOUT)
        for i, v in enumerate(init):
            arr.a[i] = v


class _CFunc:
    __slots__ = ("_mod", "_name", "argtypes", "restype", "_fn")

    def __init__(self, mod, name):
        self._mod = mod
        self._name = name
        self.argtypes = None
        self.restype = c_int
        self._fn = None

    def _bind(self):
        ret = _ffi_code(self.restype)
        args = ""
        if self.argtypes:
            args = "".join(_ffi_code(t) for t in self.argtypes)
        self._fn = self._mod.func(ret, self._name, args)

    def __call__(self, *args):
        if self._fn is None:
            self._bind()
        conv = []
        for a in args:
            if isinstance(a, Structure):
                conv.append(a._buf)
            elif isinstance(a, Array):
                conv.append(a._buf)
            elif isinstance(a, _CType):
                conv.append(a._buf)
            else:
                conv.append(a)
        return self._fn(*conv)


class CDLL:
    def __init__(self, name, mode=-1, handle=None, use_errno=False, use_last_error=False):
        _require_ffi()
        if handle is not None:
            self._mod = handle
        elif name in (None, ""):
            self._mod = _ffi.open(None)
        elif ffilib is not None and isinstance(name, str) and "/" not in name and not name.endswith(".so"):
            self._mod = ffilib.open(name)
        else:
            self._mod = _ffi.open(name)
        self._cache = {}

    def __getattr__(self, name):
        if name.startswith("_"):
            raise AttributeError(name)
        fn = self._cache.get(name)
        if fn is None:
            fn = _CFunc(self._mod, name)
            self._cache[name] = fn
        return fn


class _CDLLProxy:
    def __getattr__(self, name):
        if name == "LoadLibrary":
            return CDLL
        raise AttributeError(name)

    def __call__(self, name):
        return CDLL(name)


cdll = _CDLLProxy()


def addressof(obj):
    _require_uctypes()
    if isinstance(obj, (_CType, Structure, Array)):
        return uctypes.addressof(obj._buf)
    return uctypes.addressof(obj)


def byref(obj, offset=0):
    if isinstance(obj, (_CType, Structure, Array)):
        if offset:
            return uctypes.bytearray_at(addressof(obj) + offset, len(obj._buf) - offset)
        return obj._buf
    raise TypeError("byref() argument must be a ctypes object")


def cast(typ, value):
    if isinstance(typ, _PointerType):
        return value
    if isinstance(typ, type) and issubclass(typ, Structure):
        _require_uctypes()
        s = typ.__new__(typ)
        s._buf = uctypes.bytearray_at(value, typ._size_)
        s._s = uctypes.struct(value, typ._uctypes_desc_, _LAYOUT)
        return s
    raise TypeError("cast() not supported for this type")


def CFUNCTYPE(restype, *argtypes):
    _require_ffi()

    def factory(pyfunc):
        ret = _ffi_code(restype)
        args = "".join(_ffi_code(t) for t in argtypes)
        return _ffi.callback(ret, pyfunc, args)

    return factory
