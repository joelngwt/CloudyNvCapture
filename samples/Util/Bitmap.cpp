/*!
 * \copyright
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */


#pragma warning(disable : 4995 4996)

#include "Bitmap.h"

#include <stdio.h>
#include <string>

// Macros to help with bitmap padding
#define BITMAP_SIZE(width, height) ((((width) + 3) & ~3) * (height))
#define BITMAP_INDEX(x, y, width) (((y) * (((width) + 3) & ~3)) + (x))

// Describes the structure of a 24-bpp Bitmap pixel
struct BitmapPixel
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
};

// Describes the structure of a RGB pixel
struct RGBPixel
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

// Describes the structure of a ARGB pixel
struct ARGBPixel
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char alpha;
};

bool SaveBitmap(const char *fileName, BYTE *data, int width, int height)
{
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    FILE *outputFile;
    bool bRet = false;

    if (data)
    {
        if(outputFile = fopen(fileName, "wb"))
        {
            width = (width + 3) & (~3);
            int size = width * height * 3; // 24 bits per pixel

            fileHeader.bfType = 0x4D42;
            fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + size;
            fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

            infoHeader.biSize = sizeof(BITMAPINFOHEADER);
            infoHeader.biWidth = width;
            infoHeader.biHeight = height;
            infoHeader.biPlanes = 1;
            infoHeader.biBitCount = 24;
            infoHeader.biCompression = BI_RGB;
            infoHeader.biSizeImage = BITMAP_SIZE(width, height);
            infoHeader.biXPelsPerMeter = 0;
            infoHeader.biYPelsPerMeter = 0;
            infoHeader.biClrUsed = 0;
            infoHeader.biClrImportant = 0;

            fwrite((unsigned char *)&fileHeader, 1, sizeof(BITMAPFILEHEADER), outputFile);
            fwrite((unsigned char *)&infoHeader, 1, sizeof(BITMAPINFOHEADER), outputFile);
            fwrite(data, 1, size, outputFile);

            bRet = true;
            fclose(outputFile);
        }
    }

    return bRet;
}

bool SaveRGB(const char *fileName, BYTE *data, int width, int height, int stride)
{
    bool result = false;

    RGBPixel *input = (RGBPixel *)data;
    BitmapPixel *output = new BitmapPixel[BITMAP_SIZE(width, height)];

    // Pad bytes need to be set to zero, it's easier to just set the entire chunk of memory
    memset(output, 0, BITMAP_SIZE(width, height) * sizeof(BitmapPixel));

    for(int row = 0; row < height; ++row)
    {
        for(int col = 0; col < width; ++col)
        {
            // In a bitmap (0,0) is at the bottom left, in the frame buffer it is the top left.
            int outputIdx = BITMAP_INDEX(col, row, width);
            int inputIdx = ((height - row - 1) * stride) + col;

            output[outputIdx].red = input[inputIdx].red;
            output[outputIdx].green = input[inputIdx].green;
            output[outputIdx].blue = input[inputIdx].blue;
        }
    }

    result = SaveBitmap(fileName, (BYTE *)output, width, height);

    delete [] output;

    return result;
}

bool SaveBGR(const char *fileName, BYTE *data, int width, int height, int stride)
{
    bool result = false;

    if (!data)
        return false;
    RGBPixel *input = (RGBPixel *)data;
    BitmapPixel *output = new BitmapPixel[BITMAP_SIZE(width, height)];

    // Pad bytes need to be set to zero, it's easier to just set the entire chunk of memory
    memset(output, 0, BITMAP_SIZE(width, height) * sizeof(BitmapPixel));

    for(int row = 0; row < height; ++row)
    {
        for(int col = 0; col < width; ++col)
        {
            // In a bitmap (0,0) is at the bottom left, in the frame buffer it is the top left.
            int outputIdx = BITMAP_INDEX(col, row, width);
            int inputIdx = ((height - row - 1) * stride) + col;

            output[outputIdx].red = input[inputIdx].blue;
            output[outputIdx].green = input[inputIdx].green;
            output[outputIdx].blue = input[inputIdx].red;
        }
    }

    result = SaveBitmap(fileName, (BYTE *)output, width, height);

    delete [] output;

    return result;
}

bool SaveRGBPlanar(const char *fileName, BYTE *data, int width, int height)
{
    if (!data)
        return false;

    const char *nameExt[] = {"red", "green", "blue"};
    BitmapPixel *output = new BitmapPixel[BITMAP_SIZE(width, height)];
    memset(output, 0, BITMAP_SIZE(width, height) * sizeof(BitmapPixel));

    for(int color = 0; color < 3; ++color)
    {
        for(int row = 0; row < height; ++row)
        {
            for(int col = 0; col < width; ++col)
            {   
                int outputIdx = BITMAP_INDEX(col, row, width);
                int inputIdx = ((height - row - 1) * width) + col;

                output[outputIdx].blue = 0;
                output[outputIdx].green = 0;
                output[outputIdx].red = 0;

                switch(color)
                {
                case 0:
                    output[outputIdx].red = data[inputIdx];
                    break;

                case 1:
                    output[outputIdx].green = data[inputIdx + (width * height)];
                    break;

                case 2:
                    output[outputIdx].blue = data[inputIdx + 2 * (width * height)];
                    break;

                default:
                    break;
                }
            }
        }

        std::string outputFile = fileName;
        size_t find = outputFile.find_last_of(".");

        outputFile.insert(find, "-");
        outputFile.insert(find+1, nameExt[color]);

        if(!SaveBitmap(outputFile.c_str(), (BYTE *)output, width, height))
        {
            delete [] output;
            return false;
        }
    }

    delete [] output;

    return true;
}

bool SaveARGB(const char *fileName, BYTE *data, int width, int height, int stride)
{
    bool result = false;
    if (!data)
        return result;

    int pitch = stride ? stride : width;

    ARGBPixel *input = (ARGBPixel *)data;
    BitmapPixel *output = new BitmapPixel[BITMAP_SIZE(width, height)];
    memset(output, 0, BITMAP_SIZE(width, height) * sizeof(BitmapPixel));

    for(int row = 0; row < height; ++row)
    {
        for(int col = 0; col < width; ++col)
        {
            int outputIdx = BITMAP_INDEX(col, row, width);
            int inputIdx = ((height - row - 1) * pitch) + col;

            output[outputIdx].red = input[inputIdx].red;
            output[outputIdx].green = input[inputIdx].green;
            output[outputIdx].blue = input[inputIdx].blue;
        }
    }

    result = SaveBitmap(fileName, (BYTE *)output, width, height);

    delete [] output;

    return result;
}

bool SaveYUV(const char *fileName, BYTE *data, int width, int height)
{
    if (!data)
        return false;

    int hWidth = width >> 1;
    int hHeight = height >> 1;
    size_t find = -1;
    std::string outputFile;

    BitmapPixel *luma = new BitmapPixel[BITMAP_SIZE(width, height)];
    BitmapPixel *chrom = new BitmapPixel[BITMAP_SIZE(width, height)];

    memset(luma, 0, BITMAP_SIZE(width, height) * sizeof(BitmapPixel));
    memset(chrom, 0, BITMAP_SIZE(hWidth, hHeight) * sizeof(BitmapPixel));

    for(int row = 0; row < height; ++row)
    {
        for(int col = 0; col < width; ++col)
        {
            int outputIdx = BITMAP_INDEX(col, row, width);
            int inputIdx = ((height - row - 1) * width) + col;

            luma[outputIdx].red = data[inputIdx];
            luma[outputIdx].green = data[inputIdx];
            luma[outputIdx].blue = data[inputIdx];
        }
    }

    data += width * height;

    outputFile = fileName;
    find = outputFile.find_last_of(".");

    outputFile.insert(find, "-");
    outputFile.insert(find+1, "y");

    if(!SaveBitmap(outputFile.c_str(), (BYTE *)luma, width, height))
    {
        delete [] luma;
        delete [] chrom;
        return false;
    }

    for(int row = 0; row < hHeight; ++row)
    {
        for(int col = 0; col < hWidth; ++col)
        {
            int outputIdx = BITMAP_INDEX(col, row, hWidth);
            int inputIdx = ((hHeight - row - 1) * hWidth) + col;

            chrom[outputIdx].red = data[inputIdx];
            chrom[outputIdx].green = 255 - data[inputIdx];
            chrom[outputIdx].blue = 0;
        }
    }

    data += hWidth * hHeight;

    outputFile = fileName;
    find = outputFile.find_last_of(".");

    outputFile.insert(find, "-");
    outputFile.insert(find+1, "u");

    if(!SaveBitmap(outputFile.c_str(), (BYTE *)chrom, hWidth, hHeight))
    {
        delete [] luma;
        delete [] chrom;
        return false;
    }

    for(int row = 0; row < hHeight; ++row)
    {
        for(int col = 0; col < hWidth; ++col)
        {
            int outputIdx = BITMAP_INDEX(col, row, hWidth);
            int inputIdx = ((hHeight - row - 1) * hWidth) + col;

            chrom[outputIdx].red = 0;
            chrom[outputIdx].green = 255 - data[inputIdx];
            chrom[outputIdx].blue = data[inputIdx];
        }
    }

    data += hWidth * hHeight;

    outputFile = fileName;
    find = outputFile.find_last_of(".");

    outputFile.insert(find, "-");
    outputFile.insert(find+1, "v");

    if(!SaveBitmap(outputFile.c_str(), (BYTE *)chrom, hWidth, hHeight))
    {
        delete [] luma;
        delete [] chrom;
        return false;
    }

    delete [] luma;
    delete [] chrom;
    return true;
}

#define CLAMP_255(x)  ((x) > 255 ? 255 : ((x) < 0 ? 0 : (x)))
bool SaveYUV444(const char *fileName, BYTE *data, int width, int height)
{
    bool result = false;
    bool bIsHD = (width*height < 1280*720);
    BitmapPixel *output = new BitmapPixel[BITMAP_SIZE(width, height)];
    BYTE *y = data;
    BYTE *u = y + width*height;
    BYTE *v = u + width*height;

    // Pad bytes need to be set to zero, it's easier to just set the entire chunk of memory
    memset(output, 0, BITMAP_SIZE(width, height) * sizeof(BitmapPixel));

    for(int row = 0; row < height; ++row)
    {
        for(int col = 0; col < width; ++col)
        {
            // In a bitmap (0,0) is at the bottom left, in the frame buffer it is the top left.
            int outputIdx = BITMAP_INDEX(col, row, width);
            int inputIdx = ((height - row - 1) * width) + col;
            if (!bIsHD)
            {
                output[outputIdx].red    = (BYTE)CLAMP_255(1.164*(y[inputIdx]-16) + 0.000*(u[inputIdx]-128) + 1.793*(v[inputIdx]-128));
                output[outputIdx].green  = (BYTE)CLAMP_255(1.164*(y[inputIdx]-16) - 0.213*(u[inputIdx]-128) - 0.534*(v[inputIdx]-128));
                output[outputIdx].blue   = (BYTE)CLAMP_255(1.164*(y[inputIdx]-16) + 2.115*(u[inputIdx]-128) + 0.000*(v[inputIdx]-128));
            }
            else
            {
                output[outputIdx].red    = (BYTE)CLAMP_255(1.0*(y[inputIdx]) + 0.0000*(u[inputIdx]-128) + 1.5400*(v[inputIdx]-128));
                output[outputIdx].green  = (BYTE)CLAMP_255(1.0*(y[inputIdx]) - 0.1830*(u[inputIdx]-128) - 0.4590*(v[inputIdx]-128));
                output[outputIdx].blue   = (BYTE)CLAMP_255(1.0*(y[inputIdx]) + 1.8160*(u[inputIdx]-128) + 0.0000*(v[inputIdx]-128));
            }
        }
    }

    result = SaveBitmap(fileName, (BYTE *)output, width, height);

    delete [] output;

    return result;
}

bool YUV420ToYUV444(BYTE* in, BYTE *out, int width, int height)
{
    int hWidth = width>>1;
    int hHeight= height>>1;
    BYTE *oY = out; BYTE *oU = oY+width*height; BYTE *oV = oU+width*height;
    BYTE *iY = in;  BYTE *iU = iY+width*height; BYTE *iV = iU+hWidth*hHeight;

    for(int row = 0; row < height; ++row)
    {
        for(int col = 0; col < width; ++col)
        {
            int inputIdx = ((height - row - 1) * width) + col;
            int outputIdx= ((height - row - 1) * width) + col;
            int inputChIdx = ((hHeight - row/2 - 1) * hWidth) + col/2;
 
            oY[outputIdx] = iY[inputIdx];
            oU[outputIdx] = iU[inputChIdx];
            oV[outputIdx] = iV[inputChIdx];
        }
    }
    return true;
}

bool NV12ToYUV444(BYTE* in, BYTE *out, int width, int height, int pitch)
{
    int hWidth = width >> 1;
    int hHeight = height >> 1;
    int hPitch = pitch >> 1;
    BYTE *oY = out; BYTE *oU = oY + width*height; BYTE *oV = oU + width*height;
    BYTE *iY = in;  BYTE *iU = iY + height*pitch; 

    for (int row = 0; row < height; ++row)
    {
        for (int col = 0; col < width; ++col)
        {
            int inputIdx = ((height - row - 1) * pitch) + col;
            int outputIdx = ((height - row - 1) * width) + col;
            int inputChIdx = (((hHeight - row / 2 - 1) * hPitch) + col / 2)*2;

            oY[outputIdx] =  iY[inputIdx];
            oU[outputIdx] =  iU[inputChIdx];
            oV[outputIdx] =  iU[inputChIdx + 1];
        }

    }
    return true;
}

bool SaveYUV420(const char *fileName, BYTE *data, int width, int height)
{
    bool result = false;

    int hWidth = width>>1;
    int hHeight= height>>1;

    BYTE *yuv444 = new BYTE[width*height*3];
    BYTE *y = yuv444;
    BYTE *u = y + width*height;
    BYTE *v = u + hWidth*hHeight;

    YUV420ToYUV444(data, yuv444, width, height);
    result = SaveYUV444(fileName, yuv444, width, height);
    delete [] yuv444;
    return result;
}

bool SaveNV12(const char *fileName, BYTE *data, int width, int height, int stride)
{
    bool result = false;

    int hWidth = width >> 1;
    int hHeight = height >> 1;

    BYTE *yuv444 = new BYTE[width*height * 3];
    BYTE *y = yuv444;
    BYTE *u = y + width*height;
    BYTE *v = u + hWidth*hHeight;

    NV12ToYUV444(data, yuv444, width, height, stride);
    result = SaveYUV444(fileName, yuv444, width, height);
    delete[] yuv444;
    return result;
}