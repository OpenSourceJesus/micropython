# Matplotlib for MicroPython

Pure-Python **matplotlib** / **pyplot** subset for the `ml` unix variant. It renders line plots to SVG and works with Python lists and **ulab** ndarrays.

The top-level `matplotlib/` git submodule ([OpenSourceJesus/matplotlib](https://github.com/OpenSourceJesus/matplotlib)) is upstream CPython Matplotlib (reference only). On MicroPython, use:

```python
import matplotlib.pyplot as plt
```

## Build

Included automatically when building the `ml` variant:

```bash
/path/to/micropython/build-ml.sh
```

The package is frozen via `ports/unix/variants/ml/manifest.py`.

## Supported API

- `matplotlib.__version__`
- `matplotlib.pyplot.figure`, `plot`, `title`, `xlabel`, `ylabel`
- `xlim`, `ylim`, `grid`, `legend`, `savefig`, `close`, `clf`, `cla`, `show`

Output format is SVG (`.svg`).

## Example

```python
from ulab import numpy as np
import matplotlib.pyplot as plt

x = np.linspace(0, 1, 20)
y = np.sin(x * 3.14159)
plt.figure()
plt.plot(x, y, label="sin")
plt.title("wave")
plt.grid(True)
plt.savefig("wave.svg")
plt.close()
```

## Training metrics

Plot C-ML training history from `lib/numeric-modules/training.json`:

```python
import json
import matplotlib.pyplot as plt

with open("lib/numeric-modules/training.json") as f:
    data = json.load(f)

epochs = list(range(len(data["epoch_training_losses"])))
plt.figure()
plt.plot(epochs, data["epoch_training_losses"], label="loss")
plt.plot(epochs, data["epoch_training_accuracies"], label="accuracy")
plt.legend()
plt.savefig("training.svg")
plt.close()
```
