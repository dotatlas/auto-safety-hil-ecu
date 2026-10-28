cmake_minimum_required(VERSION 3.22)
project(ecu_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../app)
set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../generated)
set(UNITY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/Unity/src)

set(STRICT_FLAGS -Wall -Wextra -Werror -Wconversion -Wdouble-promotion)

add_library(unity STATIC ${UNITY_DIR}/unity.c)
target_include_directories(unity PUBLIC ${UNITY_DIR})

# The SIL library. Python loads it with ctypes.
add_library(ecu_core SHARED ${APP_DIR}/core.c)
target_include_directories(ecu_core PUBLIC ${APP_DIR})
target_compile_options(ecu_core PRIVATE ${STRICT_FLAGS})
set_target_properties(ecu_core PROPERTIES PREFIX "")
if(WIN32)
    target_link_options(ecu_core PRIVATE -static-libgcc)
endif()

enable_testing()

add_executable(test_core test_core.c ${APP_DIR}/core.c)
target_include_directories(test_core PRIVATE ${APP_DIR})
target_compile_options(test_core PRIVATE ${STRICT_FLAGS})
target_link_libraries(test_core PRIVATE unity)
add_test(NAME test_core COMMAND test_core)

add_executable(test_codec test_codec.c ${GENERATED_DIR}/vehicle_hil.c)
target_include_directories(test_codec PRIVATE ${GENERATED_DIR})
target_compile_options(test_codec PRIVATE -Wall -Wextra -Werror)
target_link_libraries(test_codec PRIVATE unity)
add_test(NAME test_codec COMMAND test_codec)
