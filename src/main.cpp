#include "global.h"
#include "graphics.h"
#include "pipelines/forward_pipeline.h"
#include "pipelines/ui_pipeline.h"
#include <iostream>
#include <random>

int main()
{
    if (!glfwInit())
        std::cout << "Issues!";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);

    Global::g_window = glfwCreateWindow(Global::WIDTH, Global::HEIGHT, "Orbital", NULL, NULL);

    if (!Global::g_window)
        throw;

    make_instance();
    make_device();

    if (auto res = glfwCreateWindowSurface(Global::g_instance,
                                           Global::g_window,
                                           nullptr,
                                           &Global::g_surface);
        res == VK_SUCCESS)
        std::cout << "Surface creation good" << std::endl;
    else
        std::cout << "Surface creation failed " << res << std::endl;

    make_swapchain();

    Global::g_depth = make_image({Global::WIDTH,
                                  Global::HEIGHT,
                                  Global::DEPTH_FORMAT,
                                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                  VK_IMAGE_ASPECT_DEPTH_BIT});

    //ui_pipeline::create_pipeline();

    glfwMakeContextCurrent(Global::g_window);

    glfwShowWindow(Global::g_window);

    struct WindowState
    {
        bool new_click;
        bool is_pressed;
        double x_delta = 0.0;
        double y_delta = 0.0;
        double camera_distance = Global::g_camera_position.z;
    } state;

    glfwSetWindowUserPointer(Global::g_window, &state);

    glfwSetMouseButtonCallback(Global::g_window,
                               [](GLFWwindow *window, int button, int action, int mods) {
                                   auto *state = static_cast<WindowState *>(
                                       glfwGetWindowUserPointer(window));

                                   if (button == GLFW_MOUSE_BUTTON_LEFT) {
                                       if (action == GLFW_PRESS) {
                                           state->is_pressed = true;
                                           state->new_click = true;
                                       } else if (action == GLFW_RELEASE) {
                                           state->is_pressed = false;
                                       }
                                   }
                               });

    glfwSetCursorPosCallback(Global::g_window, [](GLFWwindow *window, double xpos, double ypos) {
        static double last_x_position = xpos, last_y_position = ypos;

        auto *state = static_cast<WindowState *>(glfwGetWindowUserPointer(window));

        if (!state->is_pressed)
            return;

        if (state->new_click) {
            last_x_position = xpos;
            last_y_position = ypos;
            state->new_click = false;
        }

        state->x_delta -= xpos - last_x_position;
        state->y_delta += ypos - last_y_position;
        state->y_delta = glm::clamp(state->y_delta, -130.0 + 0.01, 130.0 - 0.01);

        Global::g_camera_position
            = glm::vec3(state->camera_distance
                            * (glm::sin(state->x_delta * 0.01) * glm::cos(state->y_delta * 0.01)),
                        state->camera_distance * (glm::sin(state->y_delta * 0.01)),
                        state->camera_distance
                            * (glm::cos(state->x_delta * 0.01) * glm::cos(state->y_delta * 0.01)));

        last_x_position = xpos;
        last_y_position = ypos;
    });

    glfwSetScrollCallback(Global::g_window, [](GLFWwindow *window, double xoffset, double yoffset) {
        auto *state = static_cast<WindowState *>(glfwGetWindowUserPointer(window));

        state->camera_distance -= yoffset * 0.2;

        Global::g_camera_position
            = glm::vec3(state->camera_distance
                            * (glm::sin(state->x_delta * 0.01) * glm::cos(state->y_delta * 0.01)),
                        state->camera_distance * (glm::sin(state->y_delta * 0.01)),
                        state->camera_distance
                            * (glm::cos(state->x_delta * 0.01) * glm::cos(state->y_delta * 0.01)));
    });

    forward_pipeline::create_sampler();
    forward_pipeline::create_descriptor();
    forward_pipeline::create_pipeline();

    // ===== Load assets ======

    auto earth_asset = MeshLoader::load_model(ROOT "assets/earth.glb");
    auto monkey_asset = MeshLoader::load_model(ROOT "assets/monkey.glb");

    auto monkey = make_native_model(monkey_asset);
    auto earth = make_native_model(earth_asset);

    std::vector<NativeModel> renderables{50, earth};

    // ========================

    begin_render_loop(renderables);

    constexpr double min_distance = 3.0, max_distance = 10.0;
    constexpr double min_speed = 0.0001, max_speed = 0.000001;
    std::mt19937 mt{};
    std::uniform_real_distribution<double> distance_distribution{min_distance, max_distance};
    std::uniform_real_distribution<double> speed_distribution{min_speed, max_speed};

    struct GameObjectPlanet
    {
        double distance;
        double speed;
    };

    std::vector<GameObjectPlanet> game_objects{renderables.size()};

    double total_distance = 0.0;

    for (auto i = 1; i < game_objects.size(); ++i) {
        game_objects[i].distance = total_distance + distance_distribution(mt);
        game_objects[i].speed = speed_distribution(mt);

        total_distance = game_objects[i].distance;
    }

    auto get_time = []() {};

    uint64_t frame = 0;

    double t = 0.0;
    double dt = 1 / 60.0;
    auto curr_time = std::chrono::high_resolution_clock::now();

    // Main loop
    while (1) {
        if (!Global::g_window_running)
            break;

        auto new_time = std::chrono::high_resolution_clock::now();
        auto frame_time = std::chrono::duration<double, std::milli>(new_time - curr_time).count();
        curr_time = new_time;

        while (frame_time > 0.0) {
            float delta = std::min(frame_time, dt);

            for (auto i = 1; i < renderables.size(); ++i) {
                renderables[i].position
                    = {static_cast<float>(glm::sin(game_objects[i].speed * dt * frame)
                                          * game_objects[i].distance),
                       0.0,
                       static_cast<float>(glm::cos(game_objects[i].speed * dt * frame)
                                          * game_objects[i].distance)};
            }

            frame_time -= delta;
            t += delta;
        }

        frame++;
    }

    Global::g_gui_thread.join();

    return 0;
}