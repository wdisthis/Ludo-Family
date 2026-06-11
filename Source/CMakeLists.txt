cmake_minimum_required(VERSION 3.12)
project(LudoFamily CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure Raylib download and compilation options
include(FetchContent)
FetchContent_Declare(
    raylib
    URL ${CMAKE_CURRENT_SOURCE_DIR}/5.0.tar.gz
)

# Disable Raylib examples and tests to save build time
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RAUDIO ON CACHE BOOL "" FORCE) # Ensure audio is enabled
set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
set(SUPPORT_FILEFORMAT_SVG ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

# Source files for the native C++ game
set(SOURCES
    src/main.cpp
    src/piece.cpp
    src/player.cpp
    src/rules.cpp
    src/path.cpp
    src/ai.cpp
    src/audio.cpp
    src/ui.cpp
    src/board.cpp
    src/game.cpp
)

add_executable(LudoFamily ${SOURCES})

# Target linking
target_link_libraries(LudoFamily PRIVATE raylib)

if(WIN32)
    # Link required Windows subsystems
    target_link_libraries(LudoFamily PRIVATE winmm gdi32 opengl32)
    
    # Hide the cmd console window in release builds
    # For debugging, we can keep it open or comment it out
    # set_target_properties(LudoFamily PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Post-build action: Copy assets to the executable directory
add_custom_command(TARGET LudoFamily POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:LudoFamily>/assets
)
