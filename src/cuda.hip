#include "cuda.h"
#include "graphics.h"
#include <hip/hip_runtime.h>
#include <iostream>

CudaSample::CudaSample()
{
    int deviceCount = 0;
    int currentDevice = 0;
    int computeMode;
    hipStream_t stream;
    hipGetDeviceCount(&deviceCount);

    hipSetDevice(0);
    hipStreamCreate(&stream);

    auto someMemory = makeBuffer({1024,
                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                       | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                       | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT},
                                  nullptr,
                                  true);

    printf("CUDA::Found %d devices\n", deviceCount);

    // Memory to hip
    hipExternalMemoryHandleDesc desc{};
    desc.size = 1024;
    desc.type = hipExternalMemoryHandleTypeOpaqueFd;
    
    VkMemoryGetFdInfoKHR

    fflush(stdout);
}
