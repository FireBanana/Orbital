#include "cuda.h"
#include <iostream>

__global__ void do_something() {}

CudaSample::CudaSample()
{
    float *x;
    cudaMallocManaged(&x, 100 * sizeof(float));

    std::cout << "From cuda!";

    do_something<<<1, 1>>>();
}
