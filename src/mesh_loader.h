#pragma once

#include "fastgltf/types.hpp"
#include "types.h"
#include <fastgltf/core.hpp>
#include <string>

class MeshLoader
{
public:
    static Model loadModel(const std::string &path);

private:
    static Mesh loadMesh(fastgltf::Expected<fastgltf::Asset> &asset);
    static std::vector<Image> loadImage(fastgltf::Expected<fastgltf::Asset> &asset);
};
