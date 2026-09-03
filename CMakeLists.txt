cmake_minimum_required(VERSION 3.16)

project(BinaryViewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets)

add_library(binary_viewer_core
    src/core/BinaryConverter.cpp
    src/core/BinaryConverter.h
    src/core/BinaryFileReader.cpp
    src/core/BinaryFileReader.h
    src/core/Types.h
)
target_include_directories(binary_viewer_core PUBLIC src)
target_link_libraries(binary_viewer_core PUBLIC Qt5::Core)

add_executable(BinaryViewer
    src/main.cpp
    src/ui/HexView.cpp
    src/ui/HexView.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
)
target_link_libraries(BinaryViewer PRIVATE binary_viewer_core Qt5::Gui Qt5::Widgets)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(binary_viewer_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(BinaryViewer PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
