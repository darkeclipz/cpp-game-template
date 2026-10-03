#include "engine/Scene.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
template<class Function>
void mustThrow(Function function, const std::string& message) {
    try { function(); }
    catch (const std::exception&) { return; }
    throw std::runtime_error(message);
}
}

int main() {
    try {
        const auto input = nlohmann::json::parse(R"({
            "version": 1,
            "entities": [{
                "id": "cube", "name": "Test cube", "model": "models/cube.glb",
                "spin_degrees_per_second": 90.0,
                "transform": {
                    "position": [1, 2, 3], "rotation_degrees": [0, 0, 0],
                    "scale": [1, 1, 1]
                }
            }]
        })");
        auto registry = engine::sceneFromJson(input);
        require(engine::sceneToJson(registry) == input, "Scene did not round-trip");
        engine::fixedUpdate(registry, 0.5f);
        const auto updated = engine::sceneToJson(registry);
        require(std::abs(updated["entities"][0]["transform"]["rotation_degrees"][1].get<float>()
                         - 45.0f) < 0.001f, "Fixed update used the wrong time step");
        auto bad = input;
        bad["entities"].push_back(bad["entities"][0]);
        mustThrow([&] { (void)engine::sceneFromJson(bad); }, "Duplicate IDs accepted");
        bad = input;
        bad["version"] = 99;
        mustThrow([&] { (void)engine::sceneFromJson(bad); }, "Unknown version accepted");
        bad["version"] = 1.5;
        mustThrow([&] { (void)engine::sceneFromJson(bad); }, "Fractional version accepted");
        bad = input;
        bad["entities"][0]["transform"]["scale"] = {1, 0, 1};
        mustThrow([&] { (void)engine::sceneFromJson(bad); }, "Zero scale accepted");
        bad = input;
        bad["entities"][0]["transform"]["position"] = {1, 2};
        mustThrow([&] { (void)engine::sceneFromJson(bad); }, "Short vector accepted");
        bad = input;
        bad["entities"][0]["model"] = "../escape.glb";
        mustThrow([&] { (void)engine::sceneFromJson(bad); }, "Traversal path accepted");
        mustThrow([&] { engine::fixedUpdate(registry, -1.0f); }, "Negative step accepted");
        const auto empty = nlohmann::json::parse(R"({"version":1,"entities":[]})");
        require(engine::sceneToJson(engine::sceneFromJson(empty)) == empty,
                "Empty scene did not round-trip");
        std::cout << "All scene checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
