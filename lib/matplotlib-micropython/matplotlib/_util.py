def as_list(x):
    if x is None:
        return None
    try:
        return x.tolist()
    except AttributeError:
        pass
    if isinstance(x, (list, tuple)):
        return list(x)
    return [x]


def as_floats(seq):
    return [float(v) for v in seq]


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
