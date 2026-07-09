cmake_minimum_required(VERSION 3.16)

include(FetchContent)

# set(NVIDIA_GPU ON)

# if(NVIDIA_GPU)
#     project(Orbital LANGUAGES CXX CUDA)
# else()
#     project(Orbital LANGUAGES CXX HIP)
# endif()
project(Orbital LANGUAGES CXX)

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

set(SOURCES
        src/main.cpp src/mesh_loader.h src/mesh_loader.cpp shaders/triangle.vert.slang shaders/triangle.frag.slang
        shaders/line.vert.slang shaders/line.frag.slang
        shaders/gravity.comp.slang
        src/graphics.h src/graphics.cpp external/stb_image.h
        src/types.h
        src/global.h
        src/window.h src/window.cpp
        src/passes/pass.h
        src/passes/forward_pass.h src/passes/forward_pass.cpp
        src/passes/gui_pass.h src/passes/gui_pass.cpp)

add_library(Orbital ${SOURCES})

include(GNUInstallDirs)

target_compile_definitions(Orbital PUBLIC ROOT="${CMAKE_SOURCE_DIR}/")

#target_include_directories(Orbital PRIVATE ${fastgltf_SOURCE_DIR})
target_include_directories(Orbital PUBLIC "src/" "external/")

target_link_libraries(Orbital PUBLIC Vulkan::Vulkan glfw fastgltf::fastgltf)

add_subdirectory(examples)

install(TARGETS Orbital
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
