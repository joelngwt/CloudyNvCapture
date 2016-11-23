/*!
 * \file
 *
 * Implementation of ImageCollection and ImageObject.
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

#include "ImageCollection.h"
#include "Helper.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

/*!
 * ImageObject constructor.
 */
ImageObject::ImageObject(ImageType type) :
    m_type(type),
    m_filePath(NULL),
    m_data(NULL)
{
}

/*!
 * ImageObject destructor.
 */
ImageObject::~ImageObject(void)
{
    unload();
}

/*!
 * Load image file into ImageObject.
 */
bool ImageObject::load(const char *fileName)
{
    FILE *fp = NULL;

    BmpHeader bmpHeader;
    BmpInfoHeader bmpInfoHeader;

    unsigned char *buf = NULL, *image = NULL;

    if (m_type != IMAGE_TYPE_BMP)
    {
        PRINT_ERROR("Image Type is not supported yet.\n");
        return false;
    }

    if (initialize()) {
        unload();
    }

    fp = fopen(fileName, "r");
    if (fp == NULL)
    {
        PRINT_ERROR("Failed to open file");
        return false;
    }

    /* Reade File header */
    fread(&bmpHeader, sizeof(bmpHeader), 1, fp);

    if (bmpHeader.type != BIT_MAP_IMAGE_ID)
    {
        PRINT_ERROR("File %s is not in BMP formate\n", fileName);

        fclose(fp);
    }

    fread(&bmpInfoHeader, sizeof(bmpInfoHeader), 1, fp);

    fseek(fp,bmpHeader.offset,SEEK_SET);

    buf = (unsigned char*)malloc(bmpInfoHeader.imageSize);

    image = (unsigned char*)malloc(bmpInfoHeader.height * bmpInfoHeader.width * 4);

    if (fread(buf, bmpInfoHeader.imageSize, 1, fp) !=
        bmpInfoHeader.imageSize)
    {

        int indexH, indexW;

		int nBytesPerPixel = bmpInfoHeader.bits / 8;

        for (indexH = 0; indexH < bmpInfoHeader.height; indexH++)
        {
            unsigned char *dst = image + ((bmpInfoHeader.width * 4) * indexH);
            unsigned char *src = buf   + (indexH * (bmpInfoHeader.width * nBytesPerPixel));

            for (indexW = 0; indexW < bmpInfoHeader.width; indexW++)
            {
                dst[indexW * 4 + 0] = src[indexW * nBytesPerPixel + 2];
                dst[indexW * 4 + 1] = src[indexW * nBytesPerPixel + 1];
                dst[indexW * 4 + 2] = src[indexW * nBytesPerPixel + 0];
            }
        }
    }

    free(buf);

    fclose(fp);

    m_header     = bmpHeader;
    m_infoHeader = bmpInfoHeader;
    m_data       = image;

    m_filePath = strdup(fileName);

    return true;
}

/*!
 * Unload Image file from ImageObject.
 */
void ImageObject::unload(void)
{
    if (m_filePath)
    {
        free(m_filePath);
        m_filePath = NULL;
    }

    if (m_data)
    {
        free(m_data);
        m_data = NULL;
    }
}

/*!
 * Returns 'TRUE' if ImageObject is initialized,
 * and image file is loaded into it.
 */
inline bool ImageObject::initialize(void) const
{
    return (m_filePath != NULL);
}

/*!
 * Returns width of image.
 */
unsigned int ImageObject::getWidth(void)
{
    if (!initialize())
    {
        return 0;
    }

    return m_infoHeader.width;
}

/*!
 * Returns height of image.
 */
unsigned int ImageObject::getHeight(void)
{
    if (!initialize())
    {
        return 0;
    }

    return m_infoHeader.height;
}


/*!
 * Load ImageObject's image into given 2D texture,
 * if texture Id is 0 then create new texture.
 */
GLuint ImageObject::loadToTexture(GLuint texId)
{
    if (!initialize())
    {
        return 0;
    }

    if (texId == 0)
    {
        /* Texture name generation */
        glGenTextures(1, &texId);
    }

    /* Binding of texture name */
    glBindTexture(GL_TEXTURE_RECTANGLE, texId);

    /* We will use linear interpolation for magnification filter */
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /* We will use linear interpolation for minifying filter */
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA,
                 m_infoHeader.width, m_infoHeader.height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 m_data); /* Texture specification */

    glBindTexture(GL_TEXTURE_RECTANGLE, 0);

    return texId;

}

#ifndef _WIN32
/*!
 * Filter for .bmp files.
 */
static int bmpFilter(const struct dirent *fileEntry)
{
    const char *extension = strrchr(fileEntry->d_name, '.');

    if ((fileEntry->d_type & DT_REG) &&
        extension && strcmp(extension, ".bmp") == 0)
    {
        return 1;
    }

    return 0;
}
#endif // !_WIN32

/*!
 * ImageCollection constructor.
 */
ImageCollection::ImageCollection(ImageType type) :
    m_type(type),
    m_dirPath(NULL),
    m_fileList(NULL),
    m_fileCount(0),
    m_pos(0)
{
#ifndef _WIN32
    m_dirPtr = NULL;
#endif
}

/*!
 * ImageCollection destructor.
 */
ImageCollection::~ImageCollection(void)
{
    unload();
}

#ifdef _WIN32
/*!
 * qsort function to sort alphabetically
 */
int alphasort(const void *arg1, const void *arg2)
{
    return _stricmp(((WIN32_FIND_DATA*)arg1)->cFileName, ((WIN32_FIND_DATA*)arg2)->cFileName);
}
#endif // _WIN32

/*!
 * Load Images from given directory into collection.
 */
bool ImageCollection::load(const char *dirPath)
{
    if (m_type != IMAGE_TYPE_BMP)
    {
        PRINT_ERROR("Image Type is not supported yet\n");
        return false;
    }

    if (initialize()) {
        unload();
    }

    m_dirPath = strdup(dirPath);
    if (m_dirPath == NULL)
    {
        return false;
    }

#ifdef _WIN32
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    char dirSpec[MAX_PATH];

    strcpy(dirSpec, m_dirPath);
    strcat(dirSpec, "\\*.bmp");

    // first count files
    hFind = FindFirstFile(dirSpec, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
        return false;

    m_fileCount++;

    while (FindNextFile(hFind, &FindFileData) != 0) 
        m_fileCount++;

    FindClose(hFind);

    if (m_fileCount != 0)
    {
        unsigned int index;

        // alloc file list
        m_fileList = new WIN32_FIND_DATA[m_fileCount];
        if (!m_fileList)
            return false;

        // fill out file list
        index = 0;
        hFind = FindFirstFile(dirSpec, &m_fileList[index]);
        if (hFind == INVALID_HANDLE_VALUE)
            return false;

        index++;

        while (FindNextFile(hFind, &m_fileList[index]) != 0) 
            index++;

        FindClose(hFind);

        // sort by name
        qsort(m_fileList, m_fileCount, sizeof(WIN32_FIND_DATA), alphasort);
    }
#else
    /* Open given directory */
    m_dirPtr = opendir(m_dirPath);
    if (m_dirPtr == NULL)
    {
        return false;
    }

    /*
     * Scan all files from directory,
     * and count number of image files present in it.
     */
    m_fileCount = scandir(m_dirPath, &m_fileList,
                             bmpFilter, alphasort);
#endif

    m_pos = 0;

    return true;
}

/*
 * Unload all Images from collection.
 */
void ImageCollection::unload(void)
{
    if (m_dirPath)
    {
        free(m_dirPath);
        m_dirPath = NULL;
    }

#ifdef _WIN32
    delete [ ] m_fileList;
    m_fileList = NULL;
    m_fileCount = 0;
#else
    if (m_dirPtr)
    {
        closedir(m_dirPtr);
        m_dirPtr = NULL;
    }

    /*
     * Free list of image files,
     * generated during creation of collection.
     */
    if (m_fileCount > 0)
    {
        while (m_fileCount--)
        {
            free(m_fileList[m_fileCount]);
        }
        m_fileCount = 0;

        free(m_fileList);
        m_fileList = 0;
    }
#endif
}

/*!
 * Returns 'TRUE' if ImageCollection Object is initialized
 */
inline bool ImageCollection::initialize(void) const
{
#ifdef _WIN32
    return (m_dirPath != NULL);
#else
    return (m_dirPtr != NULL);
#endif
}

/*!
 * Returns number of images in coleection.
 */
unsigned int ImageCollection::getCount(void)
{
    if (!initialize())
    {
        return 0;
    }

    return m_fileCount;
}

/*!
 * Rewind image reading pointer.
 */
void ImageCollection::rewind(void)
{
    m_pos = 0;
}

/*!
 * Return ImageObject for next image from collection.
 */
ImageObject* ImageCollection::getNextImage(void)
{
#ifdef _WIN32
    WIN32_FIND_DATA *fileEntry;
#else
    struct dirent *fileEntry = NULL;
#endif
    char *fullPath = NULL;

    ImageObject *imageObj = NULL;

    if (!initialize())
    {
        return NULL;
    }

    if (m_pos >= m_fileCount)
    {
        return NULL;
    }

#ifdef _WIN32
    fileEntry = &m_fileList[m_pos++];

    /* Build full absolute path of image file */
    fullPath = new char[strlen(m_dirPath) + strlen(fileEntry->cFileName) + 2];
    strcpy(fullPath, m_dirPath);
    strcat(fullPath, "\\");
    strcat(fullPath, fileEntry->cFileName);
#else
    fileEntry = m_fileList[m_pos++];

    /* Build full absolute path of image file */
    fullPath = new char[strlen(m_dirPath) + strlen(fileEntry->d_name) + 2];
    strcpy(fullPath, m_dirPath);
    strcat(fullPath, "/");
    strcat(fullPath, fileEntry->d_name);
#endif

    /* Create image object for image file */
    imageObj = new ImageObject(IMAGE_TYPE_BMP);
    if (imageObj == NULL)
    {
        PRINT_ERROR("Insufficient resource\n");
    }

    if (imageObj  && !imageObj->load(fullPath))
    {
        PRINT_ERROR("Failed to load image file\n");

        delete imageObj;
        imageObj = NULL;
    }

    delete fullPath;

    return imageObj;
}

