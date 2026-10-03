#include "engine/Assets.hpp"
#include "engine/Scene.hpp"

#include <raylib.h>

#if SEED_WITH_EDITOR
#include <imgui.h>
#include <rlImGui.h>
#endif

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
namespace fs = std::filesystem;

struct Options {
    fs::path assets = fs::path(GetApplicationDirectory()) / "assets";
    fs::path scene;
    fs::path save = fs::current_path() / "scene.saved.json";
    bool smokeTest = false;
};

Options parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument(argv[i]);
        if (argument == "--smoke-test") {
            options.smokeTest = true;
        } else if (argument == "--assets" || argument == "--scene" || argument == "--save") {
            if (++i >= argc) throw std::runtime_error("Missing value for " + argument);
            if (argument == "--assets") options.assets = argv[i];
            else if (argument == "--scene") options.scene = argv[i];
            else options.save = argv[i];
        } else {
            throw std::runtime_error("Unknown option: " + argument);
        }
    }
    options.assets = fs::absolute(options.assets);
    if (options.scene.empty()) options.scene = options.assets / "scenes/demo.json";
    return options;
}

class Window final {
public:
    Window() {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
        InitWindow(1280, 800, "C++20 engine seed");
        if (!IsWindowReady()) throw std::runtime_error("Window initialization failed");
        SetWindowMinSize(720, 480);
        // Handle Escape ourselves, so UI keyboard capture can prevent it from closing.
        SetExitKey(KEY_NULL);
        SetTargetFPS(144);
    }
    ~Window() { CloseWindow(); }
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};

struct OrbitCamera {
    Camera3D native{{6.0f, 4.0f, 6.0f}, {0.0f, 0.7f, 0.0f},
                    {0.0f, 1.0f, 0.0f}, 45.0f, CAMERA_PERSPECTIVE};
    float yaw = 0.75f;
    float pitch = 0.4f;
    float distance = 8.0f;

    void update(bool mouseCaptured) {
        if (!mouseCaptured) {
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                const auto delta = GetMouseDelta();
                yaw -= delta.x * 0.006f;
                pitch = std::clamp(pitch + delta.y * 0.006f, -1.35f, 1.35f);
            }
            distance = std::clamp(distance * std::exp(-GetMouseWheelMove() * 0.12f),
                                  1.0f, 100.0f);
        }
        const float horizontal = distance * std::cos(pitch);
        native.position = {
            native.target.x + horizontal * std::sin(yaw),
            native.target.y + distance * std::sin(pitch),
            native.target.z + horizontal * std::cos(yaw)
        };
    }
};

#if SEED_WITH_EDITOR
class Editor final {
public:
    Editor() {
        rlImGuiSetup(true);
        ImGui::GetIO().IniFilename = nullptr;
    }
    ~Editor() { rlImGuiShutdown(); }
    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;

    void draw(entt::registry& world, engine::AssetCache& assets,
              const Options& options, bool& playing) {
        ImGui::SetNextWindowPos(ImVec2{16.0f, 16.0f}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2{330.0f, 600.0f}, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Scene inspector")) {
            ImGui::Text("%d FPS | %zu cached models", GetFPS(), assets.size());
            ImGui::Checkbox("Play fixed-step simulation", &playing);
            ImGui::TextWrapped("Right-drag outside this panel to orbit. Scroll to zoom.");
            ImGui::Separator();
            if (ImGui::Button("Save snapshot")) {
                try {
                    engine::saveScene(world, options.save);
                    status_ = "Saved: " + options.save.string();
                } catch (const std::exception& error) { status_ = error.what(); }
            }
            ImGui::SameLine();
            if (ImGui::Button("Load snapshot")) reload(world, assets, options.save);
            if (ImGui::Button("Reload startup scene")) reload(world, assets, options.scene);
            if (!status_.empty()) ImGui::TextWrapped("%s", status_.c_str());
            ImGui::SeparatorText("Entities");
            for (const auto [entity, identity, name] :
                 world.view<const engine::Identity, const engine::Name>().each()) {
                ImGui::PushID(identity.value.c_str());
                if (ImGui::Selectable(name.value.c_str(), entity == selected_)) selected_ = entity;
                ImGui::PopID();
            }
            if (world.valid(selected_) && world.all_of<engine::Transform3D>(selected_)) {
                ImGui::SeparatorText("Transform");
                auto& transform = world.get<engine::Transform3D>(selected_);
                ImGui::DragFloat3("Position", transform.position.data(), 0.02f);
                ImGui::DragFloat3("Rotation (deg)", transform.rotationDegrees.data(), 0.5f);
                ImGui::DragFloat3("Scale", transform.scale.data(), 0.01f, 0.01f, 20.0f,
                                 "%.2f", ImGuiSliderFlags_AlwaysClamp);
                if (auto* spin = world.try_get<engine::Spin>(selected_)) {
                    ImGui::DragFloat("Spin (deg/s)", &spin->degreesPerSecond, 1.0f,
                                     -360.0f, 360.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
                }
                if (const auto* model = world.try_get<engine::ModelRef>(selected_)) {
                    ImGui::TextWrapped("Model: %s", model->path.c_str());
                }
            } else {
                ImGui::TextWrapped("Select an entity to edit its transform.");
            }
        }
        ImGui::End();
    }

private:
    void reload(entt::registry& world, engine::AssetCache& assets, const fs::path& path) {
        try {
            auto replacement = engine::loadScene(path);
            assets.preload(replacement);
            world = std::move(replacement);
            selected_ = entt::null;
            status_ = "Loaded: " + path.string();
        } catch (const std::exception& error) { status_ = error.what(); }
    }
    entt::entity selected_ = entt::null;
    std::string status_;
};
#endif

int run(const Options& options) {
    auto world = engine::loadScene(options.scene);
    Window window; // Must outlive every GPU resource and the ImGui backend.
    engine::AssetCache assets(options.assets);
    assets.preload(world);
    OrbitCamera camera;
#if SEED_WITH_EDITOR
    Editor editor;
#endif
    bool playing = true;
    bool running = true;
    int frames = 0;
    constexpr double step = 1.0 / 60.0;
    double accumulator = 0.0;

    while (running && !WindowShouldClose()) {
        const double elapsed = std::clamp(static_cast<double>(GetFrameTime()), 0.0, 0.25);
        BeginDrawing();
        ClearBackground(Color{28, 31, 40, 255});
        bool mouseCaptured = false;
        bool keyboardCaptured = false;
#if SEED_WITH_EDITOR
        // Backend gathers input and begins an ImGui frame. Its draw data is rendered last.
        rlImGuiBegin();
        editor.draw(world, assets, options, playing);
        mouseCaptured = ImGui::GetIO().WantCaptureMouse;
        keyboardCaptured = ImGui::GetIO().WantCaptureKeyboard;
#endif
        if (!keyboardCaptured && IsKeyPressed(KEY_ESCAPE)) running = false;
        camera.update(mouseCaptured);
        if (playing) {
            accumulator += elapsed;
            int steps = 0;
            while (accumulator >= step && steps < 8) {
                engine::fixedUpdate(world, static_cast<float>(step));
                accumulator -= step;
                ++steps;
            }
            // Favor responsiveness after a long stall instead of an unbounded catch-up loop.
            if (accumulator >= step) accumulator = std::fmod(accumulator, step);
        } else {
            accumulator = 0.0;
        }
        BeginMode3D(camera.native);
        DrawGrid(20, 1.0f);
        engine::drawScene(world, assets);
        EndMode3D();
        DrawText("C++20 / raylib / EnTT / JSON", 16, GetScreenHeight() - 30, 18, RAYWHITE);
#if SEED_WITH_EDITOR
        rlImGuiEnd();
#else
        DrawFPS(16, 16);
#endif
        EndDrawing();
        if (options.smokeTest && ++frames >= 3) {
            // A manual-frame-control raylib build can open a window without ever
            // presenting frames or updating timing. Do not report that as success.
            if (!(GetFrameTime() > 0.0f)) {
                throw std::runtime_error("Frame timing did not advance; disable raylib custom frame control");
            }
            running = false;
        }
    }
    return 0;
}
} // namespace

int main(int argc, char** argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--help") {
                std::cout << "sandbox [--assets DIR] [--scene FILE] [--save FILE] [--smoke-test]\n";
                return 0;
            }
        }
        return run(parseOptions(argc, argv));
    } catch (const std::exception& error) {
        std::cerr << "Fatal: " << error.what() << '\n';
        return 1;
    }
}
