#include "window.h"
#include <passes/forward_pass.h>

int main()
{
    Window w{};
    Graphics g{&w};

    std::vector<Pass *> passes{new ForwardPass{&g}};
    std::vector<Pass *> cPasses{};

    auto map = AssetLoader::loadScene(ROOT "examples/fps/assets/testscene.glb");
    auto nmap = g.makeNativeModel(map);

    std::vector<NativeModel> models{nmap};

    passes[0]->attachModels(&models);

    bool isPressed;
    bool newClick;
    double xDelta = 0, yDelta = 0;
    double cameraDistance = 3;

    auto fPass = static_cast<ForwardPass *>(passes[0]);

    w.registerMouseButton([&isPressed, &newClick](int button, int action, int mod) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                isPressed = true;
                newClick = true;
            } else if (action == GLFW_RELEASE) {
                isPressed = false;
            }
        }
    });

    w.registerMousePosition(
        [&isPressed, &newClick, &xDelta, &yDelta, &cameraDistance, &fPass](double xp, double yp) {
            static double lastXPosition = xp, lastYPosition = yp;

            if (!isPressed)
                return;

            if (newClick) {
                lastXPosition = xp;
                lastYPosition = yp;
                newClick = false;
            }

            xDelta -= xp - lastXPosition;
            yDelta += yp - lastYPosition;
            yDelta = glm::clamp(yDelta, -130.0 + 0.01, 130.0 - 0.01);

            fPass->setCameraPosition(
                glm::vec3(cameraDistance * (glm::sin(xDelta * 0.01) * glm::cos(yDelta * 0.01)),
                          cameraDistance * (glm::sin(yDelta * 0.01)),
                          cameraDistance * (glm::cos(xDelta * 0.01) * glm::cos(yDelta * 0.01))));

            lastXPosition = xp;
            lastYPosition = yp;
        });

    w.registerMouseScroll(
        [&cameraDistance, &xDelta, &yDelta, &fPass](double xoffset, double yoffset) {
            cameraDistance -= yoffset * 0.2;

            fPass->setCameraPosition(
                glm::vec3(cameraDistance * (glm::sin(xDelta * 0.01) * glm::cos(yDelta * 0.01)),
                          cameraDistance * (glm::sin(yDelta * 0.01)),
                          cameraDistance * (glm::cos(xDelta * 0.01) * glm::cos(yDelta * 0.01))));
        });

    g.beginRenderLoop(passes, cPasses, [](double, double) {});
}
