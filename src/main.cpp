#include "cuda.h"
#include "global.h"
#include "graphics.h"
#include "passes/forward_pass.h"
#include "passes/gui_pass.h"
#include "passes/n_body_compute.h"
#include <iostream>
#include <random>

int main()
{
    if (!glfwInit())
        std::cout << "Issues!";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);

    Global::g_window = glfwCreateWindow(Global::WIDTH, Global::HEIGHT, "Orbital", NULL, NULL);

    if (!Global::g_window)
        throw;

    makeInstance();
    makeDevice();

    if (auto res = glfwCreateWindowSurface(Global::g_instance,
                                           Global::g_window,
                                           nullptr,
                                           &Global::g_surface);
        res == VK_SUCCESS)
        std::cout << "Surface creation good" << std::endl;
    else
        std::cout << "Surface creation failed " << res << std::endl;

    makeSwapchain();
    makeRenderTarget();

    Global::g_depth = makeImage({Global::WIDTH,
                                  Global::HEIGHT,
                                  Global::DEPTH_FORMAT,
                                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                  VK_IMAGE_ASPECT_DEPTH_BIT});

    //ui_pipeline::createPipeline();

    glfwMakeContextCurrent(Global::g_window);

    glfwShowWindow(Global::g_window);

    struct WindowState
    {
        bool newClick;
        bool isPressed;
        double xDelta = 0.0;
        double yDelta = 0.0;
        double cameraDistance = Global::g_camera_position.z;
    } state;

    glfwSetWindowUserPointer(Global::g_window, &state);

    glfwSetMouseButtonCallback(Global::g_window,
                               [](GLFWwindow *window, int button, int action, int mods) {
                                   auto *state = static_cast<WindowState *>(
                                       glfwGetWindowUserPointer(window));

                                   if (button == GLFW_MOUSE_BUTTON_LEFT) {
                                       if (action == GLFW_PRESS) {
                                           state->isPressed = true;
                                           state->newClick = true;
                                       } else if (action == GLFW_RELEASE) {
                                           state->isPressed = false;
                                       }
                                   }
                               });

    glfwSetCursorPosCallback(Global::g_window, [](GLFWwindow *window, double xpos, double ypos) {
        static double lastXPosition = xpos, lastYPosition = ypos;

        auto *state = static_cast<WindowState *>(glfwGetWindowUserPointer(window));

        if (!state->isPressed)
            return;

        if (state->newClick) {
            lastXPosition = xpos;
            lastYPosition = ypos;
            state->newClick = false;
        }

        state->xDelta -= xpos - lastXPosition;
        state->yDelta += ypos - lastYPosition;
        state->yDelta = glm::clamp(state->yDelta, -130.0 + 0.01, 130.0 - 0.01);

        Global::g_camera_position
            = glm::vec3(state->cameraDistance
                            * (glm::sin(state->xDelta * 0.01) * glm::cos(state->yDelta * 0.01)),
                        state->cameraDistance * (glm::sin(state->yDelta * 0.01)),
                        state->cameraDistance
                            * (glm::cos(state->xDelta * 0.01) * glm::cos(state->yDelta * 0.01)));

        lastXPosition = xpos;
        lastYPosition = ypos;
    });

    glfwSetScrollCallback(Global::g_window, [](GLFWwindow *window, double xoffset, double yoffset) {
        auto *state = static_cast<WindowState *>(glfwGetWindowUserPointer(window));

        state->cameraDistance -= yoffset * 0.2;

        Global::g_camera_position
            = glm::vec3(state->cameraDistance
                            * (glm::sin(state->xDelta * 0.01) * glm::cos(state->yDelta * 0.01)),
                        state->cameraDistance * (glm::sin(state->yDelta * 0.01)),
                        state->cameraDistance
                            * (glm::cos(state->xDelta * 0.01) * glm::cos(state->yDelta * 0.01)));
    });

    ForwardPass fPass{};
    GuiPass gPass{};
    GravityComputePass gcPass{};

    // ===== Load assets ======

    auto earthAsset = MeshLoader::loadModel(ROOT "assets/earth.glb");
    auto monkeyAsset = MeshLoader::loadModel(ROOT "assets/monkey.glb");

    auto monkey = makeNativeModel(monkeyAsset);
    auto earth = makeNativeModel(earthAsset);

    std::vector<NativeModel> renderables{50};

    for (int i = 0; i < 50; ++i) {
        if (i % 2 == 0)
            renderables[i] = earth;
        else
            renderables[i] = monkey;
    }

    std::vector<Texture> computeResources{earth.texture};

    fPass.attachModels(&renderables);

    // ========================
    std::vector<Pass *> passes = {&fPass, &gPass};
    std::vector<Pass *> cPasses = {&gcPass};

    beginRenderLoop(passes, cPasses);

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

    auto getTime = []() {};

    uint64_t frame = 0;

    double t = 0.0;
    double dt = 1 / 60.0;
    auto currTime = std::chrono::high_resolution_clock::now();

    // Main loop
    while (1) {
        if (!Global::g_window_running)
            break;

        auto newTime = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::duration<double, std::milli>(newTime - currTime).count();
        currTime = newTime;

        while (frameTime > 0.0) {
            float delta = std::min(frameTime, dt);

            for (auto i = 1; i < renderables.size(); ++i) {
                renderables[i].position = {static_cast<float>(glm::sin(gameObjects[i].speed * t)
                                                              * gameObjects[i].distance),
                                           0.0,
                                           static_cast<float>(glm::cos(gameObjects[i].speed * t)
                                                              * gameObjects[i].distance)};
            }

            frameTime -= delta;
            t += delta;
        }

        frame++;
    }

    Global::g_gui_thread.join();

    return 0;
}
