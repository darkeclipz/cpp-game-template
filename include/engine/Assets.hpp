#pragma once

#include "engine/Scene.hpp"
#include <raylib.h>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace engine {

class ModelAsset final {
public:
    static std::unique_ptr<ModelAsset> load(const std::filesystem::path& path);
    ~ModelAsset() noexcept;
    ModelAsset(const ModelAsset&) = delete;
    ModelAsset& operator=(const ModelAsset&) = delete;
    ModelAsset(ModelAsset&&) = delete;
    ModelAsset& operator=(ModelAsset&&) = delete;

    // Borrowed for immediate rendering only. Never unload or mutate this model.
    const Model& native() const noexcept { return model_; }

private:
    ModelAsset() = default;
    Model model_{};
};

class AssetCache final {
public:
    explicit AssetCache(std::filesystem::path root);
    void preload(const entt::registry& registry);
    const ModelAsset& get(const std::string& path) const;
    std::size_t size() const noexcept { return models_.size(); }

private:
    std::filesystem::path resolve(const std::string& path) const;
    std::filesystem::path root_;
    std::unordered_map<std::string, std::unique_ptr<ModelAsset>> models_;
};

void drawScene(const entt::registry& registry, const AssetCache& assets);

} // namespace engine
