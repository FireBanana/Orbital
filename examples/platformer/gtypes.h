#ifndef GTYPES_H
#define GTYPES_H

#include <cstdint>
#include <types.h>

#include <box2d/id.h>

class Texture;

struct PhysicsComponent
{
    b2BodyId bodyId;
    b2ShapeId shapeId;
};

struct Sprite
{
    Rect rect;
    uint32_t totalSpriteX;
    uint32_t totalSpriteY;
    uint32_t currentIndex;
    float screenWidth;
    float screenHeight;
    Texture *texture; //Is also passed to shader (bad)

    void incrementIdleIndex() { currentIndex = ++currentIndex % totalSpriteX; }
};

struct Movement
{
    unsigned int up : 1;
    unsigned int down : 1;
    unsigned int left : 1;
    unsigned int right : 1;
};

#endif // GTYPES_H
