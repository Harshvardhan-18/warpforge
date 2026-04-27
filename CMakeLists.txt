cmake_minimum_required(VERSION 3.16)
project(MiniPTX VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Collect all source files
set(MINIPTX_SOURCES
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/sema.cpp
    src/ir.cpp
    src/passes/dce.cpp
    src/passes/const_fold.cpp
    src/passes/cse.cpp
    src/regalloc.cpp
    src/codegen.cpp
)

add_executable(miniptx ${MINIPTX_SOURCES})

target_include_directories(miniptx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Enable warnings
if(MSVC)
    target_compile_options(miniptx PRIVATE /W4)
else()
    target_compile_options(miniptx PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ═══════════════════════════════════════════════════════════════════
#  Validation target (requires CUDA Toolkit)
# ═══════════════════════════════════════════════════════════════════

# Paths
set(MINIPTX_EXE $<TARGET_FILE:miniptx>)
set(PTX_OUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/out)
set(TESTS_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Custom commands: compile .ptxc → .ptx using our compiler
add_custom_command(
    OUTPUT  ${PTX_OUT_DIR}/vec_add.ptx
    COMMAND ${MINIPTX_EXE} ${TESTS_DIR}/vec_add.ptxc ${PTX_OUT_DIR}/vec_add.ptx
    DEPENDS miniptx ${TESTS_DIR}/vec_add.ptxc
    COMMENT "Compiling vec_add.ptxc → vec_add.ptx"
)

add_custom_command(
    OUTPUT  ${PTX_OUT_DIR}/scalar_mul.ptx
    COMMAND ${MINIPTX_EXE} ${TESTS_DIR}/scalar_mul.ptxc ${PTX_OUT_DIR}/scalar_mul.ptx
    DEPENDS miniptx ${TESTS_DIR}/scalar_mul.ptxc
    COMMENT "Compiling scalar_mul.ptxc → scalar_mul.ptx"
)

# Custom target to compile all PTX files
add_custom_target(compile_ptx
    DEPENDS ${PTX_OUT_DIR}/vec_add.ptx ${PTX_OUT_DIR}/scalar_mul.ptx
)

# Try to find CUDA for the validation harness
find_package(CUDAToolkit QUIET)

if(CUDAToolkit_FOUND)
    enable_language(CUDA)
    message(STATUS "CUDA found: ${CUDAToolkit_VERSION} — validation target enabled")

    add_executable(validate_bin tests/validate.cu)
    target_link_libraries(validate_bin PRIVATE CUDA::cuda_driver)
    set_target_properties(validate_bin PROPERTIES
        CUDA_STANDARD 17
        CUDA_ARCHITECTURES "80"
        OUTPUT_NAME "validate"
    )

    # 'validate' target: compile PTX, build validator, then run it
    add_custom_target(validate
        COMMAND validate_bin
            ${PTX_OUT_DIR}/vec_add.ptx
            ${PTX_OUT_DIR}/scalar_mul.ptx
        DEPENDS validate_bin compile_ptx
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running MiniPTX validation suite"
    )
else()
    message(STATUS "CUDA not found — validation target disabled")
    message(STATUS "To enable, install CUDA Toolkit and set CMAKE_CUDA_COMPILER")

    # Provide a manual build script as fallback
    add_custom_target(validate
        COMMAND ${CMAKE_COMMAND} -E echo "=== Manual validation ==="
        COMMAND ${CMAKE_COMMAND} -E echo "1. Compile PTX:  miniptx tests/vec_add.ptxc out/vec_add.ptx"
        COMMAND ${CMAKE_COMMAND} -E echo "2. Compile PTX:  miniptx tests/scalar_mul.ptxc out/scalar_mul.ptx"
        COMMAND ${CMAKE_COMMAND} -E echo "3. Build:  nvcc -o validate tests/validate.cu -lcuda"
        COMMAND ${CMAKE_COMMAND} -E echo "4. Run:    validate out/vec_add.ptx out/scalar_mul.ptx"
        DEPENDS compile_ptx
        COMMENT "CUDA not available — printing manual steps"
    )
endif()
