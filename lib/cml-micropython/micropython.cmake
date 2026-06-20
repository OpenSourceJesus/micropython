# C-ML user module for CMake-based MicroPython ports (rp2, esp32, ...).

set(CML_DIR "${CMAKE_CURRENT_LIST_DIR}/../C-ML")
if(NOT EXISTS "${CML_DIR}/CMakeLists.txt")
    set(CML_DIR "$ENV{HOME}/C-ML")
endif()

set(CML_BUILD "${CML_DIR}/build")
set(CML_STATIC "${CML_BUILD}/lib/libcml.a")

set(CML_CMAKE_FLAGS
    -DBUILD_TESTS=OFF
    -DBUILD_EXAMPLES=OFF
    -DBUILD_MODEL_ZOO=OFF
    -DBUILD_SHARED_LIBS=OFF
    -DENABLE_LLVM_BACKEND=OFF
    -DENABLE_CUDA=OFF
    -DENABLE_ROCM=OFF
    -DENABLE_OPENCL=OFF
    -DENABLE_METAL=OFF
    -DENABLE_VULKAN=OFF
    -DENABLE_ONNX=OFF
    -DENABLE_DISTRIBUTED=OFF
)

add_custom_command(
    OUTPUT "${CML_STATIC}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CML_BUILD}"
    COMMAND ${CMAKE_COMMAND} ${CML_CMAKE_FLAGS} -S "${CML_DIR}" -B "${CML_BUILD}"
    COMMAND ${CMAKE_COMMAND} --build "${CML_BUILD}" --target cml_static
    COMMENT "Building C-ML static library"
    VERBATIM
)

set(CML_MOD_DIR "${CMAKE_CURRENT_LIST_DIR}/cml")

add_library(usermod_cml INTERFACE)
target_sources(usermod_cml INTERFACE
    ${CML_MOD_DIR}/modcml.c
    ${CML_MOD_DIR}/modcml_util.c
    ${CML_MOD_DIR}/modcml_tensor.c
    ${CML_MOD_DIR}/modcml_nn.c
    ${CML_MOD_DIR}/modcml_autograd.c
    ${CML_MOD_DIR}/modcml_optim.c
    ${CML_MOD_DIR}/modcml_runtime.c
)
target_include_directories(usermod_cml INTERFACE
    ${CML_MOD_DIR}
    ${CML_DIR}/include
)
target_compile_definitions(usermod_cml INTERFACE CML_STATIC_DEFINE)
target_link_libraries(usermod_cml INTERFACE
    "${CML_STATIC}"
    m dl pthread
)
add_dependencies(usermod_cml "${CML_STATIC}")
target_link_libraries(usermod INTERFACE usermod_cml)
