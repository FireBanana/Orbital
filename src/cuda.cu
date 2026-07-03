#include "cuda.h"
#include "graphics.h"
#include <cuda.h>
#include <cuda_runtime.h>
#include <iostream>

CudaSample::CudaSample()
{
    int deviceCount = 0;
    int currentDevice = 0;
    int computeMode;
    float *someBuffer;

    cudaExternalMemory_t memBuffer;
    cudaExternalSemaphore_t cudaSem;

    cudaGetDeviceCount(&deviceCount);

    cudaDeviceGetAttribute(&computeMode, cudaDevAttrComputeMode, currentDevice);
    cudaSetDevice(currentDevice);

    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, currentDevice);

    printf("GPU Device %d: \"%s\" with compute capability %d.%d\n\n",
           currentDevice,
           prop.name,
           prop.major,
           prop.minor);

    BufferDescription desc{};
    desc.bufferSize = 1024;
    desc.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    auto buffer = makeBuffer(desc, nullptr);

    cudaExternalMemoryHandleDesc cudaExtMemHandleDesc{};
    cudaExtMemHandleDesc.type = cudaExternalMemoryHandleTypeOpaqueFd;
    //cudaExtMemHandleDesc.handle.fd = getVkImageMemHandle(VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR)
    cudaExtMemHandleDesc.size = desc.bufferSize;
    cudaImportExternalMemory(&memBuffer, &cudaExtMemHandleDesc);

    cudaExternalMemoryBufferDesc cudaExtMemBufferDesc{};
    cudaExtMemBufferDesc.offset = 0;
    cudaExtMemBufferDesc.size = desc.bufferSize;
    cudaExtMemBufferDesc.flags = 0;
    cudaExternalMemoryGetMappedBuffer((void **) &someBuffer, memBuffer, &cudaExtMemBufferDesc);

    cudaExternalSemaphoreHandleDesc semaphoreHandleDesc{};
    semaphoreHandleDesc.type = cudaExternalSemaphoreHandleTypeTimelineSemaphoreFd;
    //semaphoreHandleDesc.handle.fd = (int)(uintptr_t) getSemaphoreHandle;
    semaphoreHandleDesc.flags = 0;
    cudaImportExternalSemaphore(&cudaSem, &semaphoreHandleDesc);

    fflush(stdout);
}
