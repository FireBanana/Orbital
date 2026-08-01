#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <vector>

struct vec3
{
    float x, y, z;
};

struct vec4
{
    float x, y, z, w;
};

struct Rect
{
    float x, y, width, height;
};

struct vec2
{
    float u, v;
};

struct vertex
{
    vec3 position;
    vec3 normal;
    vec2 uv;
};

struct Mesh
{
    std::vector<vertex> vertices;
    std::vector<uint32_t> indices;
};

struct Image
{
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    unsigned char *data;

    //~Image() { delete data; }
};

struct Material
{
    std::vector<Image> textures;
};

struct Model
{
    Material material;
    Mesh mesh;
};

#endif // TYPES_H
