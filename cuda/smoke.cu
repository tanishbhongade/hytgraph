#include <cstdio>

__global__ void hytgraph_cuda_smoke_kernel(int* value) {
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        *value = 42;
    }
}

int main() {
    int* device_value = nullptr;
    int host_value = 0;
    cudaMalloc(&device_value, sizeof(int));
    hytgraph_cuda_smoke_kernel<<<1, 1>>>(device_value);
    cudaMemcpy(&host_value, device_value, sizeof(int), cudaMemcpyDeviceToHost);
    cudaFree(device_value);

    std::printf("HyTGraph CUDA smoke result: %d\\n", host_value);
    return host_value == 42 ? 0 : 1;
}
