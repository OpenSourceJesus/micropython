# Integration test: micropython-ulab (numpy + scipy) + C-ML (torch_c)

import cml
import torch
from ulab import numpy as np
from ulab import scipy

cml.init()
print("cml version:", cml.version())

# ulab numpy
a = np.array([[1.0, 2.0], [3.0, 4.0]])
b = np.ones((2, 2))
c = np.dot(a, b)
print("numpy dot:", c)

# ulab scipy
vals = np.array([0.0, 1.0, 2.0])
erf = scipy.special.erf(vals)
print("scipy erf:", erf[1])

# ndarray -> torch_c tensor (buffer protocol, no tolist copy)
t = cml.tensor(a)
assert t.shape == (2, 2)
assert t.tolist() == [[1.0, 2.0], [3.0, 4.0]]

# torch_c nn on tensor data
linear = cml.nn_linear(2, 2)
out = linear.forward(t)
print("linear out shape:", out.shape)

# torch module alias
assert torch.matmul(t, cml.ones((2, 2))).shape == (2, 2)

# tensor -> ndarray round trip
back = np.array(out.tolist())
print("round-trip shape:", back.shape)

cml.cleanup()
print("ml integration ok")
