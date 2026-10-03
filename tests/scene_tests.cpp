#include "engine/Scene.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <stdexcept>

namespace {
nlohmann::json sampleScene() {
    return nlohmann::json::parse(R"({
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
}
} // namespace

TEST_CASE("Scene JSON round-trips", "[scene]") {
    const auto input = sampleScene();
    const auto registry = engine::sceneFromJson(input);
    REQUIRE(engine::sceneToJson(registry) == input);
}

TEST_CASE("Fixed update uses the supplied time step", "[scene][update]") {
    auto registry = engine::sceneFromJson(sampleScene());
    engine::fixedUpdate(registry, 0.5f);
    const auto updated = engine::sceneToJson(registry);
    const auto rotation = updated["entities"][0]["transform"]["rotation_degrees"][1].get<float>();
    REQUIRE_THAT(rotation, Catch::Matchers::WithinAbs(45.0, 0.001));
}

TEST_CASE("Scene rejects duplicate entity IDs", "[scene][validation]") {
    auto input = sampleScene();
    input["entities"].push_back(input["entities"][0]);
    REQUIRE_THROWS_AS(engine::sceneFromJson(input), std::runtime_error);
}

TEST_CASE("Scene rejects unknown schema versions", "[scene][validation]") {
    auto input = sampleScene();
    input["version"] = 99;
    REQUIRE_THROWS_AS(engine::sceneFromJson(input), std::runtime_error);
}

TEST_CASE("Scene rejects fractional schema versions", "[scene][validation]") {
    auto input = sampleScene();
    input["version"] = 1.5;
    REQUIRE_THROWS_AS(engine::sceneFromJson(input), std::runtime_error);
}

TEST_CASE("Scene rejects zero scale", "[scene][validation]") {
    auto input = sampleScene();
    input["entities"][0]["transform"]["scale"] = {1, 0, 1};
    REQUIRE_THROWS_AS(engine::sceneFromJson(input), std::runtime_error);
}

TEST_CASE("Scene rejects short transform vectors", "[scene][validation]") {
    auto input = sampleScene();
    input["entities"][0]["transform"]["position"] = {1, 2};
    REQUIRE_THROWS_AS(engine::sceneFromJson(input), std::runtime_error);
}

TEST_CASE("Scene rejects asset path traversal", "[scene][validation]") {
    auto input = sampleScene();
    input["entities"][0]["model"] = "../escape.glb";
    REQUIRE_THROWS_AS(engine::sceneFromJson(input), std::runtime_error);
}

TEST_CASE("Fixed update rejects negative time steps", "[scene][update][validation]") {
    auto registry = engine::sceneFromJson(sampleScene());
    REQUIRE_THROWS_AS(engine::fixedUpdate(registry, -1.0f), std::runtime_error);
}

TEST_CASE("Empty scene JSON round-trips", "[scene]") {
    const auto input = nlohmann::json::parse(R"({"version":1,"entities":[]})");
    REQUIRE(engine::sceneToJson(engine::sceneFromJson(input)) == input);
}
