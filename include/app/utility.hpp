#pragma once
#include "ve_model.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
namespace ve{
    template<typename T, typename... Rest>
    void hashCombine(std::size_t& seed, const T& v, const Rest&... rest){
        std::hash<T> hasher;
        seed ^=std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);
    }

    inline const std::vector<std::string> modelFileNames = {
        "colored_cube",
        "cube",
        "quad",
        "flat_vase",
        "smooth_vase",
        "Cute_Demon"
        "CesiumMan"
    };
    inline const std::vector<std::string> textureFileNames = {
        "brick_texture",
        "metal",
        "wood",
        "wall_gray",
        "tile"
    };
    inline std::unordered_map<std::string, std::shared_ptr<VeModel>> preLoadedModels;
    inline void preLoadModels(VeDevice& veDevice){
        preLoadedModels["cube"] = VeModel::createModelFromFile(veDevice, "assets/models/cube.obj");
        preLoadedModels["quad"] = VeModel::createModelFromFile(veDevice, "assets/models/quad.obj");
        preLoadedModels["flat_vase"] = VeModel::createModelFromFile(veDevice, "assets/models/flat_vase.obj");
        preLoadedModels["smooth_vase"] = VeModel::createModelFromFile(veDevice, "assets/models/smooth_vase.obj");
        preLoadedModels["colored_cube"] = VeModel::createModelFromFile(veDevice, "assets/models/colored_cube.obj");
        preLoadedModels["Cute_Demon"] = VeModel::createModelFromFile(veDevice, "assets/models/result.gltf");
        preLoadedModels["CesiumMan"] = VeModel::createModelFromFile(veDevice, "assets/models/CesiumManAnimations.gltf");
    }
    inline void cleanupPreloadedModels() {
    for (auto& [key, model] : preLoadedModels) {
        model.reset(); 
    }
    preLoadedModels.clear();  
}

}