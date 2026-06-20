_DEFAULT_COLORS = (
    "#1f77b4",
    "#ff7f0e",
    "#2ca02c",
    "#d62728",
    "#9467bd",
    "#8c564b",
    "#e377c2",
    "#7f7f7f",
    "#bcbd22",
    "#17becf",
)


def default_color(index):
    return _DEFAULT_COLORS[index % len(_DEFAULT_COLORS)]


def _esc(text):
    return (
        str(text)
        .replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
    )


def _map_x(x, x0, x1, px0, px1):
    if x1 == x0:
        return (px0 + px1) * 0.5
    return px0 + (x - x0) * (px1 - px0) / (x1 - x0)


def _map_y(y, y0, y1, py0, py1):
    if y1 == y0:
        return (py0 + py1) * 0.5
    return py1 - (y - y0) * (py1 - py0) / (y1 - y0)


class SvgRenderer:
    def __init__(self, width, height, margin=48):
        self.width = width
        self.height = height
        self.margin = margin
        self.plot_left = margin
        self.plot_right = width - margin
        self.plot_top = margin
        self.plot_bottom = height - margin
        self.lines = []
        self.texts = []
        self.grid = False

    def set_grid(self, enabled):
        self.grid = bool(enabled)

    def add_line(self, xs, ys, color, linewidth, label):
        self.lines.append(
            {
                "xs": xs,
                "ys": ys,
                "color": color,
                "linewidth": linewidth,
                "label": label,
            }
        )

    def add_text(self, x, y, text, size=12, anchor="middle", color="#222222"):
        self.texts.append(
            {
                "x": x,
                "y": y,
                "text": text,
                "size": size,
                "anchor": anchor,
                "color": color,
            }
        )

    def render(self, title, xlabel, ylabel, xlim, ylim, legend):
        xs_all = []
        ys_all = []
        for line in self.lines:
            xs_all.extend(line["xs"])
            ys_all.extend(line["ys"])

        if xlim is None:
            xlim = axis_limits(xs_all)
        if ylim is None:
            ylim = axis_limits(ys_all)

        parts = [
            '<?xml version="1.0" encoding="UTF-8"?>',
            '<svg xmlns="http://www.w3.org/2000/svg" width="{}" height="{}">'.format(
                self.width, self.height
            ),
            '<rect width="100%" height="100%" fill="white"/>',
        ]

        if self.grid:
            for i in range(6):
                t = i / 5.0
                gx = self.plot_left + t * (self.plot_right - self.plot_left)
                gy = self.plot_top + t * (self.plot_bottom - self.plot_top)
                parts.append(
                    '<line x1="{:.2f}" y1="{:.2f}" x2="{:.2f}" y2="{:.2f}" stroke="#dddddd" stroke-width="1"/>'.format(
                        gx, self.plot_top, gx, self.plot_bottom
                    )
                )
                parts.append(
                    '<line x1="{:.2f}" y1="{:.2f}" x2="{:.2f}" y2="{:.2f}" stroke="#dddddd" stroke-width="1"/>'.format(
                        self.plot_left, gy, self.plot_right, gy
                    )
                )

        parts.append(
            '<rect x="{:.2f}" y="{:.2f}" width="{:.2f}" height="{:.2f}" fill="none" stroke="#222222" stroke-width="1"/>'.format(
                self.plot_left,
                self.plot_top,
                self.plot_right - self.plot_left,
                self.plot_bottom - self.plot_top,
            )
        )

        for line in self.lines:
            if len(line["xs"]) < 2:
                continue
            coords = []
            for x, y in zip(line["xs"], line["ys"]):
                px = _map_x(x, xlim[0], xlim[1], self.plot_left, self.plot_right)
                py = _map_y(y, ylim[0], ylim[1], self.plot_top, self.plot_bottom)
                coords.append("{:.2f},{:.2f}".format(px, py))
            parts.append(
                '<polyline fill="none" stroke="{}" stroke-width="{:.2f}" points="{}"/>'.format(
                    line["color"], line["linewidth"], " ".join(coords)
                )
            )

        if title:
            parts.append(
                '<text x="{:.2f}" y="{:.2f}" font-family="sans-serif" font-size="14" text-anchor="middle" fill="#222222">{}</text>'.format(
                    self.width * 0.5, self.margin * 0.55, _esc(title)
                )
            )

        if xlabel:
            parts.append(
                '<text x="{:.2f}" y="{:.2f}" font-family="sans-serif" font-size="12" text-anchor="middle" fill="#222222">{}</text>'.format(
                    self.width * 0.5, self.height - self.margin * 0.25, _esc(xlabel)
                )
            )

        if ylabel:
            parts.append(
                '<text x="{:.2f}" y="{:.2f}" font-family="sans-serif" font-size="12" text-anchor="middle" fill="#222222" transform="rotate(-90 {:.2f} {:.2f})">{}</text>'.format(
                    self.margin * 0.35,
                    self.height * 0.5,
                    self.margin * 0.35,
                    self.height * 0.5,
                    _esc(ylabel),
                )
            )

        if legend:
            lx = self.plot_right - 8
            ly = self.plot_top + 16
            for i, (label, color) in enumerate(legend):
                y = ly + i * 16
                parts.append(
                    '<line x1="{:.2f}" y1="{:.2f}" x2="{:.2f}" y2="{:.2f}" stroke="{}" stroke-width="2"/>'.format(
                        lx - 28, y, lx - 8, y, color
                    )
                )
                parts.append(
                    '<text x="{:.2f}" y="{:.2f}" font-family="sans-serif" font-size="11" text-anchor="start" fill="#222222">{}</text>'.format(
                        lx - 4, y + 4, _esc(label)
                    )
                )

        for item in self.texts:
            parts.append(
                '<text x="{:.2f}" y="{:.2f}" font-family="sans-serif" font-size="{}" text-anchor="{}" fill="{}">{}</text>'.format(
                    item["x"],
                    item["y"],
                    item["size"],
                    item["anchor"],
                    item["color"],
                    _esc(item["text"]),
                )
            )

        parts.append("</svg>")
        return "\n".join(parts)


def axis_limits(values, pad=0.05):
    if not values:
        return 0.0, 1.0
    lo = min(values)
    hi = max(values)
    if lo == hi:
        delta = 1.0 if lo == 0 else abs(lo) * 0.1
        return lo - delta, hi + delta
    span = hi - lo
    return lo - span * pad, hi + span * pad
