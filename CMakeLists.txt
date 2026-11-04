cmake_minimum_required(VERSION 3.20)
project(vulkan_learning CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED COMPONENTS glslc)   # CHANGED: also find glslc
find_package(glfw3 REQUIRED)

# --- Shaders: compile each .vert/.frag to build/shaders/<name>.spv ---
set(SHADER_SOURCES
    ${CMAKE_SOURCE_DIR}/shaders/shader.vert
    ${CMAKE_SOURCE_DIR}/shaders/shader.frag
)
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)

foreach(SRC ${SHADER_SOURCES})
    get_filename_component(NAME ${SRC} NAME)
    set(SPV ${SHADER_OUT_DIR}/${NAME}.spv)
    add_custom_command(
        OUTPUT ${SPV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
        COMMAND Vulkan::glslc ${SRC} -o ${SPV}
        DEPENDS ${SRC}                      # recompile only when the .vert/.frag changes
        COMMENT "Compiling shader ${NAME}")
    list(APPEND SPV_FILES ${SPV})
endforeach()

add_custom_target(shaders DEPENDS ${SPV_FILES})

# --- App ---
add_executable(app
    src/main.cpp
    src/app.cpp
    src/device.cpp
    src/swapchain.cpp
    src/pipeline.cpp
    src/frame.cpp
    src/buffer.cpp
    src/descriptors.cpp
    src/texture.cpp
    src/model.cpp
)
target_link_libraries(app PRIVATE Vulkan::Vulkan glfw)
target_compile_options(app PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
add_dependencies(app shaders)                                   # build shaders first
target_compile_definitions(app PRIVATE SHADER_DIR="${SHADER_OUT_DIR}/" TEXTURE_DIR="${CMAKE_SOURCE_DIR}/textures/" MODEL_DIR="${CMAKE_SOURCE_DIR}/models/")  # so C++ can find them