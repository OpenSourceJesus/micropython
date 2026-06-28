import os
sep = '/'

def normcase(s) -> "str":
    return s

def normpath(s) -> "str":
    return s

def abspath(s):
    if s[0] != '/':
        return os.getcwd() + '/' + s
    return s

def join(*args):
    if type(args[0]) is bytes:
        return b'/'.join(args)
    else:
        return '/'.join(args)

def split(path):
    if path == '':
        return ('', '')
    r = path.rsplit('/', 1)
    if len(r) == 1:
        return ('', path)
    head = r[0]
    if not head:
        head = '/'
    return (head, r[1])

def dirname(path):
    return split(path)[0]

def basename(path):
    return split(path)[1]

def exists(path) -> 'int':
    try:
        os.stat(path)
        return True
    except OSError:
        return False
lexists = exists

def isdir(path):
    try:
        mode = os.stat(path)[0]
        return mode & 16384
    except OSError:
        return False

def isfile(path):
    try:
        return bool(os.stat(path)[0] & 32768)
    except OSError:
        return False

def expanduser(s):
    if s == '~' or s.startswith('~/'):
        h = os.getenv('HOME')
        return h + s[1:]
    if s[0] == '~':
        return '/home/' + s[1:]
    return s

def isabs(path):
    return path.startswith('/')

def splitext(path):
    for i in range(len(path) - 1, -1, -1):
        if path[i] == '.':
            return path[:i], path[i:]
        if path[i] == '/':
            break
    return path, ''

def relpath(path, start=None):
    if start is None:
        start = os.getcwd()
    path = abspath(path)
    start = abspath(start)
    if path == start:
        return '.'
    prefix = start if start.endswith('/') else start + '/'
    if path.startswith(prefix):
        return path[len(prefix):]
    return path

def commonpath(paths):
    if not paths:
        raise ValueError('commonpath() arg is an empty sequence')
    paths = [abspath(p) for p in paths]
    split_paths = [p.split('/') for p in paths]
    common = []
    for parts in zip(*split_paths):
        if len(set(parts)) == 1:
            common.append(parts[0])
        else:
            break
    result = '/'.join(common)
    return result if result else '/'
