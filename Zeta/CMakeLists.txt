cmake_minimum_required(VERSION 3.10.0)
project(Zeta VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(DECORATION_XML "/usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml")
# Find Dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-protocols)
find_package(Vulkan REQUIRED)

#Generate xdg-shell glue code
execute_process(COMMAND wayland-scanner client-header 
    /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml 
    ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h)

execute_process(COMMAND wayland-scanner private-code 
    /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml 
    ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c)

# Generate decoration glue code (Separate calls!)
execute_process(COMMAND wayland-scanner client-header 
    ${DECORATION_XML} 
    ${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-unstable-v1-client-protocol.h)

execute_process(COMMAND wayland-scanner private-code 
    ${DECORATION_XML} 
    ${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-unstable-v1-protocol.c)

# --- 1. Define the library ---
add_library(Zeta STATIC 
    time.cpp 
    window.cpp 
    render.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c
    ${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-unstable-v1-protocol.c
)

# NOW you can set the definitions
target_compile_definitions(Zeta PUBLIC VK_USE_PLATFORM_WAYLAND_KHR)

set_target_properties(Zeta PROPERTIES DEBUG_POSTFIX "_d")

target_include_directories(Zeta 
PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Link everything
target_link_libraries(Zeta PUBLIC wayland-client Vulkan::Vulkan)

# --- 2. Generate the Config File from Template ---
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ZetaConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ZetaConfig.cmake"
    INSTALL_DESTINATION lib/cmake/Zeta
)

# --- 3. Install Rules ---
install(TARGETS Zeta
    EXPORT ZetaTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/Zeta DESTINATION include)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ZetaConfig.cmake"
    DESTINATION lib/cmake/Zeta
)

install(EXPORT ZetaTargets
    FILE ZetaTargets.cmake
    NAMESPACE Zeta::
    DESTINATION lib/cmake/Zeta
)