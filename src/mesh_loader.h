#pragma once

#include "fastgltf/types.hpp"
#include "types.h"
#include <fastgltf/core.hpp>
#include <string>

class MeshLoader
{
public:
    static Model load_model(const std::string &path);

private:
    static Mesh load_mesh(fastgltf::Expected<fastgltf::Asset> &asset);
    static std::vector<Image> load_image(fastgltf::Expected<fastgltf::Asset> &asset);
};
