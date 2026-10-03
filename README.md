# C++20 game engine seed

A deliberately small desktop starting point with C++20, CMake, raylib, EnTT,
nlohmann/json, spdlog, Dear ImGui, the rlImGui integration backend, and Catch2 tests.

## What is implemented

- A 3D desktop sandbox, orbit camera, and an original bundled GLB cube.
- Two ECS entities referring to one cached model resource.
- Static `.glb`, `.gltf`, and `.obj` model loading through raylib.
- Explicit GPU-resource ownership, including imported material texture cleanup.
- An optional ImGui entity/transform inspector, pause toggle, and scene snapshots.
- A 60 Hz fixed-update accumulator with bounded catch-up after stalls.
- Versioned JSON scene serialization with stable application-level entity IDs.
- Input capture handling for the inspector and separate non-graphical scene tests.
- Debug, editor-free Release, and graphics-free test CMake presets.
- Console logging for startup, scene snapshots, shutdown, and errors through spdlog.

This is not a full editor or a complete renderer. There is no physics, lighting
system, PBR shader, shadow pass, skeletal animation playback, asset hot reload,
parent/child scene graph, picking, scripting, networking, or game UI framework.
Rotation in the demo is a simple ECS Spin component, not skeletal animation.

## Requirements

Use a C++20-capable toolchain, CMake 3.25 or newer, Git, and Ninja for the supplied
presets. The first configure needs network access to GitHub. A desktop OpenGL
context and raylib's platform development dependencies are needed for the sandbox.

Windows: use an MSVC Developer PowerShell or equivalent configured compiler shell.
macOS: install the Xcode command-line tools plus CMake and Ninja.
Linux: the template selects GLFW's X11 backend. For a Debian/Ubuntu-style setup,
the upstream raylib Linux instructions list the relevant development packages:

```sh
sudo apt install build-essential cmake ninja-build git libasound2-dev \
  libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev \
  libxcursor-dev libxinerama-dev
```

Package names can differ by distribution. See the upstream platform instructions
in the references below. These platforms have not been tested by this template's
generation environment.

## Build and run

From this directory:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/bin/sandbox
```

On Windows, run `./build/debug/bin/sandbox.exe`. The provided presets use Ninja,
so their executable path does not contain an additional Visual Studio `Debug`
subdirectory. Assets are copied next to the executable on every build. At run
time the default asset root is resolved relative to the executable, not the shell's
working directory.

```sh
cmake --preset release
cmake --build --preset release
./build/release/bin/sandbox
```

The Release preset turns off the inspector and does not fetch ImGui or rlImGui.
The core simulation and rendering remain enabled.

The sandbox logs to the console with timestamps and severity levels, with color
when supported by the terminal. Startup, scene loading/saving, and shutdown use
`info`; snapshot failures use `error`, and fatal errors use `critical`. To add a
message in `src/main.cpp`, use `spdlog::info("[engine] Loaded {}", name)`
(or `warn`/`error`). Application messages use the `[engine]` prefix.
Raylib diagnostics are routed through the same spdlog console logger with a
`[raylib]` prefix and matching severity levels (`LOG_FATAL` maps to `critical`
and still exits the process). The spdlog level controls filtering for both sources;
the default is `info`.

For scene tests without raylib, a GPU, or a window system:

```sh
cmake --preset headless-tests
cmake --build --preset headless-tests
ctest --preset headless-tests
```

The scene tests use Catch2, with each test case automatically registered in CTest.
Add new `TEST_CASE`s to `tests/scene_tests.cpp`. Set `SEED_BUILD_TESTS=OFF` to
disable the tests and skip fetching Catch2.

This configuration still fetches EnTT, nlohmann/json, and Catch2. If Ninja is not available,
use a normal CMake generator workflow instead of the presets:

```sh
cmake -S . -B build/manual -DCMAKE_BUILD_TYPE=Debug
cmake --build build/manual --config Debug
ctest --test-dir build/manual -C Debug --output-on-failure
```

With a multi-configuration generator, the sandbox and staged assets are under
`build/manual/bin/Debug/` for the Debug configuration.

## Controls and scene files

Right-drag outside the inspector to orbit; use the mouse wheel to zoom. Select an
entity in the inspector to edit its position, Euler rotation in degrees, positive
scale, or spin speed. Pause the simulation before making a persistent rotation
edit. Escape closes the application unless the inspector is capturing keyboard
input.

`Save snapshot` writes `scene.saved.json` in the process working directory.
`Load snapshot` loads that file. `Reload startup scene` reloads the original scene.
The bundled source scene is not overwritten by these buttons. An unsuccessful
parse or model preload leaves the live scene in place.

```sh
./build/debug/bin/sandbox --assets /path/to/assets --scene /path/to/scene.json
./build/debug/bin/sandbox --save /writable/directory/my-scene.json
./build/debug/bin/sandbox --smoke-test
```

`--smoke-test` opens a real graphics window and exits after three frames. It is not
a headless rendering mode. It also checks that frame timing advances, so a
misconfigured manual-frame-control build cannot silently pass. All explicitly
supplied CLI paths are interpreted by
the filesystem relative to the current working directory unless absolute.

## Add a model

Place a trusted, authored static model under `assets/models/` and add an entity to
`assets/scenes/demo.json`. A complete minimal scene looks like this:

```json
{
  "version": 1,
  "entities": [
    {
      "id": "robot-01",
      "name": "Robot",
      "model": "models/robot.glb",
      "transform": {
        "position": [0, 0, 0],
        "rotation_degrees": [0, 0, 0],
        "scale": [1, 1, 1]
      }
    }
  ]
}
```

Rebuild to restage assets, then run. Alternatively, supply `--assets` pointing at
the source asset directory while iterating. Model paths use forward slashes and
are relative to the asset root. Use lowercase `.glb`, `.gltf`, or `.obj` extensions.
For multi-file assets, keep referenced buffers, material files, and textures at
the relative locations expected by the model. This template enables JPEG support
in addition to raylib's default PNG support.

Prefer an uncompressed GLB with embedded resources for the initial asset pipeline.
The bundled cube needs no external textures and uses face vertex colors, so it
is visible without a lighting shader. Its original generator is provided:

```sh
python tools/make_sample_model.py
```

The demo renderer uses raylib's default shader. Importing metallic/roughness maps
does not turn this into a PBR renderer. Custom lighting, material interpretation,
alpha handling, glTF extensions, and animated assets require a separate tested
rendering/animation implementation. A loaded raylib Model is not automatically
converted into a graph of ECS entities.

## Project layout

```text
CMakeLists.txt
CMakePresets.json
cmake/Dependencies.cmake
include/engine/Scene.hpp       # Components and scene API, no raylib dependency
include/engine/Assets.hpp      # Model owner, asset cache, render entry point
src/engine/Scene.cpp           # Validation, JSON, fixed update
src/engine/Assets.cpp          # Import, resource cleanup, rendering
src/main.cpp                   # Window lifetime, loop, camera, optional inspector
assets/models/cube.glb
assets/scenes/demo.json
tests/scene_tests.cpp          # Round-trip, validation, and timestep checks
tools/make_sample_model.py
```

`engine_scene` only links EnTT and JSON. `engine_render` adds raylib. `sandbox`
adds spdlog and the optional inspector. Strict warnings are applied to our code,
not forced onto dependency source files. For a larger project, move the inspector and camera
out of `main.cpp`; their current placement keeps the initial file count small.

## Ownership and extension boundaries

ECS components store model paths, not owning copies of raylib Model structures.
The cache owns each imported ModelAsset and keeps it alive until the cache is
destroyed. The destructor releases each imported texture ID once, skips raylib's
default white texture, and then calls UnloadModel. It intentionally does not own
the default shader. Do not mutate material pointers, share imported textures with
another owner, or unload a value returned by ModelAsset::native(). Custom shader
ownership must be implemented separately.

The window is constructed before the cache and inspector and destroyed after
them. Model loading, rendering, and resource destruction stay on the main thread.
The cache does not evict old assets or reread an already cached file on scene
reload. A hot-reload implementation must replace resources safely, not just call
LoadModel in a render loop. Canonical paths are checked against the asset root,
but referenced glTF/OBJ sidecar files are loaded by raylib; this is **not a sandbox
for untrusted model files**.

JSON is an explicit, versioned persistence format. Entities without an Identity
are treated as transient and are not saved. Add serializers when adding new
persistent components; unknown JSON fields are not preserved. Runtime EnTT IDs
are intentionally not stored. Stable identity strings must be unique within a
scene. Parent links, prefab references, and external references would need a
second-pass ID-to-entity resolution step.

The simulation runs at 60 Hz, separately from the render rate. It deliberately
drops excess catch-up time after a long stall. There is no render interpolation,
rollback, deterministic replay guarantee, or input action system yet. The save
utility writes a temporary file in the destination directory before renaming it;
it is a single-writer convenience, not an fsync-based durable storage system.

## Dependency selections

| Component | Selected revision |
| --- | --- |
| raylib | `6.0` |
| EnTT | `v3.15.0` |
| nlohmann/json | `v3.12.0` |
| spdlog | `v1.15.3` |
| Dear ImGui | `v1.92.7` |
| rlImGui | `Raylib_6_0` |
| Catch2 | `v3.8.1` |

These are selected revisions, not a claim that every dependency is the newest.
The rlImGui release explicitly pairs raylib 6.0 with ImGui 1.92.7. The template
uses release tags, rather than floating development branches. For a production
supply-chain lock, resolve Git tags to full commit hashes, use verified archive
hashes, and keep your own dependency cache or mirror. Test upgrades together.
Third-party source and binaries are fetched at configure time, not bundled here.

## Sensible next additions

Start with a lighting shader, input actions, viewport picking, and an ImGuizmo
transform gizmo. Add file or editor logging sinks when they become useful.
Add Jolt for 3D rigid-body physics when gameplay requires it, using a
fixed physics step and an explicit transform synchronization policy. Add Tracy
when profiling actual workloads.

## Upstream references

- raylib API: https://raw.githubusercontent.com/raysan5/raylib/6.0/src/raylib.h
- raylib loader and lifetime behavior: https://raw.githubusercontent.com/raysan5/raylib/6.0/src/rmodels.c
- raylib format configuration: https://raw.githubusercontent.com/raysan5/raylib/6.0/src/config.h
- Linux prerequisites: https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux
- rlImGui compatibility release: https://github.com/raylib-extras/rlImGui/releases/tag/Raylib_6_0
- EnTT: https://github.com/skypjack/entt
- JSON CMake integration: https://json.nlohmann.me/integration/cmake/
- CMake FetchContent: https://cmake.org/cmake/help/latest/module/FetchContent.html
- Jolt: https://github.com/jrouwe/JoltPhysics
- ImGuizmo: https://github.com/CedricGuillemet/ImGuizmo
- spdlog: https://github.com/gabime/spdlog
- Tracy: https://github.com/wolfpld/tracy
- Catch2: https://github.com/catchorg/Catch2
- Assimp: https://github.com/assimp/assimp

## License

Original template code and the generated cube are provided under the accompanying
MIT license. Third-party dependencies have their own licenses and notices, which
you must preserve as applicable when distributing their source or binaries.
