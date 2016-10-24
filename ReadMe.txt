
/*
 *
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 *
 */

NvFBC/NvIFR Dependencies
------------------------
- CUDA Toolkit version 6.5 or newer
- Microsoft DirectX SDK (June 2010)
- Driver: Refer NVIDIA Capture SDK release notes

NVIDIA Capture Sample Dependencies
-------------------------------
- NvIFR DX9  samples require DX9, d3d9.dll
- NvIFR DX10 samples require DX10, d3dx10_43.dll
- NvIFR DX11 samples require DX11, d3dx11_43.dll, 
  dxgi.lib, and d3dxeffects.lib (included)
- NvFBC samples require DX9
- Stereo samples require a stereo-capable NV card


Install Notes (3/15/2016)
-------------------------

1. High-end Quadro cards, GRID, TESLA boards only. e.g. Quadro K5000, M5000

2. Use Latest Driver, as per NVIDIA Capture SDK release notes

3. Install CUDA 6.5 Toolkit, do not install the display drivers included with this package

4. Install installer .msi package

5. Run as administrator: 
    > cd\Program Files(x86)\NVIDIA Corporation\NVIDIA Capture SDK\bin
	> NvFBCEnable -enable

6. Build and Run samples.
