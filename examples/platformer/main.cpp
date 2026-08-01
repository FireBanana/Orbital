#include "graphics.h"
#include "gtypes.h"
#include "twodpass.h"
#include "window.h"
#include <box2d/box2d.h>
#include <box2d/types.h>
#include <iostream>
#include <passes/gui_pass.h>

int main()
{
    Window w{};
    Graphics g{&w};

    std::vector<Sprite> sprites{};
    std::unordered_map<uint32_t, PhysicsComponent> physicsComponents{};

    auto world = b2DefaultWorldDef();
    world.gravity = {0.0f, -9.81f};
    auto worldId = b2CreateWorld(&world);

    Sprite mainChar{0,
                    64,
                    64,
                    64,
                    6,
                    1,
                    0,
                    static_cast<float>(g.getSwapchainSize().width),
                    static_cast<float>(g.getSwapchainSize().height)};

    auto charBodyDef = b2DefaultBodyDef();
    charBodyDef.type = b2_dynamicBody;
    charBodyDef.fixedRotation = true;
    charBodyDef.position = {(mainChar.rect.x / 64.0f) + (32.0f / 64.0f),
                            (mainChar.rect.y / 64.0f) + (32.0f / 64.0f)};
    auto charId = b2CreateBody(worldId, &charBodyDef);
    auto charBox = b2MakeBox(0.5, 0.5);
    auto charShapeDef = b2DefaultShapeDef();
    charShapeDef.density = 1.0f;
    charShapeDef.material.friction = 0.1;
    auto charShapeId = b2CreatePolygonShape(charId, &charShapeDef, &charBox);

    physicsComponents.insert({0, {charId, charShapeId}});

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

    auto mainCharIdle = AssetLoader::loadImage(ROOT "examples/platformer/assets/walk.png");
    auto mainCharIdleTex = g.makeImage({mainCharIdle.width,
                                        mainCharIdle.height,
                                        VK_FORMAT_R8G8B8A8_SRGB,
                                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                        VK_IMAGE_ASPECT_COLOR_BIT},
                                       &mainCharIdle);

    auto floorImg = AssetLoader::loadImage(ROOT "examples/platformer/assets/grassCenter.png");
    auto floorTexture = g.makeImage({floorImg.width,
                                     floorImg.height,
                                     VK_FORMAT_R8G8B8A8_SRGB,
                                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                     VK_IMAGE_ASPECT_COLOR_BIT},
                                    &floorImg);

    mainChar.texture = &mainCharIdleTex;

    sprites.push_back(mainChar);

    GuiPass gPass{&g};
    TwoDPass tdPass{&g};

    // width 960, height 960
    // 15 tiles across
    const int mapWidth = 15;
    std::string map = "---------------"
                      "---------------"
                      "---------------"
                      "---------------"
                      "---------------"
                      "---------------"
                      "---------------"
                      "--------x------"
                      "---------------"
                      "----x----------"
                      "--------x------"
                      "---------------"
                      "---------xxxxxx"
                      "---------------"
                      "xxxxxx---------";

    for (int x = 0; x < mapWidth; ++x) {
        for (int y = 0; y < mapWidth; ++y) {
            auto index = x + mapWidth * y;

            if (map[index] != 'x')
                continue;

            auto groundBodyDef = b2DefaultBodyDef();
            groundBodyDef.position = {static_cast<float>(x) + (32.0f / 64.0f),
                                      static_cast<float>(15 - 1 - y) + (32.0f / 64.0f)};
            auto groundId = b2CreateBody(worldId, &groundBodyDef);
            auto groundBox = b2MakeBox(0.5, 0.5);
            auto groundShapeDef = b2DefaultShapeDef();
            auto shapeId = b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);

            sprites.push_back({64.0f * (x),
                               960.0f - 64.0f - (64.0f * (y)),
                               64,
                               64,
                               1,
                               1,
                               0,
                               static_cast<float>(g.getSwapchainSize().width),
                               static_cast<float>(g.getSwapchainSize().height),
                               &floorTexture});

            physicsComponents.insert({static_cast<uint32_t>(index), {groundId, shapeId}});
        }
    }

    tdPass.addSprite(sprites.data(), sprites.size());

    std::vector<Pass *> gPasses{&tdPass, &gPass};
    std::vector<Pass *> cPasses{};

    g.beginRenderLoop(gPasses,
                      cPasses,
                      [&tdPass, &sprites, &m, worldId, &physicsComponents, &gPass](double t,
                                                                                   double delta) {
                          static int d = 0;
                          static bool grounded = false;

                          int c = t - d;

                          if (c >= 50) {
                              d = t;

                              sprites[0].incrementIdleIndex();
                          }

                          //physics
                          b2World_Step(worldId, delta / 1000, 4);
                          auto cPos = b2Body_GetPosition(physicsComponents[0].bodyId);
                          sprites[0].rect.x = ((cPos.x - 0.5) * 64.0f);
                          sprites[0].rect.y = ((cPos.y - 0.5) * 64.0f);

                          auto velocity = b2Body_GetLinearVelocity(physicsComponents[0].bodyId);

                          if (m.up && grounded) {
                              velocity.y = 6.;
                              grounded = false;
                          } else if (m.down) {
                          }

                          if (m.right)
                              velocity.x = 2.5;
                          else if (m.left)
                              velocity.x = -2.5;

                          b2Body_SetLinearVelocity(physicsComponents[0].bodyId, velocity);

                          //ground collision
                          auto capacity = b2Body_GetContactCapacity(physicsComponents[0].bodyId);
                          std::vector<b2ContactData> contacts{static_cast<size_t>(capacity)};
                          auto count = b2Body_GetContactData(physicsComponents[0].bodyId,
                                                             contacts.data(),
                                                             capacity);

                          for (int i = 0; i < count; ++i) {
                              auto *m = &contacts[i].manifold;

                              if (m->pointCount == 0)
                                  continue;

                              auto n = m->normal;

                              if (!B2_ID_EQUALS(contacts[i].shapeIdA, physicsComponents[0].shapeId))
                                  n = b2Neg(n);

                              if (n.y < 0.7)
                                  grounded = true;
                          }

        //debug draw
#if 0
                          for (auto &comp : physicsComponents) {
                              auto shape = b2Shape_GetAABB(comp.second.shapeId);
                              gPass.drawDebugRect(
                                  {shape.lowerBound.x * 64.0f,
                                   shape.lowerBound.y * 64.0f,
                                   shape.upperBound.x * 64.0f - shape.lowerBound.x * 64.0f,
                                   shape.upperBound.y * 64.0f - shape.lowerBound.y * 64.0f});
                          }
#endif
                      });
}