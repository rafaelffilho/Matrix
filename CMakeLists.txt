cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(TgClient VERSION 0.1 LANGUAGES CXX)

add_subdirectory(include/td)

add_executable(tg_client src/main.cpp)
target_include_directories(tg_client PRIVATE include)
target_link_libraries(tg_client PRIVATE Td::TdJson)
set_property(TARGET tg_client PROPERTY CXX_STANDARD 14)
