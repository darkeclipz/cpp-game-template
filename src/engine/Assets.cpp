#include "engine/Assets.hpp"

#include <raymath.h>
#include <rlgl.h>
#include <stdexcept>
#include <utility>

namespace engine {

std::unique_ptr<ModelAsset> ModelAsset::load(const std::filesystem::path& path) {
    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("Model file not found: " + path.string());
    }
    const auto extension = path.extension().string();
    if (extension != ".glb" && extension != ".gltf" && extension != ".obj") {
        throw std::runtime_error("This starter accepts .glb, .gltf and .obj models");
    }
    // Construct the owner before loading, so failed validation also frees partial data.
    auto owner = std::unique_ptr<ModelAsset>(new ModelAsset);
    owner->model_ = LoadModel(path.string().c_str());
    if (!IsModelValid(owner->model_)) {
        throw std::runtime_error("Model import failed: " + path.string());
    }
    return owner;
}

ModelAsset::~ModelAsset() noexcept {
    // raylib 6.0 UnloadModel frees meshes/maps, but NOT the textures or shaders.
    // These textures came from LoadModel; they are not shared with other ModelAssets.
    // The default shader/white texture belong to raylib and must not be unloaded here.
    // No allocation in the destructor. Zero matching IDs to avoid duplicate frees.
    if (model_.materials) {
        for (int material = 0; material < model_.materialCount; ++material) {
            auto* maps = model_.materials[material].maps;
            if (!maps) continue;
            for (int slot = MATERIAL_MAP_ALBEDO; slot <= MATERIAL_MAP_BRDF; ++slot) {
                const Texture2D texture = maps[slot].texture;
                if (texture.id == 0 || texture.id == rlGetTextureIdDefault()) continue;
                for (int other = 0; other < model_.materialCount; ++other) {
                    auto* otherMaps = model_.materials[other].maps;
                    if (!otherMaps) continue;
                    for (int index = MATERIAL_MAP_ALBEDO; index <= MATERIAL_MAP_BRDF; ++index) {
                        if (otherMaps[index].texture.id == texture.id) {
                            otherMaps[index].texture.id = 0;
                        }
                    }
                }
                UnloadTexture(texture);
            }
        }
    }
    // No custom shader or animation-pose ownership exists in this static-model starter.
    UnloadModel(model_);
}

AssetCache::AssetCache(std::filesystem::path root)
    : root_(std::filesystem::canonical(std::move(root))) {
    if (!std::filesystem::is_directory(root_)) {
        throw std::runtime_error("Asset root is not a directory");
    }
}

std::filesystem::path AssetCache::resolve(const std::string& text) const {
    const std::filesystem::path relative(text);
    if (relative.empty() || relative.is_absolute() || relative.has_root_name()) {
        throw std::runtime_error("An asset path must be relative");
    }
    const auto candidate = std::filesystem::weakly_canonical(root_ / relative);
    const auto withinRoot = candidate.lexically_relative(root_);
    if (withinRoot.empty() || withinRoot.is_absolute()) {
        throw std::runtime_error("Cannot resolve asset under the asset root");
    }
    for (const auto& part : withinRoot) {
        if (part == "..") throw std::runtime_error("Asset resolves outside the asset root");
    }
    return candidate;
}

void AssetCache::preload(const entt::registry& registry) {
    for (const auto [entity, reference] : registry.view<const ModelRef>().each()) {
        (void)entity;
        if (!models_.contains(reference.path)) {
            auto resource = ModelAsset::load(resolve(reference.path));
            models_.emplace(reference.path, std::move(resource));
        }
    }
}

const ModelAsset& AssetCache::get(const std::string& path) const {
    return *models_.at(path);
}

void drawScene(const entt::registry& registry, const AssetCache& assets) {
    for (const auto [entity, transform, reference] :
         registry.view<const Transform3D, const ModelRef>().each()) {
        (void)entity;
        const auto& p = transform.position;
        const auto& r = transform.rotationDegrees;
        const auto& s = transform.scale;
        const Quaternion orientation = QuaternionFromEuler(r[0] * DEG2RAD,
                                                           r[1] * DEG2RAD,
                                                           r[2] * DEG2RAD);
        Vector3 axis{0.0f, 1.0f, 0.0f};
        float radians = 0.0f;
        QuaternionToAxisAngle(orientation, &axis, &radians);
        DrawModelEx(assets.get(reference.path).native(), Vector3{p[0], p[1], p[2]},
                    axis, radians * RAD2DEG, Vector3{s[0], s[1], s[2]}, WHITE);
    }
}
} // namespace engine
