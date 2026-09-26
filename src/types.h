#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <functional>
#include <glm/ext/matrix_float4x4.hpp>
#include <string>
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

struct GuiRect
{
    float x, y, width, height;
};

struct GuiSlider
{
    std::string name;
    float min;
    float max;
    float value;
    std::function<void(float)> valueChanged;
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
    MetallicRoughness = 2,

    // Additional textures (4 allowed right now for no reason)
    Texture0 = 3,
    Texture1 = 4,
    Texture2 = 5,
    Texture3 = 6,

    Count = 7 // This is used as the total count
};

struct Mesh
{
    std::vector<vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<TextureType, int32_t> textureIds;
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
