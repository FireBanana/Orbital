#ifndef WINDOW_H
#define WINDOW_H

#include "global.h"

struct WindowState
{
    bool newClick;
    bool isPressed;
    double xDelta = 0.0;
    double yDelta = 0.0;
    double cameraDistance = Global::g_camera_position.z;
};

class Window
{
public:
    Window();

    void makeSurface();
    VkExtent2D getExtent() const;

private:
    WindowState m_state{};
    VkExtent2D m_extent{Global::g_width, Global::g_height};
};

#endif // WINDOW_H
