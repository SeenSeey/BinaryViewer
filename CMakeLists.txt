cmake_minimum_required(VERSION 3.16)

project(BinaryViewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets)

function(binary_viewer_configure_compiler target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(binary_viewer_core
    src/core/BinaryConverter.cpp
    src/core/BinaryConverter.h
    src/core/BinaryFileReader.cpp
    src/core/BinaryFileReader.h
    src/core/Types.h
    src/core/ViewerSession.cpp
    src/core/ViewerSession.h
)
target_include_directories(binary_viewer_core PUBLIC src)
target_link_libraries(binary_viewer_core PUBLIC Qt5::Core)
binary_viewer_configure_compiler(binary_viewer_core)

add_library(binary_viewer_presentation
    src/presentation/BinaryTextFormatter.cpp
    src/presentation/BinaryTextFormatter.h
)
target_include_directories(binary_viewer_presentation PUBLIC src)
target_link_libraries(binary_viewer_presentation PUBLIC binary_viewer_core Qt5::Core)
binary_viewer_configure_compiler(binary_viewer_presentation)

add_executable(BinaryViewer
    src/main.cpp
    src/ui/HexView.cpp
    src/ui/HexView.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/MainWindowUi.cpp
    src/ui/MainWindowUi.h
    src/ui/UiEffects.cpp
    src/ui/UiEffects.h
    resources/resources.qrc
)
target_link_libraries(BinaryViewer PRIVATE binary_viewer_presentation Qt5::Gui Qt5::Widgets)
binary_viewer_configure_compiler(BinaryViewer)

if(WIN32)
    set_target_properties(BinaryViewer PROPERTIES WIN32_EXECUTABLE TRUE)
    set(binary_viewer_runtime_destination ".")
else()
    include(GNUInstallDirs)
    set(binary_viewer_runtime_destination "${CMAKE_INSTALL_BINDIR}")
endif()

install(TARGETS BinaryViewer
    RUNTIME DESTINATION "${binary_viewer_runtime_destination}"
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
