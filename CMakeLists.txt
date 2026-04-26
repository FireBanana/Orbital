cmake_minimum_required(VERSION 3.16)

project(Orbital LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_executable(Orbital main.cpp shaders/triangle.vert.slang shaders/triangle.frag.slang)

include(GNUInstallDirs)

target_link_libraries(Orbital PRIVATE Vulkan::Vulkan glfw)

install(TARGETS Orbital
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
