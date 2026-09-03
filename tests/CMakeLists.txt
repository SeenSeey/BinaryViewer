find_package(Qt5 5.15 REQUIRED COMPONENTS Test)

add_executable(BinaryConverterTests BinaryConverterTests.cpp)
target_link_libraries(BinaryConverterTests PRIVATE binary_viewer_core Qt5::Test)
add_test(NAME BinaryConverterTests COMMAND BinaryConverterTests)

add_executable(BinaryFileReaderTests BinaryFileReaderTests.cpp)
target_link_libraries(BinaryFileReaderTests PRIVATE binary_viewer_core Qt5::Test)
add_test(NAME BinaryFileReaderTests COMMAND BinaryFileReaderTests)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(BinaryConverterTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(BinaryFileReaderTests PRIVATE -Wall -Wextra -Wpedantic)
endif()
