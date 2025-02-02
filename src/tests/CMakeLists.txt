cmake_minimum_required(VERSION 3.28)

project(CipherSafeTests)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB CIPHERSAFE_TEST_HEADER_FILES ${SRC_DIR}/*.h)
file(GLOB CIPHERSAFE_TEST_SOURCE_FILES ${SRC_DIR}/*.cpp)
file(GLOB CIPHERSAFE_HEADER_FILES ${SRC_DIR}/../*.h)

file(GLOB CIPHERSAFE_SOURCE_FILES ${SRC_DIR}/../*.cpp)
list(FILTER CIPHERSAFE_SOURCE_FILES EXCLUDE REGEX "main\\.cpp$")

# Combine all files into one variable
set(SRC_FILES 
    ${CIPHERSAFE_TEST_HEADER_FILES} 
    ${CIPHERSAFE_TEST_SOURCE_FILES} 
    ${CIPHERSAFE_HEADER_FILES}
    ${CIPHERSAFE_SOURCE_FILES}
    ${DOCTEST_FILES}
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_DIR}/../../lib/doctest)

# Set compiler options for the target
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall    # enable all warnings
    #-g       # include debug info in the executable.
    -Wformat # enables warnings related to the use of format strings.

)

# Include directories for ImGui
set(IMGUI_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/imgui)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${IMGUI_LIB_SRC}
    ${IMGUI_LIB_SRC}/backends
    ${IMGUI_LIB_SRC}/misc
    ${IMGUI_LIB_SRC}/misc/cpp
    ${IMGUI_LIB_SRC}/fonts
    ${IMGUI_LIB_SRC}/freetype
    ${IMGUI_LIB_SRC}/single_file
)

find_package(SQLite3 REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenSSL REQUIRED)

# Link against libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/imgui/build/libimgui.a
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${SQLite3_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${LIBSODIUM_LIBRARIES}
)

