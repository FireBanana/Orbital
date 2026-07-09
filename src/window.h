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

private:
    WindowState m_state{};
};

#endif // WINDOW_H
