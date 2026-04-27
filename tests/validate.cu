/*
 * validate.cu — CUDA Driver API validation harness for MiniPTX
 *
 * Loads PTX files emitted by the MiniPTX compiler, launches them on the GPU,
 * and compares results against CPU reference implementations.
 *
 * Compile: nvcc -o validate tests/validate.cu -lcuda
 * Run:     validate out/vec_add.ptx out/scalar_mul.ptx
 */

#include <cuda.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

// ═══════════════════════════════════════════════════════════════════
//  Error checking macro
// ═══════════════════════════════════════════════════════════════════

#define CU_CHECK(call)                                                      \
    do {                                                                    \
        CUresult err = (call);                                              \
        if (err != CUDA_SUCCESS) {                                          \
            const char* errStr = nullptr;                                   \
            cuGetErrorString(err, &errStr);                                 \
            fprintf(stderr, "CUDA Driver Error at %s:%d: %s (%d)\n",       \
                    __FILE__, __LINE__, errStr ? errStr : "unknown", err);  \
            exit(1);                                                        \
        }                                                                   \
    } while (0)

// ═══════════════════════════════════════════════════════════════════
//  Utility: read PTX file to string
// ═══════════════════════════════════════════════════════════════════

static std::string readFile(const char* path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        fprintf(stderr, "error: cannot open '%s'\n", path);
        exit(1);
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

// ═══════════════════════════════════════════════════════════════════
//  CUDA Driver API context
// ═══════════════════════════════════════════════════════════════════

struct CudaCtx {
    CUdevice  device;
    CUcontext context;

    void init() {
        CU_CHECK(cuInit(0));
        CU_CHECK(cuDeviceGet(&device, 0));

        char name[256];
        CU_CHECK(cuDeviceGetName(name, sizeof(name), device));
        printf("Device: %s\n", name);

        CU_CHECK(cuCtxCreate(&context, 0, device));
    }

    void destroy() {
        CU_CHECK(cuCtxDestroy(context));
    }
};

// ═══════════════════════════════════════════════════════════════════
//  Load a PTX module and get a kernel function
// ═══════════════════════════════════════════════════════════════════

struct PtxKernel {
    CUmodule   module;
    CUfunction function;

    void load(const std::string& ptxSource, const char* kernelName) {
        // Load PTX with JIT compilation logging
        CUjit_option options[] = {
            CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES,
            CU_JIT_ERROR_LOG_BUFFER,
            CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES,
            CU_JIT_INFO_LOG_BUFFER,
        };
        char errorLog[4096]  = {};
        char infoLog[4096]   = {};
        size_t errorLogSize  = sizeof(errorLog);
        size_t infoLogSize   = sizeof(infoLog);
        void* optionValues[] = {
            (void*)errorLogSize, errorLog,
            (void*)infoLogSize,  infoLog,
        };

        CUresult res = cuModuleLoadDataEx(&module, ptxSource.c_str(),
                                           4, options, optionValues);
        if (res != CUDA_SUCCESS) {
            fprintf(stderr, "PTX JIT compilation failed!\n");
            fprintf(stderr, "Error log:\n%s\n", errorLog);
            const char* errStr = nullptr;
            cuGetErrorString(res, &errStr);
            fprintf(stderr, "CUDA error: %s (%d)\n",
                    errStr ? errStr : "unknown", res);
            exit(1);
        }

        if (strlen(infoLog) > 0) {
            printf("PTX JIT info: %s\n", infoLog);
        }

        CU_CHECK(cuModuleGetFunction(&function, module, kernelName));
        printf("  Loaded kernel '%s' from PTX\n", kernelName);
    }

    void unload() {
        CU_CHECK(cuModuleUnload(module));
    }
};

// ═══════════════════════════════════════════════════════════════════
//  Test 1: vec_add — c[i] = a[i] + b[i]
// ═══════════════════════════════════════════════════════════════════

static bool testVecAdd(const char* ptxPath) {
    printf("\n=== Test: vec_add ===\n");
    printf("  PTX: %s\n", ptxPath);

    const int N = 1024;
    const int blockSize = 256;
    const int gridSize  = (N + blockSize - 1) / blockSize;

    // Host arrays
    std::vector<float> h_a(N), h_b(N), h_c(N), h_ref(N);

    srand((unsigned)time(nullptr));
    for (int i = 0; i < N; i++) {
        h_a[i] = (float)rand() / RAND_MAX * 100.0f;
        h_b[i] = (float)rand() / RAND_MAX * 100.0f;
        h_ref[i] = h_a[i] + h_b[i];  // CPU reference
    }

    // Read and load PTX
    std::string ptxSrc = readFile(ptxPath);
    PtxKernel kernel;
    kernel.load(ptxSrc, "vec_add");

    // Allocate device memory
    CUdeviceptr d_a, d_b, d_c;
    CU_CHECK(cuMemAlloc(&d_a, N * sizeof(float)));
    CU_CHECK(cuMemAlloc(&d_b, N * sizeof(float)));
    CU_CHECK(cuMemAlloc(&d_c, N * sizeof(float)));

    // Copy inputs to device
    CU_CHECK(cuMemcpyHtoD(d_a, h_a.data(), N * sizeof(float)));
    CU_CHECK(cuMemcpyHtoD(d_b, h_b.data(), N * sizeof(float)));
    CU_CHECK(cuMemsetD8(d_c, 0, N * sizeof(float)));

    // Launch kernel: vec_add(float* a, float* b, float* c, int n)
    int n = N;
    void* args[] = { &d_a, &d_b, &d_c, &n };
    CU_CHECK(cuLaunchKernel(kernel.function,
                             gridSize, 1, 1,    // grid
                             blockSize, 1, 1,   // block
                             0, nullptr,        // shared mem, stream
                             args, nullptr));
    CU_CHECK(cuCtxSynchronize());

    // Copy result back
    CU_CHECK(cuMemcpyDtoH(h_c.data(), d_c, N * sizeof(float)));

    // Compare
    float maxErr = 0.0f;
    for (int i = 0; i < N; i++) {
        float err = fabsf(h_c[i] - h_ref[i]);
        if (err > maxErr) maxErr = err;
    }

    // Cleanup
    CU_CHECK(cuMemFree(d_a));
    CU_CHECK(cuMemFree(d_b));
    CU_CHECK(cuMemFree(d_c));
    kernel.unload();

    bool pass = (maxErr < 1e-5f);
    printf("  Max error: %.8e\n", maxErr);
    printf("  Result: %s\n", pass ? "PASS ✓" : "FAIL ✗");
    return pass;
}

// ═══════════════════════════════════════════════════════════════════
//  Test 2: scalar_mul — out[i] = arr[i] * arr[i]
// ═══════════════════════════════════════════════════════════════════

static bool testScalarMul(const char* ptxPath) {
    printf("\n=== Test: scalar_mul ===\n");
    printf("  PTX: %s\n", ptxPath);

    const int N = 1024;
    const int blockSize = 256;
    const int gridSize  = (N + blockSize - 1) / blockSize;

    // Host arrays
    std::vector<float> h_arr(N), h_out(N), h_ref(N);

    srand((unsigned)time(nullptr) + 42);
    for (int i = 0; i < N; i++) {
        h_arr[i] = (float)rand() / RAND_MAX * 10.0f;
        h_ref[i] = h_arr[i] * h_arr[i];  // CPU reference: square
    }

    // Read and load PTX
    std::string ptxSrc = readFile(ptxPath);
    PtxKernel kernel;
    kernel.load(ptxSrc, "scalar_mul");

    // Allocate device memory
    CUdeviceptr d_arr, d_out;
    CU_CHECK(cuMemAlloc(&d_arr, N * sizeof(float)));
    CU_CHECK(cuMemAlloc(&d_out, N * sizeof(float)));

    // Copy inputs to device
    CU_CHECK(cuMemcpyHtoD(d_arr, h_arr.data(), N * sizeof(float)));
    CU_CHECK(cuMemsetD8(d_out, 0, N * sizeof(float)));

    // Launch kernel: scalar_mul(float* arr, float* out, int n)
    int n = N;
    void* args[] = { &d_arr, &d_out, &n };
    CU_CHECK(cuLaunchKernel(kernel.function,
                             gridSize, 1, 1,
                             blockSize, 1, 1,
                             0, nullptr,
                             args, nullptr));
    CU_CHECK(cuCtxSynchronize());

    // Copy result back
    CU_CHECK(cuMemcpyDtoH(h_out.data(), d_out, N * sizeof(float)));

    // Compare
    float maxErr = 0.0f;
    for (int i = 0; i < N; i++) {
        float err = fabsf(h_out[i] - h_ref[i]);
        if (err > maxErr) maxErr = err;
    }

    // Cleanup
    CU_CHECK(cuMemFree(d_arr));
    CU_CHECK(cuMemFree(d_out));
    kernel.unload();

    bool pass = (maxErr < 1e-5f);
    printf("  Max error: %.8e\n", maxErr);
    printf("  Result: %s\n", pass ? "PASS ✓" : "FAIL ✗");
    return pass;
}

// ═══════════════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════════════

int main(int argc, char* argv[]) {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║    MiniPTX Validation Harness (CUDA Driver)   ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");

    if (argc < 2) {
        fprintf(stderr,
            "usage: validate <vec_add.ptx> [scalar_mul.ptx]\n\n"
            "  Validates MiniPTX-generated PTX kernels by running\n"
            "  them on the GPU and comparing against CPU reference.\n");
        return 1;
    }

    // Initialize CUDA
    CudaCtx ctx;
    ctx.init();

    int passed = 0, total = 0;

    // Test 1: vec_add
    if (argc >= 2) {
        total++;
        if (testVecAdd(argv[1])) passed++;
    }

    // Test 2: scalar_mul
    if (argc >= 3) {
        total++;
        if (testScalarMul(argv[2])) passed++;
    }

    // Summary
    printf("\n════════════════════════════════════════════════\n");
    printf("Results: %d / %d tests passed\n", passed, total);
    printf("════════════════════════════════════════════════\n");

    ctx.destroy();
    return (passed == total) ? 0 : 1;
}
