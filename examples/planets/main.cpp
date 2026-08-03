#include "graphics.h"
#include "n_body_compute.h"
#include "passes/forward_pass.h"
#include "passes/gui_pass.h"
#include "window.h"
#include <random>

int main()
{
    Window w{};
    Graphics graphics{&w};

    // ===== Setup input ======

    bool isPressed;
    bool newClick;
    double xDelta = 0, yDelta = 0;
    double cameraDistance = 3;

    ForwardPass fPass{&graphics};

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

            fPass.setCameraPosition(
                glm::vec3(cameraDistance * (glm::sin(xDelta * 0.01) * glm::cos(yDelta * 0.01)),
                          cameraDistance * (glm::sin(yDelta * 0.01)),
                          cameraDistance * (glm::cos(xDelta * 0.01) * glm::cos(yDelta * 0.01))));

            lastXPosition = xp;
            lastYPosition = yp;
        });

    w.registerMouseScroll(
        [&cameraDistance, &xDelta, &yDelta, &fPass](double xoffset, double yoffset) {
            cameraDistance -= yoffset * 0.2;

            fPass.setCameraPosition(
                glm::vec3(cameraDistance * (glm::sin(xDelta * 0.01) * glm::cos(yDelta * 0.01)),
                          cameraDistance * (glm::sin(yDelta * 0.01)),
                          cameraDistance * (glm::cos(xDelta * 0.01) * glm::cos(yDelta * 0.01))));
        });

    // ===== Make Psses =======

    GuiPass gPass{&graphics};
    GravityComputePass gcPass{&graphics};

    // ===== Load assets ======

    auto earthAsset = AssetLoader::loadModel(ROOT "assets/earth.glb");
    auto monkeyAsset = AssetLoader::loadModel(ROOT "assets/earth.glb");

    auto monkey = graphics.makeNativeModel(monkeyAsset);
    auto earth = graphics.makeNativeModel(earthAsset);

    std::vector<NativeModel> renderables{50};

    for (int i = 0; i < 50; ++i) {
        if (i % 2 == 0)
            renderables[i] = earth[i % earth.size()];
        else
            renderables[i] = monkey[i % monkey.size()];
    }

    fPass.attachModels(&renderables);

    // ========================
    std::vector<Pass *> passes = {&fPass, &gPass};
    std::vector<Pass *> cPasses = {&gcPass};

    constexpr double minDistance = 3.0, maxDistance = 10.0;
    constexpr double minSpeed = 0.000001, maxSpeed = 0.0001;
    std::mt19937 mt{};
    std::uniform_real_distribution<double> distanceDistribution{minDistance, maxDistance};
    std::uniform_real_distribution<double> speedDistribution{minSpeed, maxSpeed};

    struct GameObjectPlanet
    {
        double distance;
        double speed;
    };

    std::vector<GameObjectPlanet> gameObjects{renderables.size()};

    double totalDistance = 0.0;

    for (auto i = 1; i < gameObjects.size(); ++i) {
        gameObjects[i].distance = totalDistance + distanceDistribution(mt);
        gameObjects[i].speed = speedDistribution(mt);

        totalDistance = gameObjects[i].distance;
    }

    auto update = [&renderables, &gameObjects](double t, double dt) {
        for (auto i = 1; i < renderables.size(); ++i) {
            renderables[i].position = {static_cast<float>(glm::sin(gameObjects[i].speed * t)
                                                          * gameObjects[i].distance),
                                       0.0,
                                       static_cast<float>(glm::cos(gameObjects[i].speed * t)
                                                          * gameObjects[i].distance)};
        }
    };

    graphics.beginRenderLoop(passes, cPasses, update);

    return 0;
}
