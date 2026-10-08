cmake_minimum_required(VERSION 3.16)
project(OpenGL_Lab4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
)
FetchContent_MakeAvailable(glad)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()

add_executable(OpenGL_Lab4
    src/main.cpp
    src/Shader.cpp
    src/Camera.cpp
    src/Texture.cpp
)

target_include_directories(OpenGL_Lab4 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GLAD_BINARY_DIR}/include
    ${glm_SOURCE_DIR}
    ${stb_SOURCE_DIR}
)

find_package(OpenGL REQUIRED)

target_link_libraries(OpenGL_Lab4 PRIVATE
    glfw
    glad
    OpenGL::GL
)
