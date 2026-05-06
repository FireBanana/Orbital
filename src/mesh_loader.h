#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct vec3
{
    float x, y, z;
};

struct vec2
{
    float u, v;
};

struct vertex
{
    vec3 position;
    vec2 uv;
};

struct Mesh
{
    std::vector<vertex> vertices;
    std::vector<uint16_t> indices;
};

class MeshLoader
{
public:
    static Mesh load_mesh(const std::string &path);
};
