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

    find_program(binary_viewer_powershell
        NAMES powershell.exe powershell
    )
    if(NOT binary_viewer_powershell)
        message(FATAL_ERROR "PowerShell is required to generate the Windows application icon")
    endif()

    set(binary_viewer_generated_directory "${CMAKE_CURRENT_BINARY_DIR}/generated/windows")
    set(binary_viewer_windows_icon "${binary_viewer_generated_directory}/BinaryViewer.ico")
    set(binary_viewer_windows_resource "${binary_viewer_generated_directory}/BinaryViewer.rc")

    add_custom_command(
        OUTPUT "${binary_viewer_windows_icon}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${binary_viewer_generated_directory}"
        COMMAND "${binary_viewer_powershell}"
            -NoProfile
            -NonInteractive
            -ExecutionPolicy Bypass
            -File "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/GenerateIcon.ps1"
            -OutputPath "${binary_viewer_windows_icon}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/GenerateIcon.ps1"
        COMMENT "Generating the Windows icon from the Binary Viewer brand symbol"
        VERBATIM
    )

    set(BINARY_VIEWER_WINDOWS_ICON "${binary_viewer_windows_icon}")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/BinaryViewer.rc.in"
        "${binary_viewer_windows_resource}"
        @ONLY
    )
    set_source_files_properties(
        "${binary_viewer_windows_icon}"
        PROPERTIES GENERATED TRUE HEADER_FILE_ONLY TRUE
    )
    add_custom_target(binary_viewer_windows_icon
        DEPENDS "${binary_viewer_windows_icon}"
    )
    add_dependencies(BinaryViewer binary_viewer_windows_icon)
    set_property(
        SOURCE "${binary_viewer_windows_resource}"
        APPEND PROPERTY OBJECT_DEPENDS "${binary_viewer_windows_icon}"
    )
    target_sources(BinaryViewer PRIVATE
        "${binary_viewer_windows_resource}"
        "${binary_viewer_windows_icon}"
    )

    set(binary_viewer_runtime_destination ".")
else()
    include(GNUInstallDirs)
    set(binary_viewer_runtime_destination "${CMAKE_INSTALL_BINDIR}")
endif()

install(TARGETS BinaryViewer
    RUNTIME DESTINATION "${binary_viewer_runtime_destination}"
)

if(WIN32)
    add_custom_target(package_windows
        COMMAND "${CMAKE_COMMAND}"
            "-DBINARY_VIEWER_EXECUTABLE=$<TARGET_FILE:BinaryViewer>"
            "-DWINDEPLOYQT_EXECUTABLE=$<TARGET_FILE_DIR:Qt5::qmake>/windeployqt.exe"
            "-DBUILD_CONFIG=$<CONFIG>"
            "-DTARGET_POINTER_SIZE=${CMAKE_SIZEOF_VOID_P}"
            "-DPROJECT_VERSION=${PROJECT_VERSION}"
            "-DPACKAGE_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/package/windows"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/PackageWindows.cmake"
        DEPENDS BinaryViewer
        COMMENT "Creating the Binary Viewer portable Windows package"
        USES_TERMINAL
        VERBATIM
    )
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
