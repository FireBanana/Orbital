#ifndef WINDOW_H
#define WINDOW_H

#include "global.h"

// struct WindowState
// {
//     bool newClick;
//     bool isPressed;
//     double xDelta = 0.0;
//     double yDelta = 0.0;
//     double cameraDistance = Global::g_camera_position.z;
//     VkExtent2D extent{Global::g_width, Global::g_height};
// };

struct WindowContext
{
    VkExtent2D extent{Global::g_width, Global::g_height};
    std::function<void(int button, int action, int mod)> mouseButtonCallback;
    std::function<void(double x, double y)> mousePositionCallback;
    std::function<void(double xoffset, double yoffset)> mouseScrollCallback;
    std::function<void(int key, int scancode, int action, int mods)> keyCallback;
};

class Window
{
public:
    Window();

    void makeSurface();
    VkExtent2D getExtent() const;

    void registerKey(std::function<void(int key, int scancode, int action, int mods)> cb);
    void registerMouseButton(std::function<void(int button, int action, int mod)> cb);
    void registerMousePosition(std::function<void(double x, double y)> cb);
    void registerMouseScroll(std::function<void(double xoffset, double yoffset)> cb);

private:
    WindowContext m_state{};
};

#endif // WINDOW_H
