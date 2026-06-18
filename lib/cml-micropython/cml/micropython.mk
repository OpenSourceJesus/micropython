CML_MOD_DIR := $(USERMOD_DIR)

# Prefer lib/C-ML submodule, fall back to ~/C-ML for local development.
CML_DIR ?= $(abspath $(USERMOD_DIR)/../../C-ML)
ifeq ($(wildcard $(CML_DIR)/CMakeLists.txt),)
CML_DIR := $(abspath $(HOME)/C-ML)
endif

CML_BUILD := $(CML_DIR)/build
CML_STATIC := $(CML_BUILD)/lib/libcml.a

CML_CMAKE_FLAGS = \
	-DBUILD_TESTS=OFF \
	-DBUILD_EXAMPLES=OFF \
	-DBUILD_MODEL_ZOO=OFF \
	-DBUILD_SHARED_LIBS=OFF \
	-DENABLE_LLVM_BACKEND=OFF \
	-DENABLE_CUDA=OFF \
	-DENABLE_ROCM=OFF \
	-DENABLE_OPENCL=OFF \
	-DENABLE_METAL=OFF \
	-DENABLE_VULKAN=OFF \
	-DENABLE_ONNX=OFF \
	-DENABLE_DISTRIBUTED=OFF

$(CML_STATIC):
	$(ECHO) "Building C-ML static library"
	$(Q)mkdir -p $(CML_BUILD)
	$(Q)cd $(CML_BUILD) && cmake $(CML_CMAKE_FLAGS) $(CML_DIR)
	$(Q)cmake --build $(CML_BUILD) --target cml_static -j$$(nproc)

SRC_USERMOD += \
	$(CML_MOD_DIR)/modcml.c \
	$(CML_MOD_DIR)/modcml_util.c \
	$(CML_MOD_DIR)/modcml_tensor.c \
	$(CML_MOD_DIR)/modcml_nn.c \
	$(CML_MOD_DIR)/modcml_autograd.c \
	$(CML_MOD_DIR)/modcml_optim.c \
	$(CML_MOD_DIR)/modcml_runtime.c
CFLAGS_USERMOD += -I$(CML_DIR)/include -DCML_STATIC_DEFINE
LDFLAGS_USERMOD += $(CML_STATIC) -lm -ldl -lpthread

$(BUILD)/cml/modcml.o: $(CML_STATIC)
