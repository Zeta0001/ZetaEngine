cmake_minimum_required(VERSION 3.10.0)


project(Iota VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Zeta REQUIRED)

add_executable(Iota 
main.cpp
app.cpp
)

set_target_properties(Iota PROPERTIES OUTPUT_NAME_RELEASE "Iota")
#set_target_properties(Iota PROPERTIES OUTPUT_NAME_DEBUG "Iota_d")


target_link_libraries(Iota PRIVATE Zeta::Zeta)