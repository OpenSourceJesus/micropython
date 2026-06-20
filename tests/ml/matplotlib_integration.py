# Integration test: micropython matplotlib (SVG pyplot) + ulab arrays

import os
import json

import matplotlib
import matplotlib.pyplot as plt
from ulab import numpy as np

print("matplotlib version:", matplotlib.__version__)

xs = np.linspace(0, 4, 5)
ys = np.array([0.0, 1.0, 4.0, 9.0, 16.0])
plt.figure(clear=True)
plt.plot(xs, ys, label="y=x^2")
plt.title("ulab ndarray plot")
plt.xlabel("x")
plt.ylabel("y")
plt.grid(True)

out = "/tmp/micropython_matplotlib_test.svg"
plt.savefig(out)
plt.close()

with open(out, "r") as f:
    svg = f.read()

assert "<svg" in svg
assert "polyline" in svg
assert "y=x^2" in svg
os.remove(out)

training_path = "lib/numeric-modules/training.json"
with open(training_path, "r") as f:
    training = json.load(f)

losses = training["epoch_training_losses"][:10]
epochs = list(range(len(losses)))
plt.figure(clear=True)
plt.plot(epochs, losses, label="loss")
plt.title("training loss")
plt.xlabel("epoch")
plt.ylabel("loss")

out2 = "/tmp/micropython_training_plot.svg"
plt.savefig(out2)
plt.close()

with open(out2, "r") as f:
    svg2 = f.read()

assert "training loss" in svg2
os.remove(out2)

print("matplotlib integration ok")
