from .figure import Figure

_current = None
_figures = []


def _active():
    global _current
    if _current is None:
        figure()
    return _current


def figure(num=None, figsize=(6.4, 4.8), dpi=100, clear=False):
    global _current
    fig = Figure(figsize=figsize, dpi=dpi)
    if clear:
        fig.clear()
    _figures.append(fig)
    _current = fig
    return fig


def plot(*args, **kwargs):
    fig = _active()
    if len(args) == 1:
        return fig.plot(range(len(as_list(args[0]))), args[0], **kwargs)
    if len(args) >= 2:
        return fig.plot(args[0], args[1], **kwargs)
    raise TypeError("plot expected at least 1 argument")


def title(label):
    _active().set_title(label)


def xlabel(label):
    _active().set_xlabel(label)


def ylabel(label):
    _active().set_ylabel(label)


def xlim(*args):
    fig = _active()
    if len(args) == 1:
        fig.set_xlim(args[0])
    elif len(args) == 2:
        fig.set_xlim(args[0], args[1])
    else:
        return fig.xlim
    return fig.xlim


def ylim(*args):
    fig = _active()
    if len(args) == 1:
        fig.set_ylim(args[0])
    elif len(args) == 2:
        fig.set_ylim(args[0], args[1])
    else:
        return fig.ylim
    return fig.ylim


def grid(visible=True, **kwargs):
    _active().grid(visible)


def legend():
    return _active().legend_entries()


def savefig(fname, **kwargs):
    _active().savefig(fname, **kwargs)


def close(fig=None):
    global _current
    target = fig or _current
    if target is not None:
        target.close()
    if target is _current:
        _current = None


def clf():
    _active().clear()


def cla():
    _active().clear()


def show():
    pass


def as_list(x):
    try:
        return x.tolist()
    except AttributeError:
        pass
    if isinstance(x, (list, tuple)):
        return list(x)
    return [x]
