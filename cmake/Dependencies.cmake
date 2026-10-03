include(FetchContent)

# Selected releases, not floating main/master branches. Review these together on upgrade.
set(ENTT_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENTT_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)

FetchContent_Declare(entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.15.0
    GIT_SHALLOW TRUE
)
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(entt json)

if(SEED_BUILD_TESTS)
    FetchContent_Declare(catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.8.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(catch2)
endif()

if(SEED_BUILD_SANDBOX)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
    # The sandbox uses EndDrawing for presentation, timing and input polling.
    set(SUPPORT_CUSTOM_FRAME_CONTROL OFF CACHE BOOL "" FORCE)
    # glTF commonly uses JPEG as well as PNG. JPEG is not on in raylib's default config.
    set(SUPPORT_FILEFORMAT_JPG ON CACHE BOOL "" FORCE)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # A deliberately explicit X11 desktop build; do not require both window systems.
        set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    endif()
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 6.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)
    # Treat upstream headers (including raymath inline functions) as external.
    # Keep strict diagnostics enabled for the engine sources that include them.
    set_property(TARGET raylib PROPERTY SYSTEM TRUE)
    # GCC diagnoses an unchecked fread in raylib's bundled MOD decoder at -O3.
    # Scope this upstream-only diagnostic exception to the audio translation unit.
    set_property(SOURCE ${raylib_SOURCE_DIR}/src/raudio.c TARGET_DIRECTORY raylib
        APPEND PROPERTY COMPILE_OPTIONS $<$<C_COMPILER_ID:GNU>:-Wno-unused-result>)

    if(SEED_WITH_EDITOR)
        # rlImGui documents this raylib/ImGui compatibility pairing.
        # SOURCE_SUBDIR prevents any upstream build system from being added implicitly.
        FetchContent_Declare(imgui_source
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.92.7
            GIT_SHALLOW TRUE
            SOURCE_SUBDIR _seed_no_upstream_cmake
        )
        FetchContent_Declare(rlimgui_source
            GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
            GIT_TAG Raylib_6_0
            GIT_SHALLOW TRUE
            GIT_SUBMODULES ""
            SOURCE_SUBDIR _seed_no_upstream_cmake
        )
        FetchContent_MakeAvailable(imgui_source rlimgui_source)
        add_library(seed_imgui STATIC
            ${imgui_source_SOURCE_DIR}/imgui.cpp
            ${imgui_source_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_source_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_source_SOURCE_DIR}/imgui_widgets.cpp
        )
        target_include_directories(seed_imgui SYSTEM PUBLIC ${imgui_source_SOURCE_DIR})
        target_compile_features(seed_imgui PUBLIC cxx_std_20)
        add_library(seed_rlimgui STATIC ${rlimgui_source_SOURCE_DIR}/rlImGui.cpp)
        target_include_directories(seed_rlimgui SYSTEM PUBLIC ${rlimgui_source_SOURCE_DIR})
        target_compile_definitions(seed_rlimgui PRIVATE NO_FONT_AWESOME)
        target_link_libraries(seed_rlimgui PUBLIC seed_imgui raylib)
    endif()
endif()
