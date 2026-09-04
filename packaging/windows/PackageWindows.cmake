cmake_minimum_required(VERSION 3.16)

foreach(required_variable IN ITEMS
    BINARY_VIEWER_EXECUTABLE
    WINDEPLOYQT_EXECUTABLE
    BUILD_CONFIG
    TARGET_POINTER_SIZE
    PROJECT_VERSION
    PACKAGE_OUTPUT_DIRECTORY
)
    if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} must be provided")
    endif()
endforeach()

if(NOT BUILD_CONFIG STREQUAL "Release")
    message(FATAL_ERROR
        "The portable package must be built from Release, not '${BUILD_CONFIG}'. "
        "Use: cmake --build <build-dir> --config Release --target package_windows"
    )
endif()

if(NOT TARGET_POINTER_SIZE EQUAL 8)
    message(FATAL_ERROR
        "The windows-x64 package requires a 64-bit build (pointer size: ${TARGET_POINTER_SIZE})"
    )
endif()

if(NOT EXISTS "${BINARY_VIEWER_EXECUTABLE}")
    message(FATAL_ERROR "Binary Viewer executable not found: ${BINARY_VIEWER_EXECUTABLE}")
endif()
if(NOT EXISTS "${WINDEPLOYQT_EXECUTABLE}")
    message(FATAL_ERROR
        "windeployqt was not found beside Qt's qmake: ${WINDEPLOYQT_EXECUTABLE}"
    )
endif()

get_filename_component(executable_name "${BINARY_VIEWER_EXECUTABLE}" NAME)
if(NOT executable_name STREQUAL "BinaryViewer.exe")
    message(FATAL_ERROR
        "Expected the Release executable to be named BinaryViewer.exe, got '${executable_name}'"
    )
endif()

get_filename_component(package_output_directory "${PACKAGE_OUTPUT_DIRECTORY}" ABSOLUTE)
set(package_basename "BinaryViewer-${PROJECT_VERSION}-windows-x64")
set(staging_directory "${package_output_directory}/${package_basename}")
set(archive_path "${package_output_directory}/${package_basename}.zip")

function(run_checked operation)
    execute_process(
        COMMAND ${ARGN}
        RESULT_VARIABLE operation_result
        OUTPUT_VARIABLE operation_output
        ERROR_VARIABLE operation_error
    )
    if(NOT operation_result EQUAL 0)
        message(FATAL_ERROR
            "${operation} failed with exit code ${operation_result}\n"
            "${operation_output}\n${operation_error}"
        )
    endif()
endfunction()

run_checked("Cleaning the staging directory"
    "${CMAKE_COMMAND}" -E remove_directory "${staging_directory}"
)
run_checked("Removing the previous archive"
    "${CMAKE_COMMAND}" -E remove -f "${archive_path}"
)
run_checked("Creating the staging directory"
    "${CMAKE_COMMAND}" -E make_directory "${staging_directory}"
)
run_checked("Staging BinaryViewer.exe"
    "${CMAKE_COMMAND}" -E copy
    "${BINARY_VIEWER_EXECUTABLE}"
    "${staging_directory}/BinaryViewer.exe"
)

run_checked("Deploying the Qt runtime"
    "${WINDEPLOYQT_EXECUTABLE}"
    --release
    --compiler-runtime
    --dir "${staging_directory}"
    "${staging_directory}/BinaryViewer.exe"
)

foreach(required_file IN ITEMS
    BinaryViewer.exe
    Qt5Core.dll
    Qt5Gui.dll
    Qt5Widgets.dll
    platforms/qwindows.dll
)
    if(NOT EXISTS "${staging_directory}/${required_file}")
        message(FATAL_ERROR
            "windeployqt did not produce required runtime file: ${required_file}"
        )
    endif()
endforeach()

file(GLOB_RECURSE deployed_dlls "${staging_directory}/*.dll")
foreach(deployed_dll IN LISTS deployed_dlls)
    get_filename_component(deployed_dll_name "${deployed_dll}" NAME)
    string(TOLOWER "${deployed_dll_name}" deployed_dll_name_lower)
    if(deployed_dll_name_lower MATCHES "^qt5.*d\\.dll$"
        OR deployed_dll_name_lower MATCHES "^q.*d\\.dll$")
        message(FATAL_ERROR
            "A debug Qt runtime was found in the Release package: ${deployed_dll_name}"
        )
    endif()
endforeach()

file(GLOB_RECURSE archive_entries
    LIST_DIRECTORIES FALSE
    RELATIVE "${staging_directory}"
    "${staging_directory}/*"
)
if(NOT archive_entries)
    message(FATAL_ERROR "The staging directory is empty: ${staging_directory}")
endif()
list(SORT archive_entries)

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar cf "${archive_path}"
            --format=zip
            --
            ${archive_entries}
    WORKING_DIRECTORY "${staging_directory}"
    RESULT_VARIABLE archive_result
    OUTPUT_VARIABLE archive_output
    ERROR_VARIABLE archive_error
)
if(NOT archive_result EQUAL 0)
    message(FATAL_ERROR
        "Creating the ZIP archive failed with exit code ${archive_result}\n"
        "${archive_output}\n${archive_error}"
    )
endif()
if(NOT EXISTS "${archive_path}")
    message(FATAL_ERROR "The package archive was not created: ${archive_path}")
endif()

file(SIZE "${archive_path}" archive_size)
if(archive_size EQUAL 0)
    message(FATAL_ERROR "The package archive is empty: ${archive_path}")
endif()

message(STATUS "Created Windows package: ${archive_path}")
