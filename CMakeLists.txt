cmake_minimum_required(VERSION 3.16)

include(FetchContent)
project(Orbital LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# ===================RETRIEVE LIBRARIES==================

FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG main
)
FetchContent_MakeAvailable(fastgltf)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
)

FetchContent_MakeAvailable(imgui)

# ======================SET UP IMGUI=====================

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan glfw)

# ========================================================

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

target_link_libraries(Orbital PUBLIC Vulkan::Vulkan glfw fastgltf::fastgltf imgui)

add_subdirectory(examples)

install(TARGETS Orbital
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
