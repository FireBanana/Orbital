#pragma once

#include "fastgltf/types.hpp"
#include "types.h"
#include <fastgltf/core.hpp>
#include <string>

class AssetLoader
{
public:
    static Model loadModel(const std::string &path);
    static Image loadImage(const std::string &path);

private:
    static std::vector<Mesh> loadMeshes(fastgltf::Expected<fastgltf::Asset> &asset);
    static std::vector<Image> loadImage(fastgltf::Expected<fastgltf::Asset> &asset);
};
