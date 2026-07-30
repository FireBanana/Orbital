#include "window.h"
#include "global.h"
#include <iostream>

Window::Window()
{
    if (!glfwInit())
        std::cout << "Issues!";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);

    Global::g_window = glfwCreateWindow(m_state.extent.width,
                                        m_state.extent.height,
                                        "Orbital",
                                        NULL,
                                        NULL);

    if (!Global::g_window)
        throw;

    glfwShowWindow(Global::g_window);

    glfwSetWindowUserPointer(Global::g_window, &m_state);

    // glfwSetMouseButtonCallback(Global::g_window,
    //                            [](GLFWwindow *window, int button, int action, int mods) {
    //                                auto *state = static_cast<WindowState *>(
    //                                    glfwGetWindowUserPointer(window));

    //                                if (button == GLFW_MOUSE_BUTTON_LEFT) {
    //                                    if (action == GLFW_PRESS) {
    //                                        state->isPressed = true;
    //                                        state->newClick = true;
    //                                    } else if (action == GLFW_RELEASE) {
    //                                        state->isPressed = false;
    //                                    }
    //                                }
    //                            });

    // glfwSetCursorPosCallback(Global::g_window, [](GLFWwindow *window, double xpos, double ypos) {
    //     static double lastXPosition = xpos, lastYPosition = ypos;

    //     auto *state = static_cast<WindowState *>(glfwGetWindowUserPointer(window));

    //     if (!state->isPressed)
    //         return;

    //     if (state->newClick) {
    //         lastXPosition = xpos;
    //         lastYPosition = ypos;
    //         state->newClick = false;
    //     }

    //     state->xDelta -= xpos - lastXPosition;
    //     state->yDelta += ypos - lastYPosition;
    //     state->yDelta = glm::clamp(state->yDelta, -130.0 + 0.01, 130.0 - 0.01);

    //     Global::g_camera_position
    //         = glm::vec3(state->cameraDistance
    //                         * (glm::sin(state->xDelta * 0.01) * glm::cos(state->yDelta * 0.01)),
    //                     state->cameraDistance * (glm::sin(state->yDelta * 0.01)),
    //                     state->cameraDistance
    //                         * (glm::cos(state->xDelta * 0.01) * glm::cos(state->yDelta * 0.01)));

    //     lastXPosition = xpos;
    //     lastYPosition = ypos;
    // });

    // glfwSetScrollCallback(Global::g_window, [](GLFWwindow *window, double xoffset, double yoffset) {
    //     auto *state = static_cast<WindowState *>(glfwGetWindowUserPointer(window));

    //     state->cameraDistance -= yoffset * 0.2;

    //     Global::g_camera_position
    //         = glm::vec3(state->cameraDistance
    //                         * (glm::sin(state->xDelta * 0.01) * glm::cos(state->yDelta * 0.01)),
    //                     state->cameraDistance * (glm::sin(state->yDelta * 0.01)),
    //                     state->cameraDistance
    //                         * (glm::cos(state->xDelta * 0.01) * glm::cos(state->yDelta * 0.01)));
    // });

    glfwSetFramebufferSizeCallback(Global::g_window, [](GLFWwindow *window, int width, int height) {
        auto *state = static_cast<WindowContext *>(glfwGetWindowUserPointer(window));

        Global::g_swapchain_dirty = true;
        state->extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        Global::g_width = width;
        Global::g_height = height;
    });
}

void Window::makeSurface()
{
    if (auto res = glfwCreateWindowSurface(Global::g_instance,
                                           Global::g_window,
                                           nullptr,
                                           &Global::g_surface);
        res == VK_SUCCESS)
        std::cout << "Surface creation good" << std::endl;
    else
        std::cout << "Surface creation failed " << res << std::endl;
}

VkExtent2D Window::getExtent() const
{
    return m_state.extent;
}

void Window::registerKey(std::function<void(int key, int scancode, int action, int mods)> cb)
{
    m_state.keyCallback = cb;

    glfwSetKeyCallback(Global::g_window,
                       [](GLFWwindow *window, int key, int scancode, int action, int mods) {
                           auto *state = static_cast<WindowContext *>(
                               glfwGetWindowUserPointer(window));
                           state->keyCallback(key, scancode, action, mods);
                       });
}

void Window::registerMouseButton(std::function<void(int button, int action, int mod)> cb)
{
    m_state.mouseButtonCallback = cb;

    glfwSetMouseButtonCallback(Global::g_window,
                               [](GLFWwindow *window, int button, int action, int mods) {
                                   auto *state = static_cast<WindowContext *>(
                                       glfwGetWindowUserPointer(window));
                                   state->mouseButtonCallback(button, action, mods);
                               });
}

void Window::registerMousePosition(std::function<void(double, double)> cb)
{
    m_state.mousePositionCallback = cb;

    glfwSetCursorPosCallback(Global::g_window, [](GLFWwindow *window, double xpos, double ypos) {
        auto *state = static_cast<WindowContext *>(glfwGetWindowUserPointer(window));
        state->mousePositionCallback(xpos, ypos);
    });
}

void Window::registerMouseScroll(std::function<void(double, double)> cb)
{
    m_state.mouseScrollCallback = cb;

    glfwSetScrollCallback(Global::g_window, [](GLFWwindow *window, double xoffset, double yoffset) {
        auto *state = static_cast<WindowContext *>(glfwGetWindowUserPointer(window));
        state->mouseScrollCallback(xoffset, yoffset);
    });
}
