cmake_minimum_required(VERSION 3.14)

project(slight)
set(CMAKE_CXX_STANDARD 11)

add_library(sqlite3 sqlite3/sqlite3.c)

add_library(slight
    src/Bind.cpp
    src/Database.cpp
    src/Result.cpp
    src/Sql.cpp
    src/stmt_ptr.cpp
)

target_include_directories(slight PUBLIC ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/sqlite3)
target_link_libraries(slight sqlite3)

add_executable(test example.cpp)
target_link_libraries(test slight)
target_include_directories(test PRIVATE ${PROJECT_SOURCE_DIR}/include)

