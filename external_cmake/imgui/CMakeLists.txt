cmake_minimum_required(VERSION 3.28)
project(imgui)

# Set compiler options
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
file(GLOB IMGUI_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/*opengl*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/*opengl*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/*sdl2*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/*sdl2*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/cpp/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/cpp/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/fonts/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/cpp/*.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/misc/freetype/*.h
    # ${CMAKE_CURRENT_SOURCE_DIR}/misc/freetype/*.cpp
)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

# Add the library target
add_library(${PROJECT_NAME} STATIC ${IMGUI_SRC_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
)
