find_package(Qt5 5.15 REQUIRED COMPONENTS Test)

add_executable(BinaryConverterTests BinaryConverterTests.cpp)
target_link_libraries(BinaryConverterTests PRIVATE binary_viewer_core Qt5::Test)
add_test(NAME BinaryConverterTests COMMAND BinaryConverterTests)

add_executable(BinaryFileReaderTests BinaryFileReaderTests.cpp)
target_link_libraries(BinaryFileReaderTests PRIVATE binary_viewer_core Qt5::Test)
add_test(NAME BinaryFileReaderTests COMMAND BinaryFileReaderTests)

add_executable(ViewerSessionTests ViewerSessionTests.cpp)
target_link_libraries(ViewerSessionTests PRIVATE binary_viewer_core Qt5::Test)
add_test(NAME ViewerSessionTests COMMAND ViewerSessionTests)

add_executable(BinaryTextFormatterTests BinaryTextFormatterTests.cpp)
target_link_libraries(BinaryTextFormatterTests PRIVATE binary_viewer_presentation Qt5::Test)
add_test(NAME BinaryTextFormatterTests COMMAND BinaryTextFormatterTests)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(BinaryConverterTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(BinaryFileReaderTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ViewerSessionTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(BinaryTextFormatterTests PRIVATE -Wall -Wextra -Wpedantic)
endif()
