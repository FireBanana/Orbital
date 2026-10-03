#ifndef ECS_H
#define ECS_H

#include <cstdint>

using Entity = uint32_t;

struct Position
{
    float x;
    float y;
    float z;
};

struct Rotation
{
    float pitch;
    float yaw;
    float roll;
};

class World
{};

#endif