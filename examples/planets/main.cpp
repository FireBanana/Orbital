#include "graphics.h"
#include "n_body_compute.h"
#include "passes/forward_pass.h"
#include "passes/gui_pass.h"
#include "window.h"
#include <iostream>
#include <random>

int main()
{
    Window w{};
    Graphics graphics{&w};

    ForwardPass fPass{&graphics};
    GuiPass gPass{&graphics};
    GravityComputePass gcPass{&graphics};

    // ===== Load assets ======

    auto earthAsset = MeshLoader::loadModel(ROOT "assets/earth.glb");
    auto monkeyAsset = MeshLoader::loadModel(ROOT "assets/monkey.glb");

    auto monkey = graphics.makeNativeModel(monkeyAsset);
    auto earth = graphics.makeNativeModel(earthAsset);

    std::vector<NativeModel> renderables{50};

    for (int i = 0; i < 50; ++i) {
        if (i % 2 == 0)
            renderables[i] = earth;
        else
            renderables[i] = monkey;
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

    auto update = [&renderables, &gameObjects](double t) {
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
