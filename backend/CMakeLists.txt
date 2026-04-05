cmake_minimum_required(VERSION 3.16)
project(graduate_match_backend CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Drogon CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
  src/app_bootstrap.cc
  src/app_routes.cc
  src/app_support.cc
  src/ai_insight_agent.cc
  src/main.cc
  src/routes/system_routes.cc
  src/routes/education_routes.cc
  src/routes/jobs_routes.cc
  src/routes/student_employment_routes.cc
  src/routes/matching_routes.cc
  src/routes/dashboard_insight_routes.cc
  src/routes/profile_routes.cc
)

target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)
