/*!
 * \file
 *
 * Declaration of Classes ImageCollection and ImageObject,
 * which helps to manage collection of image files.
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

#ifndef __IMAGE_COLLECTION_H__

#define __IMAGE_COLLECTION_H__

#include <stdio.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dirent.h>
#endif
#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>
#include <GL/glext.h>
#include <GL/gl.h>

typedef enum {
    IMAGE_TYPE_UNKNOWN,
    IMAGE_TYPE_BMP
} ImageType;

#pragma pack(push, 1)
typedef struct {
    unsigned short type;                 /* Magic identifier            */
    unsigned int   size;                 /* File size in bytes          */
    unsigned int   reserved;
    unsigned int   offset;               /* Offset to image data, bytes */
} BmpHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    unsigned int   size;               /* Header size in bytes      */
    int            width,
                   height;             /* Width and height of image */
    unsigned short planes;             /* Number of colour planes   */
    unsigned short bits;               /* Bits per pixel            */
    unsigned int   compression;        /* Compression type          */
    unsigned int   imageSize;          /* Image size in bytes       */
    int            xResolution,
                   yResolution;        /* Pixels per meter          */
    unsigned int   nColours;           /* Number of colours         */
    unsigned int   importantColours;   /* Important colours         */
} BmpInfoHeader;
#pragma pack(pop)

#define BIT_MAP_IMAGE_ID 0x4D42

class ImageObject {

private:
    BmpHeader m_header;
    BmpInfoHeader m_infoHeader;

    /*!
     * Image Type.
     */
    ImageType m_type;

    /*!
     * Absolute path of Image file
     */
    char *m_filePath;

    /*!
     * Image Data.
     */
    unsigned char *m_data;

public:
    ImageObject(ImageType type);
    ~ImageObject(void);

    /*!
     * Return 'TRUE' if Image file loaded successfully.
     */
    bool load(const char *fileName);

    /*!
     * Unload image file from ImageObject.
     */
    void unload(void);

    /*!
     * Returns 'TRUE' if ImageObject is initilized.
     */
    inline bool initialize(void) const;

    /*!
     * Returns Width and Height of Image.
     */
    unsigned int getWidth(void);
    unsigned int getHeight(void);

    /*!
     * Load Image into given texture, if textId is valid;
     * otherwise create new texture and return it.
     */
    GLuint loadToTexture(GLuint texId);
};

class ImageCollection {

private:
    /*!
     * Image Type.
     */
    ImageType m_type;

    /*!
     * Path of directory.
     */
    char *m_dirPath;

#ifndef _WIN32
    /*!
     * Pointer of DIR structure
     */
    DIR  *m_dirPtr;
#endif

    /*!
     * List of Image files present in Image collection.
     */
#ifdef _WIN32
    WIN32_FIND_DATA *m_fileList;
#else
    struct dirent **m_fileList;
#endif

    unsigned int m_fileCount, m_pos;

public:
    ImageCollection(ImageType type);
    ~ImageCollection(void);

    /*!
     * Load Images from given directory into collection,
     * and return 'TRUE' on success.
     */
    bool load(const char *dirPath);

    /*!
     * Unload all Images from collection.
     */
    void unload(void);

    /*!
     * Returns 'TRUE' if ImageCollection is initilized.
     */
    inline bool initialize(void) const;

    /*!
     * Return number of images.
     */
    unsigned int getCount(void);

    /*!
     * Rewind image reading pointer to first image.
     */
    void rewind(void);

    /*!
     * Returns next available image from Image Collection.
     */
    ImageObject* getNextImage(void);
};

#endif
