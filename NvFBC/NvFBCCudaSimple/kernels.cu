#include <assert.h>
#include <cuda.h>
#include <cuda_runtime.h>

__global__ static void CudaProcess(int w, int h, float * pZBuffer1, float * pZBuffer2, unsigned char * pHostMappedResult)
{
    unsigned int i = blockIdx.x*blockDim.x + threadIdx.x;
    unsigned int j = blockIdx.y*blockDim.y + threadIdx.y; 
    float z1 = pZBuffer1[i+j*w];
    float z2 = pZBuffer2[i+j*w];
    
    // sample processing / image analysis
    if (z1<=z2)		            // pixels are BGRA, so .x is blue component
    {
		// ....
    }
    
    //...
    
    
 }

extern "C" 
{
	cudaError launch_CudaProcess(int w, int h, float * pZBuffer1, float * pZBuffer2, CUdeviceptr pHostMappedResult)
	{
		dim3 dimBlock(16, 16, 1);
		dim3 dimGrid(w/dimBlock.x, h/dimBlock.y, 1);

		CudaProcess<<<dimGrid, dimBlock>>>(w, h, pZBuffer1, pZBuffer2, (unsigned char *)pHostMappedResult);	
		cudaError err = cudaGetLastError();                                
		return err;
	}
}