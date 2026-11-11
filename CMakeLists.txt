cmake_minimum_required(VERSION 3.22)

project(MisPlugins VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE se descarga automaticamente. Para usar una copia local:
#   cmake -DFETCHCONTENT_SOURCE_DIR_JUCE=/ruta/a/JUCE ...
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.17-security
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(JUCE)

add_subdirectory(plugins/Calor)
