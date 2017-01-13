# DXIFRShim
A 64-bit machine running a 64-bit application is assumed in these instructions.

## Configuring DXIFRShim
1. Open NvIFEEncoder.cpp
2. Search for "http://"
3. Change the IP address to the IP address of the machine you want to stream from.

## Compiling DXIFRShim
1. Open DXIFRShim_VS2013.sln.
2. Change build configuration to release, x64.
3. Build solution.

## Using DXIFRShim
1. Rename the d3d9.dll and dxgi.dll files in the system32 folder to _d3d9.dll and _dxgi.dll
2. Transfer d3d9.dll and dxgi.dll that was produced by DXIFRShim to the system32 folder
3. Download the FFmpeg .dll files from [Zeranoe](https://ffmpeg.zeranoe.com/builds/). Use the 64-bit shared version. If the latest build does not work due to deprecated functions, use the 20161101 version [here](https://ffmpeg.zeranoe.com/builds/win64/shared/ffmpeg-20161101-60178e7-win64-shared.zip).
4. Place all the .dll files from the /bin folder into system32. 
5. Run StartApp.exe that was produced by DXIFRShim with the command prompt. An example command:
```
StartApp.exe -r 1280x720 -gpu 1 -audio 0 -players 1 -rows 1 -cols 1  C:\Users\User\Desktop\Test\WindowsNoEditor\MyProject.exe
```
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
