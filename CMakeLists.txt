cmake_minimum_required(VERSION 3.25)
project(CppGameTemplate VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SEED_BUILD_SANDBOX "Build the desktop 3D demo" ON)
option(SEED_WITH_EDITOR "Include the ImGui debug inspector" ON)
option(SEED_BUILD_TESTS "Build non-graphical scene tests" ON)

include(cmake/Dependencies.cmake)

function(seed_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# This library has no graphics/window dependency and can be tested headlessly.
add_library(engine_scene STATIC src/engine/Scene.cpp)
target_include_directories(engine_scene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(engine_scene PUBLIC EnTT::EnTT nlohmann_json::nlohmann_json)
target_compile_features(engine_scene PUBLIC cxx_std_20)
seed_warnings(engine_scene)

if(SEED_BUILD_SANDBOX)
    add_library(engine_render STATIC src/engine/Assets.cpp)
    target_link_libraries(engine_render PUBLIC engine_scene raylib)
    seed_warnings(engine_render)

    add_executable(sandbox src/main.cpp)
    target_link_libraries(sandbox PRIVATE engine_render)
    target_compile_definitions(sandbox PRIVATE SEED_WITH_EDITOR=$<BOOL:${SEED_WITH_EDITOR}>)
    if(SEED_WITH_EDITOR)
        target_link_libraries(sandbox PRIVATE seed_rlimgui)
    endif()
    set_target_properties(sandbox PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    seed_warnings(sandbox)

    # Always stage assets, even when only an asset changed and the executable did not relink.
    add_custom_target(seed_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:sandbox>/assets"
        VERBATIM
    )
    add_dependencies(sandbox seed_assets)
endif()

if(SEED_BUILD_TESTS)
    enable_testing()
    add_executable(scene_tests tests/scene_tests.cpp)
    target_link_libraries(scene_tests PRIVATE engine_scene)
    seed_warnings(scene_tests)
    add_test(NAME scene_roundtrip COMMAND scene_tests)
endif()
