/*!
 * \file
 *
 * Declaration of Classes OpenGLDisplay and OpenGLWin,
 * which can be used to manage Windows and OpenGL contexts.
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

#ifndef __OPENGL_WIN_H__

#define __OPENGL_WIN_H__

class OpenGLDisplay
{
friend class OpenGLWin;

private:
    /*!
     * Pointer to private data of implemention.
     */
    void *m_privData;

public:
    OpenGLDisplay(void);
    ~OpenGLDisplay(void);

    /*!
     * Return 'TRUE' if OpenGLDisplay object is initialized.
     */
    inline bool initialize(void) const;

    /*!
     * Open Logical display connection.
     */
    bool open(void);

    /*!
     * Close Logical display connection.
     */
    void close(void);
};

class OpenGLWin
{
private:
    /*!
     * OpenGLDisplay on which Window is opened.
     */
    const OpenGLDisplay *m_dpy;

    /*!
     * Title for Window.
     */
    char *m_title;

    /*!
     * Dimensions for Window.
     */
    unsigned int m_width, m_height;

    /*!
     * Pointer to private data of implementation.
     */
    void *m_privData;

public:
    OpenGLWin(void);
    ~OpenGLWin(void);

    /*!
     * Returns 'TRUE' if OpenGLWin object is initialized.
     */
    inline bool initialize(void) const;

    /*!
     * Open Window on given logical display connection.
     */
    bool open(const OpenGLDisplay *dpy, const char *title,
              unsigned int width, unsigned int height, const OpenGLWin *win);

    /*!
     * Close Window if Window is opened.
     */
    void close(void);

    /*!
     * Show Window in display.
     */
    void show(void);

    /*!
     * Make Window as current rendering target.
     */
    bool makeCurrent(void);

    /*!
     * Lose Window as current rendering target.
     */
    bool loseCurrent(void);

    /*!
     * Swap Window's rendering buffers.
     */
    void swapBuffers(void);
};

#endif
