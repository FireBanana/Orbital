#ifndef GTYPES_H
#define GTYPES_H

#include <cstdint>
struct Sprite
{
    float x;
    float y;
    float width;
    float height;
    uint32_t totalSpriteX;
    uint32_t totalSpriteY;
    uint32_t currentIndex;
    float screenWidth;
    float screenHeight;

    void incrementIdleIndex() { currentIndex = ++currentIndex % totalSpriteX; }
};

#endif // GTYPES_H
