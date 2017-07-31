# DXIFRShim
A 64-bit machine running a 64-bit application is assumed in these instructions.

## Configuring DXIFRShim
1. Open NvHWEncoder.cpp.
2. Search for the `streamingIP` variable.
3. Change the IP address to the IP address of the machine you want to stream from.
4. Search for the `firstPort` variable. 
5. Change the port to the port you want to stream from. Note that this is the first port. Additional players use the next port following it.

## Compiling DXIFRShim
1. Open DXIFRShim_VS2013.sln.
2. Change build configuration to release, x64.
3. Build solution.

## Using DXIFRShim
1. Make a copy of the dxgi.dll file that are in the system32 folder.
2. Rename the dxgi.dll file in the system32 folder to _dxgi.dll. You should now have dxgi.dll and _dxgi.dll in the system32 folder.
3. Download the FFmpeg.exe files from [Zeranoe](https://ffmpeg.zeranoe.com/builds/). Use the 64-bit static version. 
4. Place the FFmpeg.exe file somewhere where it will not be moved, then add the directory into your system environment variables.
5. After compiling the DXIFRShim solution, you should have your own dxgi.dll file. Place them together with the game executable (.exe)*.
6. Launch the game .exe file.

*Some game .exe files are tricky. Find the one that actually runs. It might not be the same one you use to launch the game. In the case of packaged Unreal Engine games, you should put it in `\WindowsNoEditor\ProjectName\Binaries\Win64`

> WARNING: If you do not have a working .dll file in system32 and you log out of Windows, you will not be able to log back in. The only way to recover from this is a system restore.

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
