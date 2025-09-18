cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)

project(rsl-log CXX)
add_library(rsl-log SHARED)

target_compile_options(rsl-log PUBLIC 
  "-freflection-latest"
  "-ftime-trace"
  "-Wconsumed"
  # "-fconstexpr-steps=10000000"
)
target_include_directories(rsl-log PUBLIC 
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

find_package(rsl-util REQUIRED)
target_link_libraries(rsl-log PUBLIC rsl::util)

add_subdirectory(src)

install(TARGETS rsl-log)
install(DIRECTORY include/ DESTINATION include)

option(BUILD_TESTING "Enable tests" ON)
option(ENABLE_COVERAGE "Enable coverage instrumentation" OFF)

if (BUILD_TESTING)
  message(STATUS "Building unit tests")

  add_executable(rsl-log-test)
  add_subdirectory(test)

  find_package(rsl-test REQUIRED)
  target_link_libraries(rsl-log-test PRIVATE rsl-log)
  target_link_libraries(rsl-log-test PRIVATE rsl::test rsl::test_main)

  # target_link_libraries(rsl-log PUBLIC rsl::test_cov)

endif()

if (BUILD_EXAMPLES)
  add_subdirectory(example)
endif()