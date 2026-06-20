from . import _svg
from ._util import as_floats, as_list


class Figure:
    def __init__(self, figsize=(6.4, 4.8), dpi=100):
        self.figsize = figsize
        self.dpi = dpi
        self.width = int(figsize[0] * dpi)
        self.height = int(figsize[1] * dpi)
        self.title = None
        self.xlabel = None
        self.ylabel = None
        self.xlim = None
        self.ylim = None
        self._grid = False
        self._renderer = _svg.SvgRenderer(self.width, self.height)
        self._line_count = 0
        self._closed = False

    def plot(self, x, y, label=None, color=None, linewidth=1.5):
        xs = as_floats(as_list(x))
        ys = as_floats(as_list(y))
        if len(xs) == 1 and len(ys) > 1:
            xs = list(range(len(ys)))
        elif len(ys) == 1 and len(xs) > 1:
            ys = [ys[0]] * len(xs)
        elif len(xs) != len(ys):
            raise ValueError("x and y must have the same length")
        if color is None:
            color = _svg.default_color(self._line_count)
        self._line_count += 1
        self._renderer.add_line(xs, ys, color, linewidth, label)
        return xs, ys

    def set_title(self, title):
        self.title = title

    def set_xlabel(self, xlabel):
        self.xlabel = xlabel

    def set_ylabel(self, ylabel):
        self.ylabel = ylabel

    def set_xlim(self, left, right=None):
        if right is None:
            self.xlim = (float(left[0]), float(left[1]))
        else:
            self.xlim = (float(left), float(right))

    def set_ylim(self, bottom, top=None):
        if top is None:
            self.ylim = (float(bottom[0]), float(bottom[1]))
        else:
            self.ylim = (float(bottom), float(top))

    def grid(self, visible=True):
        self._grid = bool(visible)
        self._renderer.set_grid(visible)

    def legend_entries(self):
        entries = []
        for line in self._renderer.lines:
            if line["label"]:
                entries.append((line["label"], line["color"]))
        return entries

    def savefig(self, fname, **kwargs):
        if self._closed:
            raise RuntimeError("Figure is closed")
        entries = self.legend_entries()
        legend = entries if kwargs.get("legend", True) and entries else None
        svg = self._renderer.render(
            self.title,
            self.xlabel,
            self.ylabel,
            self.xlim,
            self.ylim,
            legend,
        )
        with open(fname, "w") as f:
            f.write(svg)

    def close(self):
        self._closed = True

    def clear(self):
        self.title = None
        self.xlabel = None
        self.ylabel = None
        self.xlim = None
        self.ylim = None
        self._grid = False
        self._renderer = _svg.SvgRenderer(self.width, self.height)
        self._line_count = 0
