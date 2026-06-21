#!/usr/bin/env python3
"""Download or update tools/py2c.py from the ShivyC repository."""

import os
import sys
import tempfile
import urllib.error
import urllib.request

PY2C_URL = "https://raw.githubusercontent.com/brentharts/ShivyC/master/tools/py2c.py"


def main():
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), "py2c.py")
    existed = os.path.exists(dest)
    verb = "Updating" if existed else "Installing"
    print("%s %s from %s" % (verb, dest, PY2C_URL))
    try:
        with urllib.request.urlopen(PY2C_URL) as resp:
            data = resp.read()
    except urllib.error.URLError as e:
        print("error: failed to download py2c.py: %s" % e, file=sys.stderr)
        sys.exit(1)
    fd, tmp = tempfile.mkstemp(dir=os.path.dirname(dest), prefix=".py2c.", suffix=".tmp")
    try:
        with os.fdopen(fd, "wb") as f:
            f.write(data)
        os.replace(tmp, dest)
    except OSError:
        os.unlink(tmp)
        raise
    os.chmod(dest, 0o755)
    print("Done.")


if __name__ == "__main__":
    main()
