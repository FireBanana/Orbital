#define STB_IMAGE_IMPLEMENTATION
#include "mesh_loader.h"
#include "fastgltf/math.hpp"
#include "fastgltf/tools.hpp"
#include "stb_image.h"
#include <iostream>

Model MeshLoader::load_model(const std::string &path)
{
    auto file = fastgltf::GltfDataBuffer::FromPath(path);
    fastgltf::Parser parser{};

    if (!file) {
        std::cout << "error loading file" << std::endl;
        return {};
    }

    auto asset = parser.loadGltf(file.get(), path);

    if (asset.error() != fastgltf::Error::None) {
        std::cout << "error parsing file" << std::endl;
        return {};
    }

    std::cout << "model loaded successfully" << std::endl;

    return {load_image(asset), load_mesh(asset)};
}

Mesh MeshLoader::load_mesh(fastgltf::Expected<fastgltf::Asset> &asset)
{
    auto &mesh = asset->meshes[0];

    std::vector<vertex> vertices;
    std::vector<uint16_t> indices;

    for (auto it = mesh.primitives.begin(); it != mesh.primitives.end(); ++it) {
        auto position_it = it->findAttribute("POSITION");
        auto texcoord_it = it->findAttribute("TEXCOORD_0");
        auto normal_it = it->findAttribute("NORMAL");

        auto &primitive = *it;
        auto &position_accessor = asset->accessors[position_it->accessorIndex];

        vertices.resize(position_accessor.count);

        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
            asset.get(), position_accessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
                vertices[idx].position = {pos.x(), pos.y(), pos.z()};
                vertices[idx].uv = {};
            });

        auto &texcoord_accessor = asset->accessors[texcoord_it->accessorIndex];

        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset.get(),
                                                                  texcoord_accessor,
                                                                  [&](fastgltf::math::fvec2 uv,
                                                                      std::size_t idx) {
                                                                      vertices[idx].uv = {uv.x(),
                                                                                          uv.y()};
                                                                  });

        auto &normal_accessor = asset->accessors[normal_it->accessorIndex];

        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
            asset.get(), normal_accessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
                vertices[idx].normal = {pos.x(), pos.y(), pos.z()};
            });

        auto &index_accessor = asset->accessors[it->indicesAccessor.value()];
        indices.resize(index_accessor.count);

        fastgltf::copyFromAccessor<uint16_t>(asset.get(), index_accessor, indices.data());
    }

    return {vertices, indices};
}

std::vector<Image> MeshLoader::load_image(fastgltf::Expected<fastgltf::Asset> &asset)
{
    std::vector<Image> result;

    for (auto &i : asset->images) {
        std::visit(fastgltf::visitor{
                       [](auto &arg) {
                           std::cout << "Error: Texture import failed" << std::endl;
                       },
                       [&](fastgltf::sources::URI &filepath) {
                           const std::string path(filepath.uri.path().begin(),
                                                  filepath.uri.path().end());
                           int width, height, channels;
                           auto *data = stbi_load(path.c_str(), &width, &height, &channels, 4);
                           result.push_back({static_cast<uint32_t>(width),
                                             static_cast<uint32_t>(height),
                                             static_cast<uint32_t>(channels),
                                             data});
                       },
                       [&](fastgltf::sources::Array &vec) {
                           int width, height, channels;
                           auto *data = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(
                                                                  vec.bytes.data()),
                                                              static_cast<int>(vec.bytes.size()),
                                                              &width,
                                                              &height,
                                                              &channels,
                                                              4);

                           result.push_back({static_cast<uint32_t>(width),
                                             static_cast<uint32_t>(height),
                                             static_cast<uint32_t>(channels),
                                             data});
                       },
                       [&](fastgltf::sources::BufferView &view) {
                           auto &buffer_view = asset->bufferViews[view.bufferViewIndex];
                           auto &buffer = asset->buffers[buffer_view.bufferIndex];

                           auto array_fn = [&](fastgltf::sources::Array &vec) {
                               int width, height, channels;

                               auto *data = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(
                                                                      vec.bytes.data()
                                                                      + buffer_view.byteOffset),
                                                                  static_cast<int>(
                                                                      buffer_view.byteLength),
                                                                  &width,
                                                                  &height,
                                                                  &channels,
                                                                  0);

                               result.push_back({static_cast<uint32_t>(width),
                                                 static_cast<uint32_t>(height),
                                                 static_cast<uint32_t>(channels),
                                                 data});
                           };

                           std::visit(fastgltf::visitor{[](auto &arg) {}, array_fn}, buffer.data);
                       }},
                   i.data);
    }

    return result;
}