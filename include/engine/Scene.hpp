#pragma once

#include <array>
#include <filesystem>
#include <string>

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

namespace engine {

// Persistent IDs are application data, not serialized entt::entity handles.
struct Identity { std::string value; };
struct Name { std::string value; };

// Plain serializable data; rendering converts these arrays to raylib vectors.
struct Transform3D {
    std::array<float, 3> position{0.0f, 0.0f, 0.0f};
    std::array<float, 3> rotationDegrees{0.0f, 0.0f, 0.0f};
    std::array<float, 3> scale{1.0f, 1.0f, 1.0f};
};

// The cache owns GPU resources. Components only refer to project-relative assets.
struct ModelRef { std::string path; };
struct Spin { float degreesPerSecond = 30.0f; };

entt::registry sceneFromJson(const nlohmann::json& document);
nlohmann::json sceneToJson(const entt::registry& registry);
entt::registry loadScene(const std::filesystem::path& path);
void saveScene(const entt::registry& registry, const std::filesystem::path& path);
void fixedUpdate(entt::registry& registry, float seconds);

} // namespace engine
