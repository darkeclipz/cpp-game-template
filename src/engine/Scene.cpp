#include "engine/Scene.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

namespace engine {
namespace {
using Json = nlohmann::json;

std::array<float, 3> readVector(const Json& value) {
    if (!value.is_array() || value.size() != 3) {
        throw std::runtime_error("A transform vector must contain exactly three numbers");
    }
    std::array<float, 3> result{};
    for (std::size_t i = 0; i < result.size(); ++i) {
        if (!value[i].is_number()) throw std::runtime_error("Non-numeric transform value");
        result[i] = value[i].get<float>();
        if (!std::isfinite(result[i])) throw std::runtime_error("Non-finite transform value");
    }
    return result;
}

void validateTransform(const Transform3D& value) {
    for (const auto& vector : {value.position, value.rotationDegrees, value.scale}) {
        for (float number : vector) {
            if (!std::isfinite(number)) throw std::runtime_error("Non-finite transform value");
        }
    }
    for (float scale : value.scale) {
        if (scale <= 0.0f) throw std::runtime_error("Scale must be positive in this starter");
    }
}

void validateAssetPath(const std::string& text) {
    const std::filesystem::path path(text);
    if (text.empty() || path.is_absolute() || path.has_root_name() ||
        text.find('\\') != std::string::npos || text.find(':') != std::string::npos) {
        throw std::runtime_error("Use an asset-relative path with forward slashes");
    }
    for (const auto& part : path) {
        if (part == "..") throw std::runtime_error("Asset paths may not contain '..'");
    }
}
} // namespace

entt::registry sceneFromJson(const Json& document) {
    if (!document.is_object() || !document.contains("version") ||
        !document.at("version").is_number_integer() || document.at("version") != 1) {
        throw std::runtime_error("Unsupported scene schema: expected integer version 1");
    }
    const auto& entities = document.at("entities");
    if (!entities.is_array()) throw std::runtime_error("'entities' must be an array");

    // Build a new registry so malformed input never partially clears the live scene.
    entt::registry result;
    std::unordered_set<std::string> ids;
    for (const auto& item : entities) {
        const auto id = item.at("id").get<std::string>();
        if (id.empty() || !ids.insert(id).second) {
            throw std::runtime_error("Entity IDs must be non-empty and unique");
        }
        Transform3D transform;
        if (item.contains("transform")) {
            const auto& data = item.at("transform");
            if (!data.is_object()) throw std::runtime_error("'transform' must be an object");
            if (data.contains("position")) transform.position = readVector(data.at("position"));
            if (data.contains("rotation_degrees")) {
                transform.rotationDegrees = readVector(data.at("rotation_degrees"));
            }
            if (data.contains("scale")) transform.scale = readVector(data.at("scale"));
        }
        validateTransform(transform);
        const auto entity = result.create();
        result.emplace<Identity>(entity, Identity{id});
        result.emplace<Name>(entity, Name{item.value("name", id)});
        result.emplace<Transform3D>(entity, transform);
        if (item.contains("model")) {
            auto path = item.at("model").get<std::string>();
            validateAssetPath(path);
            result.emplace<ModelRef>(entity, ModelRef{std::move(path)});
        }
        if (item.contains("spin_degrees_per_second")) {
            const auto& value = item.at("spin_degrees_per_second");
            if (!value.is_number()) throw std::runtime_error("Spin speed must be a number");
            const float speed = value.get<float>();
            if (!std::isfinite(speed)) throw std::runtime_error("Spin speed must be finite");
            result.emplace<Spin>(entity, Spin{speed});
        }
    }
    return result;
}

Json sceneToJson(const entt::registry& registry) {
    std::vector<Json> entities;
    std::unordered_set<std::string> ids;
    for (const auto [entity, identity] : registry.view<const Identity>().each()) {
        if (identity.value.empty() || !ids.insert(identity.value).second) {
            throw std::runtime_error("Entity IDs must be non-empty and unique");
        }
        const auto* name = registry.try_get<Name>(entity);
        const auto* transform = registry.try_get<Transform3D>(entity);
        if (!name || !transform) throw std::runtime_error("An entity lacks Name or Transform3D");
        validateTransform(*transform);
        Json item{
            {"id", identity.value}, {"name", name->value},
            {"transform", {
                {"position", transform->position},
                {"rotation_degrees", transform->rotationDegrees},
                {"scale", transform->scale}
            }}
        };
        if (const auto* model = registry.try_get<ModelRef>(entity)) {
            validateAssetPath(model->path);
            item["model"] = model->path;
        }
        if (const auto* spin = registry.try_get<Spin>(entity)) {
            if (!std::isfinite(spin->degreesPerSecond)) {
                throw std::runtime_error("Spin speed must be finite");
            }
            item["spin_degrees_per_second"] = spin->degreesPerSecond;
        }
        entities.push_back(std::move(item));
    }
    // Stable ordering makes saved scenes easier to diff in version control.
    std::sort(entities.begin(), entities.end(), [](const Json& a, const Json& b) {
        return a.at("id").get<std::string>() < b.at("id").get<std::string>();
    });
    return Json{{"version", 1}, {"entities", entities}};
}

entt::registry loadScene(const std::filesystem::path& path) {
    std::ifstream stream(path);
    if (!stream) throw std::runtime_error("Cannot read scene: " + path.string());
    Json document;
    stream >> document;
    return sceneFromJson(document);
}

void saveScene(const entt::registry& registry, const std::filesystem::path& path) {
    const auto text = sceneToJson(registry).dump(2) + '\n';
    auto temporary = path;
    temporary += ".tmp";
    try {
        // Same-directory rename avoids truncating the previous scene on a write failure.
        // Single-writer utility: not a concurrent save system or a durable fsync journal.
        std::ofstream stream;
        stream.exceptions(std::ios::badbit | std::ios::failbit);
        stream.open(temporary, std::ios::binary | std::ios::trunc);
        stream << text;
        stream.close();
        std::filesystem::rename(temporary, path);
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
}

void fixedUpdate(entt::registry& registry, float seconds) {
    if (!std::isfinite(seconds) || seconds < 0.0f) {
        throw std::runtime_error("Time step must be finite and non-negative");
    }
    for (const auto [entity, transform, spin] : registry.view<Transform3D, const Spin>().each()) {
        (void)entity;
        const double angle = static_cast<double>(transform.rotationDegrees[1]) +
                             static_cast<double>(spin.degreesPerSecond) * seconds;
        transform.rotationDegrees[1] = static_cast<float>(std::fmod(angle, 360.0));
    }
}
} // namespace engine
