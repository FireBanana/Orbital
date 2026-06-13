cmake_minimum_required(VERSION 3.16)

include(FetchContent)

project(Orbital LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG main
)
FetchContent_MakeAvailable(fastgltf)

add_executable(Orbital src/main.cpp src/mesh_loader.h src/mesh_loader.cpp shaders/triangle.vert.slang shaders/triangle.frag.slang
    shaders/line.vert.slang shaders/line.frag.slang
    src/graphics.h src/graphics.cpp external/stb_image.h
    src/types.h
    src/global.h

    src/passes/pass.h
    src/passes/forward_pass.h src/passes/forward_pass.cpp
    src/passes/gui_pass.h src/passes/gui_pass.cpp
    src/cuda.cu
    src/cuda.h
)

set_target_properties( Orbital
                       PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

set_source_files_properties(src/cuda.cu PROPERTIES LANGUAGE CUDA)

include(GNUInstallDirs)

target_compile_definitions(Orbital PRIVATE ROOT="${CMAKE_SOURCE_DIR}/")

#target_include_directories(Orbital PRIVATE ${fastgltf_SOURCE_DIR})
target_include_directories(Orbital PRIVATE "external/")

target_link_libraries(Orbital PRIVATE Vulkan::Vulkan glfw fastgltf::fastgltf)

install(TARGETS Orbital
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
