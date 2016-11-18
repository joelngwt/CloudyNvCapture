/**
 *
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 *
 */

#ifndef __cuda_drvapi_dynlink_h__
#define __cuda_drvapi_dynlink_h__

#include "cuda_drvapi_dynlink_cuda.h"

#if defined(CUDA_INIT_D3D9)||defined(CUDA_INIT_D3D10)||defined(CUDA_INIT_D3D11)
#include "cuda_drvapi_dynlink_d3d.h"
#endif

#ifdef CUDA_INIT_OPENGL
#include "cuda_drvapi_dynlink_gl.h"
#endif

#endif //__cuda_drvapi_dynlink_h__
