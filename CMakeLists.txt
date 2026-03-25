cmake_minimum_required(VERSION 3.21)
project(fake_water LANGUAGES C CXX)

find_package(OpenGL REQUIRED)

# Setup paths
set(INCLUDE /usr/local/include)
set(LIB /usr/local/lib)
set(SRC /usr/local/src)
set(CODE "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB IMGUI_SRC "${SRC}/imgui/*.cpp")
file(GLOB IMGUI_BACKEND_SOURCES "${SRC}/imgui/backends/*.cpp")

file(GLOB_RECURSE LIB_CODE_SRC "${CODE}/*.cpp")
list(REMOVE_ITEM LIB_CODE_SRC "${CODE}/main.cpp")

add_library(depends STATIC
    "${SRC}/glad.c"
    "${SRC}/learnOpengl/shader.cpp"
    "${SRC}/learnOpengl/camera.cpp"
    "${SRC}/learnOpengl/texture.cpp"
    "${SRC}/learnOpengl/utils.cpp"
    ${LIB_CODE_SRC}
    ${IMGUI_SRC}
    ${IMGUI_BACKEND_SOURCES}
)

# 3. Setup includes (Folders only)
target_include_directories(depends PUBLIC 
    ${CODE}
    ${INCLUDE}
    "${INCLUDE}/imgui"
    "${INCLUDE}/imgui/backends"
)

target_compile_options(depends PRIVATE -Wall -Wextra -g)

# 4. Link dependencies
target_link_libraries(depends PUBLIC 
    OpenGL::GL
    "${LIB}/libglfw3.a"
    minizip z pthread
    X11 Xrandr Xi
)

# 5. Final Executable
add_executable(${PROJECT_NAME} "${CODE}/gerstner.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE depends)
