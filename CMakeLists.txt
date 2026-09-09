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
    src/graph/activity_tracker.cpp
    src/graph/partition.cpp
    src/transfer/filter_engine.cpp
    src/transfer/compaction_engine.cpp
    src/transfer/zero_copy_engine.cpp
)

target_include_directories(hytgraph_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(hytgraph_runtime PUBLIC
    HYTGRAPH_PROJECT_VERSION="${PROJECT_VERSION}"
)

if(HYTGRAPH_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(hytgraph_runtime PRIVATE /W4)
    else()
        target_compile_options(hytgraph_runtime PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

add_library(hytgraph_algorithms
    src/algorithms/pagerank.cpp
    src/algorithms/sssp.cpp
)

target_include_directories(hytgraph_algorithms PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hytgraph_algorithms PUBLIC
    hytgraph_runtime
)

if(HYTGRAPH_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(hytgraph_algorithms PRIVATE /W4)
    else()
        target_compile_options(hytgraph_algorithms PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

# The CUDA target is intentionally present even on machines without nvcc.
# When CUDA is available it builds the real baseline GPU implementations;
# otherwise it keeps a placeholder target so CPU-only environments can still
# validate the project shape.
if(HYTGRAPH_ENABLE_CUDA)
    check_language(CUDA)
    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        find_package(CUDAToolkit REQUIRED)

        target_sources(hytgraph_algorithms PRIVATE
            cuda/pagerank.cu
            cuda/sssp.cu
        )

        target_link_libraries(hytgraph_algorithms PUBLIC CUDA::cudart)

        set_target_properties(hytgraph_algorithms PROPERTIES
            CUDA_STANDARD 17
            CUDA_STANDARD_REQUIRED ON
        )

        add_executable(hytgraph_cuda
            cuda/smoke.cu
        )

        set_target_properties(hytgraph_cuda PROPERTIES
            CUDA_STANDARD 17
            CUDA_STANDARD_REQUIRED ON
        )

        target_link_libraries(hytgraph_cuda PRIVATE CUDA::cudart)
    else()
        message(STATUS "CUDA compiler not found; GPU implementations will be unavailable")

        target_sources(hytgraph_algorithms PRIVATE
            src/algorithms/gpu_stub.cpp
        )

        add_custom_target(hytgraph_cuda
            COMMAND ${CMAKE_COMMAND} -E echo "HyTGraph CUDA target: nvcc not available; CUDA compilation skipped."
            VERBATIM
        )
    endif()
else()
    target_sources(hytgraph_algorithms PRIVATE
        src/algorithms/gpu_stub.cpp
    )
endif()

add_executable(hytgraph_dummy
    src/main.cpp
)

target_link_libraries(hytgraph_dummy PRIVATE
    hytgraph_runtime
)

if(HYTGRAPH_BUILD_TESTS)
    enable_testing()

    add_executable(hytgraph_unit_tests
        tests/unit_tests.cpp
    )

    target_link_libraries(hytgraph_unit_tests PRIVATE
        hytgraph_runtime
    )

    add_test(
        NAME unit_tests
        COMMAND hytgraph_unit_tests
    )

    add_executable(hytgraph_algorithm_tests
        tests/algorithm_tests.cpp
    )

    target_link_libraries(hytgraph_algorithm_tests PRIVATE
        hytgraph_algorithms
    )

    add_test(
        NAME algorithm_tests
        COMMAND hytgraph_algorithm_tests
    )

    if(HYTGRAPH_ENABLE_CUDA AND CMAKE_CUDA_COMPILER)
        add_executable(hytgraph_cuda_algorithm_tests
            tests/cuda_algorithm_tests.cu
        )

        set_target_properties(hytgraph_cuda_algorithm_tests PROPERTIES
            CUDA_STANDARD 17
            CUDA_STANDARD_REQUIRED ON
        )

        target_link_libraries(hytgraph_cuda_algorithm_tests PRIVATE
            hytgraph_algorithms
            CUDA::cudart
        )

        add_test(
            NAME cuda_algorithm_tests
            COMMAND hytgraph_cuda_algorithm_tests
        )

        set_tests_properties(
            cuda_algorithm_tests
            PROPERTIES SKIP_RETURN_CODE 77
        )
    endif()

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

install(
    TARGETS
        hytgraph_dummy
        hytgraph_runtime
        hytgraph_algorithms
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)