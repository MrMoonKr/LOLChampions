cmake_minimum_required( VERSION 3.23 )

set( VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "" )
set( CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" )

project( LOLChampions 
    VERSION 1.0
    DESCRIPTION "A c++ sample program to show the champions of League of Legends"
    LANGUAGES CXX
)

set( CMAKE_CXX_STANDARD 17 )

find_package( fmt CONFIG REQUIRED )
find_package( httplib CONFIG REQUIRED )
find_package( CURL REQUIRED )
find_package( nlohmann_json CONFIG REQUIRED )
find_package( glfw3 CONFIG REQUIRED )
find_package( imgui CONFIG REQUIRED )
find_package( Stb REQUIRED )

add_executable( LOLChampions )
target_sources( LOLChampions 
    PRIVATE ./src/DataManager.h
    PRIVATE ./src/DataManager.cpp
    PRIVATE ./src/GuiManager.h
    PRIVATE ./src/GuiManager.cpp
    PRIVATE ./src/main.cpp 
)

target_link_libraries( LOLChampions 
    PRIVATE fmt::fmt
    PRIVATE httplib::httplib
    PRIVATE CURL::libcurl
    PRIVATE nlohmann_json nlohmann_json::nlohmann_json
    PRIVATE glfw
    PRIVATE opengl32
    PRIVATE imgui::imgui
)

file( COPY ./data DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )

