#define STB_IMAGE_IMPLEMENTATION
#include "asset_loader.h"
#include "fastgltf/math.hpp"
#include "fastgltf/tools.hpp"
#include "stb_image.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

Model AssetLoader::loadModel(const std::string &path)
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

    return {loadImage(asset), loadMeshes(asset)};
}

Image AssetLoader::loadImage(const std::string &path)
{
    int width, height, channels;
    auto *img = stbi_load(path.c_str(), &width, &height, &channels, 4);

    if (img == nullptr)
        throw;

    return {static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            static_cast<uint32_t>(channels),
            img};
}

std::vector<Mesh> AssetLoader::loadMeshes(fastgltf::Expected<fastgltf::Asset> &asset)
{
    std::vector<Mesh> meshes;

    const size_t sceneIndex = asset->defaultScene.value_or(0);

    fastgltf::iterateSceneNodes(
        asset.get(),
        sceneIndex,
        fastgltf::math::fmat4x4(),
        [&](fastgltf::Node &node, fastgltf::math::fmat4x4 matrix) {
            if (!node.meshIndex.has_value())
                return;

            auto &gltfMesh = asset->meshes[node.meshIndex.value()];

            for (auto &primitive : gltfMesh.primitives) {
                Mesh mesh;
                mesh.worldTransform = glm::make_mat4(matrix.data());

                // Positions
                auto &positionAccessor
                    = asset->accessors[primitive.findAttribute("POSITION")->accessorIndex];
                mesh.vertices.resize(positionAccessor.count);

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    asset.get(), positionAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
                        mesh.vertices[idx].position = {pos.x(), pos.y(), pos.z()};
                    });

                // UVs
            }
        });

    // for (auto it = mesh.primitives.begin(); it != mesh.primitives.end(); ++it) {
    //     auto positionIt = it->findAttribute("POSITION");
    //     auto texcoordIt = it->findAttribute("TEXCOORD_0");
    //     auto normalIt = it->findAttribute("NORMAL");

    //     auto &primitive = *it;
    //     auto &positionAccessor = asset->accessors[positionIt->accessorIndex];

    //     //vertices.resize(positionAccessor.count);
    //     const size_t baseVertex = vertices.size();
    //     vertices.resize(baseVertex + positionAccessor.count);

    //     fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
    //         asset.get(), positionAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
    //             vertices[baseVertex + idx].position = {pos.x(), pos.y(), pos.z()};
    //             vertices[baseVertex + idx].uv = {};
    //         });

    //     auto &texcoordAccessor = asset->accessors[texcoordIt->accessorIndex];

    //     fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset.get(),
    //                                                               texcoordAccessor,
    //                                                               [&](fastgltf::math::fvec2 uv,
    //                                                                   std::size_t idx) {
    //                                                                   vertices[idx].uv = {uv.x(),
    //                                                                                       uv.y()};
    //                                                               });

    //     auto &normalAccessor = asset->accessors[normalIt->accessorIndex];

    //     fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
    //         asset.get(), normalAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
    //             vertices[baseVertex + idx].normal = {pos.x(), pos.y(), pos.z()};
    //         });

    //     auto &indexAccessor = asset->accessors[it->indicesAccessor.value()];
    //     const size_t firstIndex = indices.size();
    //     indices.resize(firstIndex + indexAccessor.count);

    //     fastgltf::copyFromAccessor<uint32_t>(asset.get(), indexAccessor, indices.data());

    //     for (size_t i = firstIndex; i < indices.size(); ++i)
    //         indices[i] += static_cast<uint32_t>(baseVertex);
    // }

    return {vertices, indices};
}

std::vector<Image> AssetLoader::loadImage(fastgltf::Expected<fastgltf::Asset> &asset)
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
                                             static_cast<uint32_t>(4),
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
                                             static_cast<uint32_t>(4),
                                             data});
                       },
                       [&](fastgltf::sources::BufferView &view) {
                           auto &bufferView = asset->bufferViews[view.bufferViewIndex];
                           auto &buffer = asset->buffers[bufferView.bufferIndex];

                           auto arrayFn = [&](fastgltf::sources::Array &vec) {
                               int width, height, channels;

                               auto *data = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(
                                                                      vec.bytes.data()
                                                                      + bufferView.byteOffset),
                                                                  static_cast<int>(
                                                                      bufferView.byteLength),
                                                                  &width,
                                                                  &height,
                                                                  &channels,
                                                                  4);

                               result.push_back({static_cast<uint32_t>(width),
                                                 static_cast<uint32_t>(height),
                                                 static_cast<uint32_t>(4),
                                                 data});
                           };

                           std::visit(fastgltf::visitor{[](auto &arg) {}, arrayFn}, buffer.data);
                       }},
                   i.data);
    }

    return result;
}