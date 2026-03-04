cmake_minimum_required(VERSION 3.16)
project(graduate_match_backend CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Drogon CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
  src/main.cc
)

target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)
