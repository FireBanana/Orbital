#include "fastgltf/types.hpp"
#include "types.h"
#define STB_IMAGE_IMPLEMENTATION
#include "asset_loader.h"
#include "fastgltf/math.hpp"
#include "fastgltf/tools.hpp"
#include "stb_image.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

Model AssetLoader::loadScene(const std::string &path)
{
    auto file = fastgltf::GltfDataBuffer::FromPath(path);
    fastgltf::Parser parser{};

    if (!file) {
        std::cout << "error loading file" << std::endl;
        return {};
    }

    auto asset = parser.loadGltf(file.get(), path);

    if (asset.error() != fastgltf::Error::None) {
        std::cout << "error parsing file: " << fastgltf::getErrorName(asset.error()) << std::endl;
        return {};
    }

    std::cout << "model loaded successfully" << std::endl;

    auto [meshes, images] = internalLoadModel(asset);

    return {std::move(images), std::move(meshes)};
}

Image AssetLoader::loadImage(const std::string &path)
{
    int width, height, channels;
    auto *img = stbi_load(path.c_str(), &width, &height, &channels, 4);

    if (img == nullptr)
        throw;

    return {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        static_cast<uint32_t>(channels),
        img};
}

std::tuple<std::vector<Mesh>, std::vector<Image>> AssetLoader::internalLoadModel(
    fastgltf::Expected<fastgltf::Asset> &asset)
{
    std::vector<Mesh> meshes;
    std::vector<Image> images;

    const size_t sceneIndex = asset->defaultScene.value_or(0);
    std::unordered_set<size_t> textureCache;

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

                if (primitive.materialIndex.has_value()) {
                    auto &mat = asset->materials[*primitive.materialIndex];

                    auto setTexture = [&mat, &asset, &textureCache, &mesh, &images](fastgltf::Optional<fastgltf::TextureInfo> &textureInfo, TextureType tType) {
                        if (mat.pbrData.baseColorTexture.has_value()) {
                            auto &tex = asset->textures[textureInfo->textureIndex];

                            if (tex.imageIndex.has_value()) {
                                const size_t resourceImage = *tex.imageIndex;

                                if (auto it = textureCache.find(resourceImage);
                                    it != textureCache.end()) {
                                    mesh.textureIds[tType] = static_cast<int8_t>(*it);
                                } else {
                                    images.push_back(
                                        loadModelImage(asset, asset->images[*tex.imageIndex]));
                                    mesh.textureIds[tType] = static_cast<int8_t>(images.size() - 1);
                                    // textureCache.emplace(resourceImage, mesh.textureIds[mesh.textureIds.size() - 1]);
                                }
                            }
                        }
                    };

                    // Diffuse
                    setTexture(mat.pbrData.baseColorTexture, TextureType::Diffuse);

                    // Normal
                    auto normalInfo = fastgltf::Optional<fastgltf::TextureInfo>(std::move(*mat.normalTexture));
                    setTexture(normalInfo, TextureType::Normal);
                }

                // Positions
                {
                    auto &positionAccessor
                        = asset->accessors[primitive.findAttribute("POSITION")->accessorIndex];

                    if (!positionAccessor.bufferViewIndex.has_value())
                        continue;

                    mesh.vertices.resize(positionAccessor.count);

                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                        asset.get(),
                        positionAccessor,
                        [&](fastgltf::math::fvec3 pos, std::size_t idx) {
                            mesh.vertices[idx].position = {pos.x(), pos.y(), pos.z()};
                        });
                }

                // UVs
                {
                    auto &uvAccessor
                        = asset->accessors[primitive.findAttribute("TEXCOORD_0")->accessorIndex];

                    // Currently assuming texcoord0 is UV
                    if (!uvAccessor.bufferViewIndex.has_value()
                        || uvAccessor.type != fastgltf::AccessorType::Vec2)
                        continue;

                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                        asset.get(), uvAccessor, [&](fastgltf::math::fvec2 uv, std::size_t idx) {
                            mesh.vertices[idx].uv = {uv.x(), uv.y()};
                        });
                }

                // Normals
                {
                    auto &normalAccessor
                        = asset->accessors[primitive.findAttribute("NORMAL")->accessorIndex];

                    if (!normalAccessor.bufferViewIndex.has_value())
                        continue;

                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                        asset.get(),
                        normalAccessor,
                        [&](fastgltf::math::fvec3 pos, std::size_t idx) {
                            mesh.vertices[idx].normal = {pos.x(), pos.y(), pos.z()};
                        });
                }

                // Indices
                auto &indexAccessor = asset->accessors[primitive.indicesAccessor.value()];

                if (!indexAccessor.bufferViewIndex.has_value())
                    continue;

                mesh.indices.resize(indexAccessor.count);

                fastgltf::copyFromAccessor<uint32_t>(asset.get(), indexAccessor, mesh.indices.data());

                meshes.push_back(std::move(mesh));
            }
        });

    return {std::move(meshes), std::move(images)};
}

Image AssetLoader::loadModelImage(fastgltf::Expected<fastgltf::Asset> &asset, fastgltf::Image &image)
{
    Image result{};

    std::visit(
        fastgltf::visitor{
            [](auto &arg) { std::cout << "Error: Texture import failed" << std::endl; },
            [&](fastgltf::sources::URI &filepath) {
                const std::string path(filepath.uri.path().begin(), filepath.uri.path().end());
                int width, height, channels;
                auto *data = stbi_load(path.c_str(), &width, &height, &channels, 4);
                result
                    = {static_cast<uint32_t>(width),
                       static_cast<uint32_t>(height),
                       static_cast<uint32_t>(4),
                       data};
            },
            [&](fastgltf::sources::Array &vec) {
                int width, height, channels;
                auto *data = stbi_load_from_memory(
                    reinterpret_cast<const stbi_uc *>(vec.bytes.data()),
                    static_cast<int>(vec.bytes.size()),
                    &width,
                    &height,
                    &channels,
                    4);

                result
                    = {static_cast<uint32_t>(width),
                       static_cast<uint32_t>(height),
                       static_cast<uint32_t>(4),
                       data};
            },
            [&](fastgltf::sources::BufferView &view) {
                auto &bufferView = asset->bufferViews[view.bufferViewIndex];
                auto &buffer = asset->buffers[bufferView.bufferIndex];

                auto arrayFn = [&](fastgltf::sources::Array &vec) {
                    int width, height, channels;

                    auto *data = stbi_load_from_memory(
                        reinterpret_cast<const stbi_uc *>(vec.bytes.data() + bufferView.byteOffset),
                        static_cast<int>(bufferView.byteLength),
                        &width,
                        &height,
                        &channels,
                        4);

                    result
                        = {static_cast<uint32_t>(width),
                           static_cast<uint32_t>(height),
                           static_cast<uint32_t>(4),
                           data};
                };

                std::visit(fastgltf::visitor{[](auto &arg) {}, arrayFn}, buffer.data);
            }},
        image.data);

    return result;
}
