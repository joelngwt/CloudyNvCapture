# DXIFRShim
A 64-bit machine running a 64-bit application is assumed in these instructions.

## Configuring DXIFRShim
1. Open NvIFEEncoder.cpp.
2. Search for the `streamingIP` variable.
3. Change the IP address to the IP address of the machine you want to stream from.
4. Search for the `firstPort` variable. 
5. Change the port to the port you want to stream from. Note that this is the first port. Additional players use the next port following it.
6. Search for the `bufferWidth` and `bufferHeight` variables.
7. Change the parameters as necessary for your application. This should match the size of the game window for optimal quality. 

## Compiling DXIFRShim
1. Open DXIFRShim_VS2013.sln.
2. Change build configuration to release, x64.
3. Build solution.

## Using DXIFRShim
1. Make a copy of the d3d9.dll an dxgi.dll files that are in the system32 folder.
2. Rename one of the d3d9.dll and dxgi.dll files in the system32 folder to _d3d9.dll and _dxgi.dll. You should now have d3d9.dll, _d3d9.dll, dxgi.dll, and _dxgi.dll in the system32 folder.
3. Download the FFmpeg .dll files from [Zeranoe](https://ffmpeg.zeranoe.com/builds/). Use the 64-bit shared version. If the latest build does not work due to deprecated functions, use the 20161101 version [here](https://ffmpeg.zeranoe.com/builds/win64/shared/ffmpeg-20161101-60178e7-win64-shared.zip).
4. Place all the .dll files from the /bin folder into system32.
5. After compiling the DXIFRShim solution, you should have your own d3d9.dll and dxgi.dll files. Place them together with the game executable (.exe).
6. Launch the game .exe file.

> WARNING: If you do not have a working set of the two .dll files in system32 and you log out of Windows, you will not be able to log back in. The only way to recover from this is a system restore.

# NvFBCToSys
## Configuring NvFBCToSys
1. Open NvFBCToSys.cpp
2. Search for "http://"
3. Change the IP address to the IP address of the machine you want to stream from.

## Compiling NvFBCToSys
1. Open NvFBCToSys_2013.sln.
2. Change build configuration to release, x64.
3. Build solution.

## Using NvFBCToSys
1. Navigate to the /samples folder 
2. Open the /Release/x64 folder
3. Run NvFBCToSys.exe using the command prompt. An example command:
```
NvFBCToSys.exe -scale 1920 1080 -grabCursor -output test.yuv -port 30000 -players 1 -nowait
```
