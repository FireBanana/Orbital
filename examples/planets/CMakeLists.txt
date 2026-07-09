add_executable(planets main.cpp n_body_compute.h n_body_compute.cpp)

target_link_libraries(planets PRIVATE Orbital)
