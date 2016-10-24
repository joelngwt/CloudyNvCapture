/*!
 * \file
 *
 * Targa Image File (TGA) writer
 *
 * \copyright
 * Copyright 2013-2014 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO LICENSEE:
 *
 * This source code and/or documentation ("Licensed Deliverables") are
 * subject to the applicable NVIDIA license agreement that governs the
 * use of the Licensed Deliverables.
 */

#include <stdio.h>
#include <memory.h>

#define IMAGE_TYPE_NONE 0               //!< no image data is present
#define IMAGE_TYPE_COLOR_MAPPED 1       //!< uncompressed color-mapped image
#define IMAGE_TYPE_TRUE_COLOR 2         //!< uncompressed true-color image
#define IMAGE_TYPE_BW 3                 //!< uncompressed black-and-white (grayscale) image
#define IMAGE_TYPE_RLE_COLOR_MAPPED 9   //!< run-length encoded color-mapped image
#define IMAGE_TYPE_RLE_TRUE_COLOR 10    //!< run-length encoded true-color image
#define IMAGE_TYPE_RLE_BW 11            //!< run-length encoded black-and-white (grayscale) image

#pragma pack(push,1)

//! the TGA header definition
typedef struct
{
    char IDLength;          //!< The number of bytes that the image ID field consists of (0 - 255)
    char colorMapType;      //!< Color map type (0 = none, 1 = has palette)
    char imageType;         //!< Image type (see IMAGE_TYPE_???)

    short colormapStart;    //!< offset into the color map table
    short colormapLength;   //!< number of entries
    char colormapBits;      //!< number of bits per pixel (15, 16, 24, 32)

    short xOrigin;          //!< absolute coordinate of lower-left corner for displays where origin is at the lower left
    short yOrigin;          //!< as for X-origin
    short width;            //!< image width in pixels
    short height;           //!< image height in pixels
    char bits;              //!< image bits per pixel (8, 16, 24, 32)
    char descriptor;        //!< image descriptor bits (bits 3-0 give the alpha channel depth, bits 5-4 give directions)
} TGAHeader;

/*!
 * Write an image to a TGA file
 *
 * \param [in] fileName
 *   The name of the file to write to
 * \param [in] bits
 *   image bits per pixel (8, 16, 24, 32)
 * \param [in] width
 *   image width
 * \param [in] height
 *   image height
 * \param [in] data
 *   image data
 *
 * \return false if failed
 */
bool saveAsTGA(const char *fileName, char bits, short width, short height, const void *data)
{
    TGAHeader header;
    FILE *file;

    memset(&header, 0, sizeof(header));

    file = fopen(fileName, "wb");
    if (file == NULL)
        return false;

    if (bits == 8)
        header.imageType = IMAGE_TYPE_BW;
    else
        header.imageType = IMAGE_TYPE_TRUE_COLOR;
    header.width = width;
    header.height = height;
    header.bits = bits;
    if (bits == 32)
        header.descriptor = bits - 24;

    if (fwrite(&header, 1, sizeof(header), file) != sizeof(header))
    {
        fclose(file);
        return false;
    }

    if (fwrite(data, 1, width * height * (bits >> 3), file) != (size_t)(width * height * (bits >> 3)))
    {
        fclose(file);
        return false;
    }

    fclose(file);

    return true;
}
