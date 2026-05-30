#include "global.h"
#include "graphics.h"
#include <iostream>

int main()
{
    if (!glfwInit())
        std::cout << "Issues!";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);

    auto *w = glfwCreateWindow(Global::WIDTH, Global::HEIGHT, "Orbital", NULL, NULL);

    if (!w)
        throw;

    make_instance();
    make_device();

    if (auto res = glfwCreateWindowSurface(Global::g_instance, w, nullptr, &Global::g_surface);
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

    make_sampler();
    make_descriptor();
    make_pipeline();

    glfwMakeContextCurrent(w);

    glfwShowWindow(w);

    struct WindowState
    {
        bool new_click;
        bool is_pressed;
        double x_delta = 0.0;
        double y_delta = 0.0;
        double camera_distance = Global::g_camera_position.z;
    } state;

    glfwSetWindowUserPointer(w, &state);

    glfwSetMouseButtonCallback(w, [](GLFWwindow *window, int button, int action, int mods) {
        auto *state = static_cast<WindowState *>(glfwGetWindowUserPointer(window));

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                state->is_pressed = true;
                state->new_click = true;
            } else if (action == GLFW_RELEASE) {
                state->is_pressed = false;
            }
        }
    });

    glfwSetCursorPosCallback(w, [](GLFWwindow *window, double xpos, double ypos) {
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

    glfwSetScrollCallback(w, [](GLFWwindow *window, double xoffset, double yoffset) {
        auto *state = static_cast<WindowState *>(glfwGetWindowUserPointer(window));

        state->camera_distance -= yoffset * 0.2;

        Global::g_camera_position
            = glm::vec3(state->camera_distance
                            * (glm::sin(state->x_delta * 0.01) * glm::cos(state->y_delta * 0.01)),
                        state->camera_distance * (glm::sin(state->y_delta * 0.01)),
                        state->camera_distance
                            * (glm::cos(state->x_delta * 0.01) * glm::cos(state->y_delta * 0.01)));
    });

    // ===== Load assets ======

    auto earth_asset = MeshLoader::load_model(ROOT "assets/earth.glb");
    auto monkey_asset = MeshLoader::load_model(ROOT "assets/monkey.glb");

    auto monkey = make_native_model(monkey_asset);
    auto earth = make_native_model(earth_asset);

    std::vector<NativeModel> renderables{100, monkey};

    float x = 0.0f;
    for (auto &m : renderables)
        m.position.x = x++ * 3;

    // ========================

    while (!glfwWindowShouldClose(w)) {
        uint32_t frame;
        auto r = acquire_swapchain_image(&frame);

        if (r != VK_SUCCESS) {
            vkQueueWaitIdle(Global::g_queue);
            continue;
        }

        render(frame, renderables);
        present_image(frame);

        glfwSwapBuffers(w);
        glfwPollEvents();
    }

    return 0;
}