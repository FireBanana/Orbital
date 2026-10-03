#include "window.h"
#include <iostream>
#include <passes/forward_pass.h>
#include <passes/gui_pass.h>
#include <passes/shadow_pass.h>

class CharacterController
{
public:
    CharacterController(ForwardPass *p)
        : pass{p} {};

    void move(glm::vec3 translation) { pass->translateCamera(translation); }
    void rotate(float yaw, float pitch) { pass->rotateCamera(yaw, pitch); }

private:
    ForwardPass *pass;
    float m_cameraYaw, m_cameraPitch;
};

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

    sPass->attachModels(&models);
    fPass->attachModels(&models);
    fPass->setShadowMap(shadowMap);

    CharacterController controller{fPass};
    bool isPressed;
    bool newClick;
    double xDelta = 0, yDelta = 0;
    double cameraDistance = 3;

    glfwSetInputMode(Global::g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
        [&isPressed, &newClick, &xDelta, &yDelta, &cameraDistance, &fPass, &controller](double xp,
                                                                                        double yp) {
            xp *= 1.0;
            yp *= -1.0;
            static double lastXPosition = xp, lastYPosition = yp;

            // if (!isPressed)
            //     return;

            // if (newClick) {
            //     lastXPosition = xp;
            //     lastYPosition = yp;
            //     newClick = false;
            // }

            xDelta = xp - lastXPosition;
            yDelta = yp - lastYPosition;

            // fPass->setCameraPosition(
            //     glm::vec3(cameraDistance * (glm::sin(xDelta * 0.01) * glm::cos(yDelta * 0.01)),
            //               cameraDistance * (glm::sin(yDelta * 0.01)),
            //               cameraDistance * (glm::cos(xDelta * 0.01) * glm::cos(yDelta * 0.01))));

            controller.rotate(xDelta * 0.01, yDelta * 0.01);

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

    std::atomic<bool> move = false;

    w.registerKey([&fPass, &move](int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_W) {
            if (action == GLFW_PRESS) {
                move = true;
            } else if (action == GLFW_RELEASE) {
                move = false;
            }
        } else if (key == GLFW_KEY_ESCAPE)
            glfwSetInputMode(Global::g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    });

    // Call after input is registered because our input wipes dear imgui input
    passes.push_back(new GuiPass(&g));
    auto gPass = static_cast<GuiPass *>(passes[2]);
    gPass->addSlider("roughness", 0., 1., [&models](float roughness) {
        for (auto &m : models)
            m.roughness = roughness;
    });

    g.beginRenderLoop(passes,
                      cPasses,
                      [&sPass, &fPass, &move, &controller](double time, double deltaTime) {
                          glm::vec3 lp(glm::sin(time * 0.0001), 0.3, glm::cos(time * 0.0001));

                          if (move) {
                              controller.move({0, 0, 0.001 * deltaTime});
                          }

                          sPass->setLightDirection(lp);
                          fPass->setLightVPMatrix(sPass->getLightVPMatrix());
                          fPass->setLightDirection(lp);
                      });
}
