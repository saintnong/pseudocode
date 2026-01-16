cmake_minimum_required(VERSION 3.15)
project(scsa LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(scsa ${SOURCES})

if(MSVC)
    target_compile_options(scsa PRIVATE /W4)
else()
    target_compile_options(scsa PRIVATE -Wall -Wextra -Werror)
endif()

add_test(
    NAME integration_tests
    COMMAND python tester.py $<TARGET_FILE:scsa>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

# clang-format
find_program(CLANG_FORMAT_EXE clang-format)
if(CLANG_FORMAT_EXE)
    # Target: cmake --build build --target format
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${SOURCES}
        COMMENT "Formatting source files with clang-format..."
    )

    # Target: cmake --build build --target format-check
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run -Werror ${SOURCES}
        COMMENT "Checking format style..."
    )
else()
    message(WARNING "clang-format not found. Format targets will not be available.")
endif()