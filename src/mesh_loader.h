#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct vec3
{
    float x, y, z;
};

struct vec4
{
    float x, y, z, w;
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
    std::vector<uint16_t> indices;
};

struct Image
{
    int width;
    int height;
    int channels;
    unsigned char *data;

    //~Image() { delete data; }
};

class MeshLoader
{
public:
    static Mesh load_mesh(const std::string &path);
    static std::vector<Image> load_image(const std::string &path);
};
