#include "window.h"
#include "global.h"
#include "imgui.h"
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
                           if (ImGui::GetIO().WantCaptureKeyboard)
                               return;

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
                                   if (ImGui::GetIO().WantCaptureMouse)
                                       return;

                                   auto *state = static_cast<WindowContext *>(
                                       glfwGetWindowUserPointer(window));
                                   state->mouseButtonCallback(button, action, mods);
                               });
}

void Window::registerMousePosition(std::function<void(double, double)> cb)
{
    m_state.mousePositionCallback = cb;

    glfwSetCursorPosCallback(Global::g_window, [](GLFWwindow *window, double xpos, double ypos) {
        if (ImGui::GetIO().WantCaptureMouse)
            return;

        auto *state = static_cast<WindowContext *>(glfwGetWindowUserPointer(window));
        state->mousePositionCallback(xpos, ypos);
    });
}

void Window::registerMouseScroll(std::function<void(double, double)> cb)
{
    m_state.mouseScrollCallback = cb;

    glfwSetScrollCallback(Global::g_window, [](GLFWwindow *window, double xoffset, double yoffset) {
        if (ImGui::GetIO().WantCaptureMouse)
            return;

        auto *state = static_cast<WindowContext *>(glfwGetWindowUserPointer(window));
        state->mouseScrollCallback(xoffset, yoffset);
    });
}
