cmake_minimum_required(VERSION 3.24)

project(HyTGraph
    VERSION 0.1.0
    DESCRIPTION "Incremental reproduction infrastructure for HyTGraph"
    LANGUAGES CXX
)

include(CTest)
include(CheckLanguage)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HYTGRAPH_ENABLE_WARNINGS "Enable compiler warnings" ON)
option(HYTGRAPH_BUILD_TESTS "Build CTest-based tests" ON)
option(HYTGRAPH_ENABLE_CUDA "Build the CUDA target when nvcc is available" ON)

add_library(hytgraph_runtime
    src/runtime/config.cpp
    src/runtime/logging.cpp
    src/runtime/result.cpp
    src/graph/csr_graph.cpp
    src/graph/graph_loader.cpp
)
target_include_directories(hytgraph_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(hytgraph_runtime PUBLIC
    HYTGRAPH_PROJECT_VERSION=\"${PROJECT_VERSION}\"
)

if(HYTGRAPH_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(hytgraph_runtime PRIVATE /W4)
    else()
        target_compile_options(hytgraph_runtime PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

add_executable(hytgraph_dummy
    src/main.cpp
)
target_link_libraries(hytgraph_dummy PRIVATE hytgraph_runtime)

# The CUDA target is intentionally present even on machines without nvcc.
# When CUDA is available it builds a real smoke-test executable; otherwise it
# remains a buildable placeholder so CI can still validate the project shape.
if(HYTGRAPH_ENABLE_CUDA)
    check_language(CUDA)
    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        add_executable(hytgraph_cuda
            cuda/smoke.cu
        )
        set_target_properties(hytgraph_cuda PROPERTIES
            CUDA_STANDARD 17
            CUDA_STANDARD_REQUIRED ON
        )
    else()
        message(STATUS "CUDA compiler not found; hytgraph_cuda will be a placeholder target")
        add_custom_target(hytgraph_cuda
            COMMAND ${CMAKE_COMMAND} -E echo "HyTGraph CUDA target: nvcc not available; CUDA compilation skipped."
            VERBATIM
        )
    endif()
endif()

if(HYTGRAPH_BUILD_TESTS)
    enable_testing()

    add_executable(hytgraph_unit_tests
        tests/unit_tests.cpp
    )
    target_link_libraries(hytgraph_unit_tests PRIVATE hytgraph_runtime)
    add_test(NAME unit_tests COMMAND hytgraph_unit_tests)

    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(
        NAME experiment_runner_smoke
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/experiments/run_experiment.py
            --config ${CMAKE_CURRENT_SOURCE_DIR}/experiments/configs/dummy.yaml
            --output ${CMAKE_CURRENT_BINARY_DIR}/dummy_result.json
            --executable $<TARGET_FILE:hytgraph_dummy>
    )
endif()

install(TARGETS hytgraph_dummy hytgraph_runtime
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
