cmake_minimum_required(VERSION 3.08)

project(slight)
set(CMAKE_CXX_STANDARD 11)

option(SLIGHT_BUILD_TESTS "Build tests" ON)

##############################################################################
# sqlite3
##############################################################################
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
add_library(sqlite3 sqlite3/sqlite3.c)
target_compile_definitions(
    sqlite3
    PRIVATE
        SQLITE_OMIT_AUTHORIZATION
        SQLITE_OMIT_DEPRECATED
        SQLITE_OMIT_LOAD_EXTENSION
)
target_link_libraries(sqlite3 Threads::Threads)

##############################################################################
# Library
##############################################################################
add_library(slight
    src/slight.cpp
)

target_include_directories(slight
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/sqlite3
)
target_link_libraries(slight sqlite3)

# @todo update examples
add_executable(example example.cpp)
target_link_libraries(example slight)

##############################################################################
# Tests
##############################################################################
if (SLIGHT_BUILD_TESTS)
    enable_testing()

    # Download and unpack googletest at configure time
    configure_file(Modules/Googletest.cmake googletest-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
    if(result)
        message(FATAL_ERROR "CMake step for googletest failed: ${result}")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
    if(result)
        message(FATAL_ERROR "Build step for googletest failed: ${result}")
    endif()

    # Prevent overriding the parent project's compiler/linker
    # settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Add googletest directly to our build. This defines
    # the gtest and gtest_main targets.
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
        ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL)

    # The gtest/gtest_main targets carry header search path
    # dependencies automatically when using CMake 2.8.11 or
    # later. Otherwise we have to add them here ourselves.
    if (CMAKE_VERSION VERSION_LESS 2.8.11)
        include_directories("${gtest_SOURCE_DIR}/include")
    endif ()

    add_executable(tests tests.cpp)
    target_link_libraries(tests slight gtest_main)
    add_test(NAME tests COMMAND tests)
endif ()
