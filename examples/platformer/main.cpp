#include "graphics.h"
#include "gtypes.h"
#include "twodpass.h"
#include "window.h"
#include <iostream>
#include <passes/gui_pass.h>

struct Movement
{
    unsigned int up : 1;
    unsigned int down : 1;
    unsigned int left : 1;
    unsigned int right : 1;
};

int main()
{
    Window w{};
    Graphics g{&w};

    Sprite mainChar{0,
                    0,
                    64,
                    64,
                    6,
                    1,
                    0,
                    static_cast<float>(g.getSwapchainSize().width),
                    static_cast<float>(g.getSwapchainSize().height)};
    Movement m{};

    w.registerKey([&mainChar, &m](int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_D) {
            if (action == GLFW_PRESS)
                m.right = 0x1;
            else if (action == GLFW_RELEASE)
                m.right = 0x0;
        }

        if (key == GLFW_KEY_A) {
            if (action == GLFW_PRESS)
                m.left = 0x1;
            else if (action == GLFW_RELEASE)
                m.left = 0x0;
        }

        if (key == GLFW_KEY_S) {
            if (action == GLFW_PRESS)
                m.down = 0x1;
            else if (action == GLFW_RELEASE)
                m.down = 0x0;
        }

        if (key == GLFW_KEY_W) {
            if (action == GLFW_PRESS)
                m.up = 0x1;
            else if (action == GLFW_RELEASE)
                m.up = 0x0;
        }
    });

    GuiPass gPass{&g};
    TwoDPass tdPass{&g};

    tdPass.setSprite(&mainChar);

    std::vector<Pass *> gPasses{&tdPass, &gPass};
    std::vector<Pass *> cPasses{};

    g.beginRenderLoop(gPasses, cPasses, [&tdPass, &mainChar, &m](double t) {
        static int d = 0;
        static float initialVel = 0;
        static double jumpTimeStart = t;

        int c = t - d;

        if (c >= 50) {
            d = t;

            mainChar.incrementIdleIndex();
        }

        float v = initialVel + (-9.81 * 0.005) * (t - jumpTimeStart);
        std::cout << v << std::endl;
        mainChar.y += v;

        if (mainChar.y < 0) {
            mainChar.y = 0;
        }

        if (m.right)
            mainChar.x += 1;
        else if (m.left)
            mainChar.x -= 1;

        if (m.up) {
            initialVel = 8;
            jumpTimeStart = t;
        } else if (m.down)
            mainChar.y -= 1;
    });
}