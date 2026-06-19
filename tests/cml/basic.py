import cml

cml.init()
print("cml version:", cml.version())

# Native Tensor objects
a = cml.zeros((2, 3))
b = cml.ones((3, 2))
c = cml.matmul(a, b)
print("matmul shape:", c.shape, "tolist:", c.tolist())

x = cml.tensor([[1.0, 2.0], [3.0, 4.0]])
y = cml.tensor([[5.0, 6.0], [7.0, 8.0]])
z = cml.matmul(x, y)
print("matmul [[1,2],[3,4]] @ [[5,6],[7,8]] =", z.tolist())

s = cml.add(x, y)
print("add =", s.tolist())

# torch_c alias names
assert cml.torch_zeros((1, 1)).shape == (1, 1)

# nn module smoke test
model = cml.nn_sequential()
l1 = cml.nn_linear(4, 2)
l2 = cml.nn_relu()
model.add(l1)
model.add(l2)
model.eval()
inp = cml.randn((8, 4))
out = model.forward(inp)
print("sequential out shape:", out.shape)

linear = cml.nn_linear(4, 2)
out = linear.forward(inp)
print("linear out shape:", out.shape)

cml.cleanup()
print("cml basic ok")
