#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <unordered_map>
#include <unordered_set>
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

enum class TextureType : int8_t {
    None = -1,
    Diffuse = 0,
    Normal = 1,
    Count = 2 // This is used as the total count
};

struct Mesh
{
    std::vector<vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<TextureType, int8_t> textureIds;
    glm::mat4 worldTransform{1.0f};
};

struct Image
{
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    unsigned char *data;

    //~Image() { delete data; }
};

struct Model
{
    std::vector<Image> textures;
    std::vector<Mesh> meshes;
};

#endif // TYPES_H
