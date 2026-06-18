#include "cuda.h"
#include <cuda.h>
#include <cuda_runtime.h>
#include <iostream>

static const char *_cudaGetErrorEnum(curandStatus_t error)
{
    switch (error) {
    case CURAND_STATUS_SUCCESS:
        return "CURAND_STATUS_SUCCESS";

    case CURAND_STATUS_VERSION_MISMATCH:
        return "CURAND_STATUS_VERSION_MISMATCH";

    case CURAND_STATUS_NOT_INITIALIZED:
        return "CURAND_STATUS_NOT_INITIALIZED";

    case CURAND_STATUS_ALLOCATION_FAILED:
        return "CURAND_STATUS_ALLOCATION_FAILED";

    case CURAND_STATUS_TYPE_ERROR:
        return "CURAND_STATUS_TYPE_ERROR";

    case CURAND_STATUS_OUT_OF_RANGE:
        return "CURAND_STATUS_OUT_OF_RANGE";

    case CURAND_STATUS_LENGTH_NOT_MULTIPLE:
        return "CURAND_STATUS_LENGTH_NOT_MULTIPLE";

    case CURAND_STATUS_DOUBLE_PRECISION_REQUIRED:
        return "CURAND_STATUS_DOUBLE_PRECISION_REQUIRED";

    case CURAND_STATUS_LAUNCH_FAILURE:
        return "CURAND_STATUS_LAUNCH_FAILURE";

    case CURAND_STATUS_PREEXISTING_FAILURE:
        return "CURAND_STATUS_PREEXISTING_FAILURE";

    case CURAND_STATUS_INITIALIZATION_FAILED:
        return "CURAND_STATUS_INITIALIZATION_FAILED";

    case CURAND_STATUS_ARCH_MISMATCH:
        return "CURAND_STATUS_ARCH_MISMATCH";

    case CURAND_STATUS_INTERNAL_ERROR:
        return "CURAND_STATUS_INTERNAL_ERROR";
    }

    return "<unknown>";
}

template<typename T>
void check(T result, char const *const func, const char *const file, int const line)
{
    if (result) {
        fprintf(stderr,
                "CUDA error at %s:%d code=%d(%s) \"%s\" \n",
                file,
                line,
                static_cast<unsigned int>(result),
                _cudaGetErrorEnum(result),
                func);
        exit(EXIT_FAILURE);
    }
}

#define CE(val) check((val), #val, __FILE__, __LINE__)

CudaSample::CudaSample()
{
    int deviceCount = 0;
    int currentDevice = 0;
    int computeMode;
    cudaGetDeviceCount(&deviceCount);

    printf("CUDA::Found %d devices\n", deviceCount);

    CE(cudaDeviceGetAttribute(&computeMode, cudaDevAttrComputeMode, currentDevice));

    fflush(stdout);
}
