#include "mesh_loader.h"
#include "fastgltf/core.hpp"
#include "fastgltf/math.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include <iostream>

Mesh MeshLoader::load_mesh(const std::string &path)
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

void MeshLoader::load_image(const std::string &path)
{
    auto file = fastgltf::GltfDataBuffer::FromPath(path);
    fastgltf::Parser parser{};

    if (!file) {
        std::cout << "error loading file" << std::endl;
        return;
    }

    auto asset = parser.loadGltf(file.get(), path);

    for (auto &i : asset->images) {
        std::visit(fastgltf::visitor{[](auto &arg) {},
                                     [&](fastgltf::sources::URI &filepath) {},
                                     [&](fastgltf::sources::Array &vec) {},
                                     [&](fastgltf::sources::BufferView &view) {}},
                   i.data);
    }
}