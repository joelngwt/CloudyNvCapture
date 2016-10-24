#include <assert.h>
#include <cuda.h>
#include <cuda_runtime.h>



__forceinline__  __device__ float clamp(float x, float a, float b)
{
  return max(a, min(b, x));
}

__forceinline__  __device__ float RGBA2Y(uchar4  argb)
{
  return clamp((0.257 * argb.x) + (0.504 * argb.y) + (0.098 * argb.z) + 16,0,255);
}

__global__ static void CudaProcessY(int w, int h, uchar4 * pARGBImage, unsigned char * pNV12ImageY)
{
  unsigned int i = blockIdx.x*blockDim.x + threadIdx.x;
  unsigned int j = blockIdx.y*blockDim.y + threadIdx.y; 
  if(i<w && j<h)
  {
    uchar4 argb=pARGBImage[w*j+i];
    pNV12ImageY[w*j+i]= RGBA2Y(argb);
  }
 }

__forceinline__  __device__ float RGBA2U(uchar4  argb)
{
  return clamp(-(0.148 * argb.x) - (0.291 * argb.y) + (0.439 * argb.z)+128.0,0,255);
}

__forceinline__  __device__ float RGBA2V(uchar4  argb)
{
  return clamp((0.439 * argb.x) - (0.368 * argb.y) - (0.0701 * argb.z)+128.0,0,255);
}

__global__ static void CudaProcessUV(int w, int h, uchar4 * pARGBImage, unsigned char * pNV12ImageUV)
{
  unsigned int i = blockIdx.x*blockDim.x + threadIdx.x;
  unsigned int j = blockIdx.y*blockDim.y + threadIdx.y; 
  unsigned int fi = i*2;//full size image i
  unsigned int fj = j*2;//full size image j
  unsigned int fw = w*2;//full size image w
  unsigned int fh = h*2;//full size image h
  unsigned int u_idx = i*2 + 1 + j*w*2;
  unsigned int v_idx = i*2 + j*w*2;
  if(fi<fw-1 && fj<fh-1)
  {
    uchar4 argb1=pARGBImage[fj*fw+fi];
    uchar4 argb2=pARGBImage[fj*fw+fi+1];
    uchar4 argb3=pARGBImage[(fj+1)*fw+fi];
    uchar4 argb4=pARGBImage[(fj+1)*fw+fi+1];

    float U  = RGBA2U(argb1);
    float U2 = RGBA2U(argb2);
    float U3 = RGBA2U(argb3);
    float U4 = RGBA2U(argb4);

    float V =  RGBA2V(argb1);
    float V2 = RGBA2V(argb2);
    float V3 = RGBA2V(argb3);
    float V4 = RGBA2V(argb4);

    pNV12ImageUV[u_idx] = (U+U2+U3+U4)/4.0;
    pNV12ImageUV[v_idx] = (V+V2+V3+V4)/4.0;
  }
}


extern "C" 
{
  cudaError launch_CudaARGB2NV12Process(int w, int h, CUdeviceptr pARGBImage, CUdeviceptr pNV12Image)
  {
    {
      dim3 dimBlock(16, 16, 1);
      dim3 dimGrid(((w)+dimBlock.x-1)/dimBlock.x, ((h)+dimBlock.y-1)/dimBlock.y, 1);
      CudaProcessY<<<dimGrid, dimBlock>>>(w, h, (uchar4 *)pARGBImage, (unsigned char *)pNV12Image);   
    }
    {
      dim3 dimBlock(16, 16, 1);
      dim3 dimGrid(((w/2)+dimBlock.x-1)/dimBlock.x, ((h/2)+dimBlock.y-1)/dimBlock.y, 1);
      CudaProcessUV<<<dimGrid, dimBlock>>>(w/2, h/2, (uchar4 *)pARGBImage, ((unsigned char *)pNV12Image)+w*h);        
    }
    cudaError err = cudaGetLastError();                                
    return err;
  }
}