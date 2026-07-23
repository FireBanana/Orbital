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

# =====================SHADER COMPILATION=================

find_program(SLANGC_EXECUTABLE slangc REQUIRED)

# Compiles the named *.slang shaders (relative to SHADER_DIR) to SPIR-V in
# place (next to the source), and makes TARGET depend on the compiled output.
# Shaders are loaded at runtime via ROOT + a source-relative path, so outputs
# stay in the source tree rather than the build tree.
#
# Usage: orbital_add_shaders(<target> <shader_dir> shader1.slang shader2.slang ...)
function(orbital_add_shaders TARGET SHADER_DIR)
    set(SHADER_OUTPUTS)
    foreach(SHADER_NAME ${ARGN})
        set(SHADER_SOURCE "${SHADER_DIR}/${SHADER_NAME}")
        string(REGEX REPLACE "\\.slang$" ".spv" SPV_NAME "${SHADER_NAME}")
        set(SPV_OUTPUT "${SHADER_DIR}/${SPV_NAME}")

        add_custom_command(
            OUTPUT "${SPV_OUTPUT}"
            COMMAND "${SLANGC_EXECUTABLE}" "${SHADER_SOURCE}" -o "${SPV_OUTPUT}"
            DEPENDS "${SHADER_SOURCE}"
            WORKING_DIRECTORY "${SHADER_DIR}"
            COMMENT "Compiling shader ${SHADER_NAME}"
            VERBATIM
        )

        list(APPEND SHADER_OUTPUTS "${SPV_OUTPUT}")
    endforeach()

    if(SHADER_OUTPUTS)
        add_custom_target(${TARGET}_shaders DEPENDS ${SHADER_OUTPUTS})
        add_dependencies(${TARGET} ${TARGET}_shaders)
    endif()
endfunction()

# ========================================================

set(SOURCES
        src/main.cpp src/asset_loader.h src/asset_loader.cpp shaders/triangle.vert.slang shaders/triangle.frag.slang
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

orbital_add_shaders(Orbital ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    triangle.vert.slang triangle.frag.slang
    line.vert.slang line.frag.slang
    gravity.comp.slang)

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
