cmake_minimum_required(VERSION 3.22)
project(ecu_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../generated)
set(UNITY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/Unity/src)

add_library(unity STATIC ${UNITY_DIR}/unity.c)
target_include_directories(unity PUBLIC ${UNITY_DIR})

enable_testing()

add_executable(test_codec test_codec.c ${GENERATED_DIR}/vehicle_hil.c)
target_include_directories(test_codec PRIVATE ${GENERATED_DIR})
target_compile_options(test_codec PRIVATE -Wall -Wextra -Werror)
target_link_libraries(test_codec PRIVATE unity)
add_test(NAME test_codec COMMAND test_codec)
