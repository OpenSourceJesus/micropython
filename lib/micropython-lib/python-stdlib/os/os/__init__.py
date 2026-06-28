# Replace built-in os module.
from uos import *

# Provide optional dependencies (which may be installed separately).
try:
    from . import path
except ImportError:
    pass

def walk(top):
    try:
        names = ilistdir(top)
    except OSError:
        return
    dirs = []
    files = []
    for name in names:
        is_dir = name[1] & 0x4000
        if is_dir:
            dirs.append(name[0])
        else:
            files.append(name[0])
    yield top, dirs, files
    for d in dirs:
        sub = path.join(top, d) if 'path' in globals() else top + '/' + d
        for item in walk(sub):
            yield item
