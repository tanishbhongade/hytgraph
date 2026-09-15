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

# Use the host GPU architecture by default when CUDA is enabled.
# Allow callers to override this with -DCMAKE_CUDA_ARCHITECTURES=...
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES OR
   CMAKE_CUDA_ARCHITECTURES STREQUAL "")
    set(CMAKE_CUDA_ARCHITECTURES native)
endif()

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
    src/transfer/hytm_cost_model.cpp
    src/scheduling/task_combiner.cpp
    src/scheduling/contribution_scheduler.cpp
    src/scheduling/hub_sort.cpp
    src/algorithms/pagerank.cpp
    src/algorithms/sssp.cpp
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

# ---------------------------------------------------------------------------
# Phase 12 — Vendored HyTGraph SEP-Graph + Groute
#
# The vendored tree at third_party/hytgraph_sep/ is read-only. Its own
# top-level CMakeLists.txt is the original SEP-Graph project file (builds
# hybrid_pr/bfs/sssp/cc samples against src/ and samples/ trees that we did
# not copy) and is intentionally NOT executed.
#
# The vendored SEP-Graph headers do require gflags headers. deps/gflags is a
# self-contained source subproject; we configure it here with its internal
# CTest registrations suppressed, so its 51 tests do not leak into our suite.
#
# All project-side access to the vendored library goes through
# include/sep_adapter/ (Phase 13). The only exception is the link smoke test
# in tests/integration/.
# ---------------------------------------------------------------------------
option(HYTGRAPH_WITH_SEP_GRAPH
       "Build with vendored HyTGraph SEP-Graph + Groute" ON)

if(HYTGRAPH_WITH_SEP_GRAPH)
    set(HYTGRAPH_SEP_ROOT
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/hytgraph_sep")

    if(NOT EXISTS "${HYTGRAPH_SEP_ROOT}/include/framework/algo_variants.cuh")
        message(FATAL_ERROR
            "HYTGRAPH_WITH_SEP_GRAPH=ON but the vendored SEP-Graph tree "
            "is missing or incomplete at ${HYTGRAPH_SEP_ROOT}. "
            "Expected ${HYTGRAPH_SEP_ROOT}/include/framework/algo_variants.cuh.")
    endif()

    # ---------------------------------------------------------------------
    # Vendored gflags
    #
    # Configure it as a static library, tests off. We only need its headers
    # and, once Phase 13 arrives, its symbols. BUILD_TESTING=OFF around the
    # add_subdirectory call is what suppresses gflags' internal ctest
    # registrations (unittest, version-1, helpxml, flagfile.*, ...).
    # ---------------------------------------------------------------------
    set(GFLAGS_BUILD_TESTING              OFF CACHE BOOL "" FORCE)
    set(GFLAGS_BUILD_gflags_LIB           ON  CACHE BOOL "" FORCE)
    set(GFLAGS_BUILD_gflags_nothreads_LIB OFF CACHE BOOL "" FORCE)
    set(GFLAGS_BUILD_STATIC_LIBS          ON  CACHE BOOL "" FORCE)
    set(GFLAGS_BUILD_SHARED_LIBS          OFF CACHE BOOL "" FORCE)
    set(GFLAGS_INSTALL_HEADERS            OFF CACHE BOOL "" FORCE)
    set(GFLAGS_INSTALL_SHARED_LIBS        OFF CACHE BOOL "" FORCE)
    set(GFLAGS_INSTALL_STATIC_LIBS        OFF CACHE BOOL "" FORCE)

    set(_HYTGRAPH_SAVED_BUILD_TESTING "${BUILD_TESTING}")
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

    add_subdirectory(
        "${HYTGRAPH_SEP_ROOT}/deps/gflags"
        "${CMAKE_CURRENT_BINARY_DIR}/hytgraph_sep_gflags_build"
        EXCLUDE_FROM_ALL)

    set(BUILD_TESTING "${_HYTGRAPH_SAVED_BUILD_TESTING}" CACHE BOOL "" FORCE)

    if(TARGET gflags_static)
        set(_HYTGRAPH_SEP_GFLAGS_TARGET gflags_static)
    elseif(TARGET gflags)
        set(_HYTGRAPH_SEP_GFLAGS_TARGET gflags)
    else()
        message(FATAL_ERROR
            "hytgraph_sep: vendored gflags did not define a "
            "'gflags' or 'gflags_static' target.")
    endif()

    # ---------------------------------------------------------------------
    # hytgraph_sep_lib
    #
    # INTERFACE. Header-only. Exposes:
    #   - vendored SEP-Graph / Groute include root
    #   - vendored nlohmann/json include root
    #   - generated gflags include dir (gflags.h is configure_file'd there)
    #
    # Links gflags so that a downstream TU (the smoke test) can resolve
    # gflags symbols if it ever needs them. In Phase 12 nothing does, so the
    # static archive is not actually pulled in at link time.
    # ---------------------------------------------------------------------
    add_library(hytgraph_sep_lib INTERFACE)

    target_include_directories(hytgraph_sep_lib SYSTEM INTERFACE
        "${HYTGRAPH_SEP_ROOT}/include"
        "${HYTGRAPH_SEP_ROOT}/deps/json"
        "${CMAKE_CURRENT_BINARY_DIR}/hytgraph_sep_gflags_build/include")

    target_link_libraries(hytgraph_sep_lib INTERFACE
        ${_HYTGRAPH_SEP_GFLAGS_TARGET})

    # The vendored CUB version does not match the CUDA toolkit's Thrust
    # expectation; the original SEP-Graph build silences that check.
    target_compile_definitions(hytgraph_sep_lib INTERFACE
        THRUST_IGNORE_CUB_VERSION_CHECK)

    message(STATUS
        "hytgraph_sep_lib: INTERFACE target over ${HYTGRAPH_SEP_ROOT}")

    # === PHASE 13 BEGIN ===
    # ---------------------------------------------------------------------
    # Phase 13 — sep_adapter sources
    #
    # The sep_adapter module is the single boundary between project code
    # and the vendored SEP-Graph + Groute tree. Only the .cpp files below
    # may include vendored headers; every public .hpp in
    # include/sep_adapter/ is vendored-symbol-free.
    #
    # sep_variant_mapper.cpp is the ONLY translation unit that includes
    # <framework/common.h>. It is compiled only when the vendored tree is
    # available, because the OFF build has no include path to it.
    #
    # sep_engine_adapter.cpp and sep_engine_factory.cpp are empty
    # placeholders in Phase 13 (the adapter and factory are header-only).
    # They are compiled so the compiler isolates-parses their headers,
    # and so the source list does not need to change when Phase 14 adds
    # real definitions.
    # ---------------------------------------------------------------------
    target_sources(hytgraph_runtime PRIVATE
        src/sep_adapter/sep_engine_adapter.cpp
        src/sep_adapter/sep_engine_factory.cpp
        src/sep_adapter/sep_variant_mapper.cpp
    )

    target_link_libraries(hytgraph_runtime PRIVATE hytgraph_sep_lib)
    # === PHASE 13 END ===

    # === PHASE 14 BEGIN ===
    # ---------------------------------------------------------------------
    # Phase 14 — data-movement bridge
    #
    # sep_host_graph_adapter.cpp (plain C++) serializes the project CSR
    # to a temp file in the "market_big" layout, the only format the
    # vendored ReadGraphMarket_bigdata parser actually implements.
    #
    # hytm_sep_bridge.cpp (plain C++) owns the bridge lifecycle and
    # delegates the CUDA-only engine work to detail::run_engine(), which
    # is defined in sep_engine_adapter_impl.cu.
    #
    # sep_engine_adapter_impl.cu is the ONLY Phase 14 TU that includes
    # <framework/framework.cuh>. It must be compiled by nvcc because the
    # vendored templates reach kernel-launch syntax.
    #
    # Private headers under src/sep_adapter/{detail,apps}/ are only
    # reachable from within this ON block, since only TUs added here
    # include them.
    # ---------------------------------------------------------------------
    target_include_directories(hytgraph_runtime PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_sources(hytgraph_runtime PRIVATE
        src/sep_adapter/sep_host_graph_adapter.cpp
    )

    if(CMAKE_CUDA_COMPILER)
        target_sources(hytgraph_runtime PRIVATE
            src/sep_adapter/hytm_sep_bridge.cpp
            src/sep_adapter/sep_engine_adapter_impl.cu
        )

        set_source_files_properties(
            src/sep_adapter/sep_engine_adapter_impl.cu
            PROPERTIES LANGUAGE CUDA
        )

        set_target_properties(hytgraph_runtime PROPERTIES
            CUDA_STANDARD 17
            CUDA_STANDARD_REQUIRED ON
        )

        # The vendored engine uses cudaMalloc / cudaMemcpy / cudaStream_t
        # throughout framework.cuh. CUDA::cudart carries those symbols.
        # PRIVATE is sufficient: for static libraries CMake propagates
        # PRIVATE link dependencies transitively at final link time.
        target_link_libraries(hytgraph_runtime PRIVATE CUDA::cudart)

        # The vendored CUB headers live under deps/cub and conflict with
        # the system CUB shipped in modern CUDA Toolkits. The macro is
        # already INTERFACE-defined on hytgraph_sep_lib, but nvcc does
        # not read INTERFACE defs from PRIVATE-linked libraries at
        # compile time of the dependent TU, so re-declare it here.
        target_compile_definitions(hytgraph_runtime PRIVATE
            THRUST_IGNORE_CUB_VERSION_CHECK
        )
    else()
        message(STATUS
            "Phase 14: CUDA compiler not available; HyTMSEPBridge and the "
            "vendored engine runner will not be built. Phase 14 tests are "
            "skipped.")
    endif()
    # === PHASE 14 END ===
endif()

# === PHASE 13 BEGIN ===
# Numeric macro so that sep_engine_factory.hpp's
#   #if defined(HYTGRAPH_WITH_SEP_GRAPH) && (HYTGRAPH_WITH_SEP_GRAPH == 0)
# works. CMake's option() yields ON/OFF; $<BOOL:...> converts to 1/0.
# PUBLIC so that every consumer of hytgraph_runtime (hytgraph_algorithms,
# all tests, hytgraph_dummy) sees the same value.
#
# Placed outside the if/endif above so that OFF builds also get the
# numeric definition (as 0).
# === PHASE 13 END ===
target_compile_definitions(hytgraph_runtime PUBLIC
    HYTGRAPH_WITH_SEP_GRAPH=$<BOOL:${HYTGRAPH_WITH_SEP_GRAPH}>
)

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

    # --- Phase 12 link smoke test ----------------------------------------
    # Compiled as PLAIN C++. The vendored header we exercise,
    # framework/common.h, is a pure-C++ header. We link hytgraph_sep_lib
    # (which transitively brings gflags and its headers) but we do not
    # need nvcc for this particular test.
    if(HYTGRAPH_WITH_SEP_GRAPH)

        add_executable(hytgraph_sep_link_smoke_test
            tests/integration/sep_graph_link_smoke_test.cpp
        )

        target_link_libraries(hytgraph_sep_link_smoke_test PRIVATE
            hytgraph_sep_lib
        )

        add_test(
            NAME sep_graph_link_smoke
            COMMAND hytgraph_sep_link_smoke_test
        )
    endif()

    # === PHASE 13 BEGIN ===
    # --- Phase 13 sep_adapter tests --------------------------------------
    # The adapter tests exercise the project-owned interface only. They
    # do not link hytgraph_sep_lib directly, and they do not include any
    # vendored header themselves. The vendored translation is exercised
    # indirectly through the factory, which in ON builds returns a
    # SEPEngineAdapter whose initialize()/execute_step() bodies are
    # header-inline.
    add_executable(hytgraph_sep_adapter_tests
        tests/sep_adapter_tests.cpp
    )

    target_link_libraries(hytgraph_sep_adapter_tests PRIVATE
        hytgraph_runtime
    )

    add_test(
        NAME sep_adapter_tests
        COMMAND hytgraph_sep_adapter_tests
    )
    # === PHASE 13 END ===

    # === PHASE 14 BEGIN ===
    # --- Phase 14 bridge tests -------------------------------------------
    # Compiled as PLAIN C++. The test drives HyTMSEPBridge, which is a
    # plain-C++ class. The CUDA-only work is delegated to a .cu file
    # inside hytgraph_runtime; the test executable only sees the
    # project-owned interface. CMake propagates the CUDA::cudart link
    # flags from hytgraph_runtime's PRIVATE dependency (CMake does this
    # for static libraries).
    if(HYTGRAPH_WITH_SEP_GRAPH AND CMAKE_CUDA_COMPILER)
        add_executable(hytgraph_sep_adapter_bridge_tests
            tests/sep_adapter_bridge_tests.cpp
        )

        target_link_libraries(hytgraph_sep_adapter_bridge_tests PRIVATE
            hytgraph_runtime
        )

        add_test(
            NAME sep_adapter_bridge_tests
            COMMAND hytgraph_sep_adapter_bridge_tests
        )
    endif()
    # === PHASE 14 END ===

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