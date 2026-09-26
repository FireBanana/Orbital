#include "window.h"
#include <passes/forward_pass.h>
#include <passes/gui_pass.h>
#include <passes/shadow_pass.h>

int main()
{
    Window w{};
    Graphics g{&w};

    std::vector<Pass *> passes{new ShadowPass{&g}, new ForwardPass{&g}};
    std::vector<Pass *> cPasses{};

    auto map = AssetLoader::loadScene(ROOT "examples/fps/assets/helmet.glb");
    auto nmap = g.makeNativeModel(map);

    std::vector<NativeModel> models{nmap};

    auto sPass = static_cast<ShadowPass *>(passes[0]);
    auto fPass = static_cast<ForwardPass *>(passes[1]);

    const auto *shadowMap = sPass->getShadowMap();

    //glm::vec3 light{0, 5, 0};

    sPass->attachModels(&models);
    //sPass->setLightPosition(light);
    fPass->attachModels(&models);
    fPass->setShadowMap(shadowMap);
    //fPass->setLightVPMatrix(sPass->getLightVPMatrix());
    //fPass->setLightPosition(light);

    bool isPressed;
    bool newClick;
    double xDelta = 0, yDelta = 0;
    double cameraDistance = 3;

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

    // Call after input is registered because our input wipes dear imgui input
    passes.push_back(new GuiPass(&g));
    auto gPass = static_cast<GuiPass *>(passes[2]);
    gPass->addSlider("roughness", 0., 1., [&models](float roughness) {
        for (auto &m : models)
            m.roughness = roughness;
    });

    g.beginRenderLoop(passes, cPasses, [&sPass, &fPass](double time, double deltaTime) {
        glm::vec3 lp(glm::sin(time * 0.001), glm::cos(time * 0.001), 0.);

        sPass->setLightPosition(lp);
        fPass->setLightVPMatrix(sPass->getLightVPMatrix());
        fPass->setLightPosition(lp);
    });
}
