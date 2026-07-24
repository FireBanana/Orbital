add_executable(platformer shaders/sprite.vert.slang shaders/sprite.frag.slang twodpass.h twodpass.cpp gtypes.h main.cpp)

target_link_libraries(platformer PRIVATE Orbital)

orbital_add_shaders(platformer ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    sprite.vert.slang sprite.frag.slang)
