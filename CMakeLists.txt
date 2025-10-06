cmake_minimum_required(VERSION 3.29)
project(price_time_priority)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# Optimization flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

# Google Test
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Generic typed tests (работают с любой реализацией)
add_executable(generic_engine_tests
        Tests/GenericEngineTests.cpp
        EngineConcept/Order.h
        EngineConcept/MatchingEngineConcept.h
        EnginImpl/V1/MatchingEngineV1.h
        EngineTestTypes.h
        EnginImpl/V2/MatchingEngineV2.h
        EnginImpl/V2_prealloc/MatchingEngineV2_prealloc.h
)

target_link_libraries(generic_engine_tests
        PRIVATE
        GTest::gtest
        GTest::gtest_main
)

# Performance benchmarks
add_executable(performance_benchmarks
        EngineConcept/Order.h
        Tests/GenericPerformanceTests.cpp
)

target_link_libraries(performance_benchmarks
        PRIVATE
        GTest::gtest
        GTest::gtest_main
)

add_executable(baseline_benchmark
        main.cpp
        EngineConcept/Order.h
        MatchingEngineConcept.h
        EngineTestTypes.h
        EnginImpl/V3/MatchingEngineV3.h
)

target_link_libraries(baseline_benchmark PRIVATE)

# Обнаружение тестов
include(GoogleTest)
gtest_discover_tests(generic_engine_tests)
gtest_discover_tests(performance_benchmarks)