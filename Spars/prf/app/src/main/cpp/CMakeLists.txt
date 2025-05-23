#[[
Copyright 2022 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
]]
cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(prf-android)

# Integrate GameActivity, refer to
#     https://d.android.com/games/agdk/integrate-game-activity
# for the detailed instructions.
find_package(game-activity REQUIRED CONFIG)

# build vulkan app
get_filename_component(REPO_ROOT_DIR
    ${CMAKE_SOURCE_DIR}/../../../../..  ABSOLUTE)
set(COMMON_DIR ${REPO_ROOT_DIR}/common)

add_library(${CMAKE_PROJECT_NAME} SHARED
    VulkanMain.cpp
    ${COMMON_DIR}/vulkan_wrapper/vulkan_wrapper.cpp
    AndroidMain.cpp
    ${COMMON_DIR}/src/GameActivitySources.cpp
    engine2d/BufferManager.cpp
    engine2d/PipelineManager.cpp
    engine2d/ImageManager.cpp
    engine2d/GlyphManager.cpp
    engine2d/SamplerDescriptorManager.cpp
    engine2d/pipeline_helper.cpp
    engine2d/Engine2D.cpp
    engine2d/DrawTask.cpp
    engine2d/DrawResource.cpp
    engine2d/rects/Rect.cpp
    engine2d/circles/Circle.cpp
    engine2d/image/Image.cpp
    engine2d/text/Text.cpp
    engine2d/rrects/RRect.cpp
    engine2d/paint/Paint.cpp
    renderWorker/RenderWorkerPool.cpp
    renderWorker/RenderWorkerThread.cpp
    renderTree/Animation.cpp
    renderTree/AnimationsList.cpp
    renderTree/DrawCmd.cpp
    renderTree/RenderNode.cpp
    renderTree/HorLnrMovAnimation.cpp
    treeParser/TreeParser.cpp
    drawTaskContainer/DrawTaskList.cpp
    utils/DrawTaskPool.cpp
    utils/DrawResourceCollectorQueue.cpp)

include_directories(${COMMON_DIR}/vulkan_wrapper)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall \
                     -DVK_USE_PLATFORM_ANDROID_KHR")

add_subdirectory(freetype)
target_link_libraries(${CMAKE_PROJECT_NAME}
    game-activity::game-activity
    log
    android
    freetype)
