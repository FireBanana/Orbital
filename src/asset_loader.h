#pragma once

#include "fastgltf/types.hpp"
#include "types.h"
#include <fastgltf/core.hpp>
#include <string>

class AssetLoader
{
public:
    static Model loadScene(const std::string &path);
    static Image loadImage(const std::string &path);

private:
    static std::tuple<std::vector<Mesh>, std::vector<Image>> internalLoadModel(
        fastgltf::Expected<fastgltf::Asset> &asset);
    static Image loadModelImage(fastgltf::Expected<fastgltf::Asset> &asset, fastgltf::Image &image);
};
