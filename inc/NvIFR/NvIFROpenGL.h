/*!
 * \file
 *
 * This file contains the interface constants, structure definitions and
 * function prototypes defining the NvIFR for OpenGL API.
 *
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 *
 */

#ifndef _NV_NVIFROGL_H_
#define _NV_NVIFROGL_H_

/*!
 * \mainpage NVIDIA OpenGL Inband Frame Readback (NvIFR for OpenGL).
 * NvIFR for OpenGL is a high performance,
 * low latency API to capture and optionally compress an individual OpenGL
 * framebuffer. Both named (non-zero) framebuffers and the window-system provided
 * default framebuffer can be the source of a capture operation.
 * The output from NvIFR for OpenGL does not include any window manager
 * decoration, composited overlay, cursor or taskbar; it provides solely the
 * pixels rendered into the framebuffer, as soon as their rendering is
 * complete, ahead of any compositing that may be done by the windows manager.
 * In fact, NvIFR for OpenGL does not require that the framebuffer even be
 * visible on the Windows desktop. It is ideally suited to application capture
 * and remoting, where the output of a single application, rather than the
 * entire desktop environment, is captured.
 *
 * NvIFR for OpenGL is intended to operate inband with a rendering application,
 * either as part of the application itself, or as part of a shim layer operating
 * immediately below the application. Like NvFBC, NvIFR for OpenGL operates
 * asynchronously to graphics rendering, using dedicated hardware compression
 * and copy engines in the GPU, and delivering pixel data to system memory with
 * minimal impact on rendering performance.
 *
 * All functions are thread-safe and can be called concurrently.
 * All functions are blocking calls unless stated explicitly.
 */

/*!
 * \defgroup NvIFROGL NVIDIA OpenGL Inband Frame Readback
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <GL/gl.h>

#include <stdlib.h>
#ifndef _WIN32
#include <stdint.h>
#endif

//! Calling convention
#ifdef _WIN32
#define NVIFROGLAPI __stdcall
#else
#define NVIFROGLAPI
#endif

/*!
 * Boolean value.
 */
typedef enum
{
    /*!
     * False.
     */
    NV_IFROGL_BOOL_FALSE,
    /*!
     * True.
     */
    NV_IFROGL_BOOL_TRUE,
} NV_IFROGL_BOOL;

/*!
 * NvIFROGL interface major version definition.
 */
#define NVIFROGL_VERSION_MAJOR  1
/*!
 * NvIFROGL interface minor version definition.
 */
#define NVIFROGL_VERSION_MINOR  2

/*!
 * Macro to generate per-structure version for use with API.
 */
#define NVIFROGL_VERSION (unsigned int)((NVIFROGL_VERSION_MINOR<<16) | (NVIFROGL_VERSION_MAJOR << 24))

/*!
 * The maximum number of reference frames supported by encoder.
 */
#define NVIFROGL_MAX_REF_FRAMES 0x10

/*!
 * Error Codes
 */
typedef enum
{
    /*!
     * This indicates a failure. To get more information call ::NvIFROGLGetError
     */
    NV_IFROGL_FAILURE,

    /*!
     * This indicates that API call returned with no errors.
     */
    NV_IFROGL_SUCCESS,

    /*!
     * This indicates that NvIFR library does not support this client version.
     * see ::NVIFROGL_VERSION for version details.
     */
    NV_IFROGL_CLIENT_VERSION_UNSUPPORTED,

    /*!
     * This indicates that NvIFR library does not support a feature
     * client requested for.
     */
    NV_IFROGL_UNSUPPORTED_FEATURE,

    /*!
     * This indicates that invalid parameter is passed to NvIFR library
     */
    NV_IFROGL_INVALID_PARAMETER,

    /*!
     * This indicates that sufficient memory is not available
     */
    NV_IFROGL_OUT_OF_MEMORY,

    /*!
     * This indicates the failure during encode
     */
    NV_IFROGL_ENCODER_FAILURE,

    /*!
     * This indicates a CUDA Error
     */
    NV_IFROGL_CUDA_FAILURE,

    /*!
     * This indicates OpenGL Error
     */
    NV_IFROGL_GL_FAILURE,
} NVIFROGLSTATUS;

/*!
 * Transfer object codec type.
 */
typedef enum
{
    /*!
     * Use H.264 hardware encoder.
     */
    NV_IFROGL_HW_ENC_H264 = 0x00,

    /*!
     * Use H.265 hardware encoder.
     */
    NV_IFROGL_HW_ENC_H265 = 0x01,
} NV_IFROGL_HW_ENC_TYPE;

/*!
 * Transfer object flags.
 */
typedef enum
{
    /*!
     * No flag set.
     */
    NV_IFROGL_TRANSFER_OBJECT_FLAG_NONE = 0x00,
} NV_IFROGL_TRANSFER_OBJECT_FLAGS;

/*!
 * Transfer object target format
 */
typedef enum
{
    /*!
     * The format and type will be specified by the ::NV_IFROGL_TO_SYS_CONFIG::customFormat
     * and NV_IFROGL_TO_SYS_CONFIG::customType parameters. The default for the
     * format is GL_RGBA and for the type GL_UNSIGNED_BYTE.
     */
    NV_IFROGL_TARGET_FORMAT_CUSTOM,
    /*!
     * Data will be converted to NV12 format using HDTV weights according
     * to ITU-R BT.709. This is a YUV 4:2:0 image with a plane of 8 bit Y
     * samples followed by an interleaved U/V plane containing 8 bit 2x2
     * subsampled colour difference samples.
     */
    NV_IFROGL_TARGET_FORMAT_NV12,
    /*!
     * Data will be converted to YUV 420 planar format using HDTV weights
     * according to ITU-R BT.709.
     */
    NV_IFROGL_TARGET_FORMAT_YUV420P,
    /*!
     * Data will be converted to YUV 444 format using HDTV weights according
     * to ITU-R BT.709.
     */
    NV_IFROGL_TARGET_FORMAT_YUV444,
    /*!
     * Data will be converted to YUV 444 planar format using HDTV weights
     * according to ITU-R BT.709.
     */
    NV_IFROGL_TARGET_FORMAT_YUV444P,
    /*!
     * Data will be converted to RGB unsigned byte planar format, stored
     * sequentially in memory as complete red channel, complete green channel,
     * complete blue channel.
     */
    NV_IFROGL_TARGET_FORMAT_RGB_PLANAR,
} NV_IFROGL_TARGET_FORMATS;

/*!
 * Transfer framebuffer flags.
 */
typedef enum
{
    /*!
     * No flag set.
     */
    NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAG_NONE = 0x00,
    /*!
     * Scales the image to the size given by the width and height parameters.
     * The xOffset and yOffset paramters are ignored.
     * This can't be set together with the ::NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAG_CROP
     * flag.
     */
    NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAG_SCALE = 0x01,
    /*!
     * Grabs a sub-region of the framebuffer based on the provided xOffset,
     * yOffset, width and height parameters.
     * This can't be set together with the ::NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAG_SCALE
     * flag.
     */
    NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAG_CROP = 0x02,
} NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAGS;

/*!
 * The stereo formats supported by the hw encoder.
 */
typedef enum
{
    /*!
     * No stereo coding. Use this value when encoding non-stereo frames.
     */
    NV_IFROGL_HW_ENC_STEREO_NONE,
    /*!
     * Compressed side by side coding.
     */
    NV_IFROGL_HW_ENC_STEREO_COMPRESSED_SBS,
    /*!
     * Full frames, top/bottom coding with SEI messages.
     */
    NV_IFROGL_HW_ENC_STEREO_SEI_TB,
    /*!
     * Multiview video coding.
     */
    NV_IFROGL_HW_ENC_STEREO_MVC
} NV_IFROGL_HW_ENC_STEREO_FORMAT;

/*!
 * The rate control methods supported by the hw encoder.
 */
typedef enum
{
    /*!
     * Constant Quantizer (CQ).
     * Uses constant value specified by the ::NV_IFROGL_HW_ENC_CONFIG::quantizationParam
     * parameter. This parameter controls the level of quantization in each
     * frame rather than targeting a certain bitrate. While the peak and average
     * bitrates will not be known in advance when using this method, the overall
     * image quality should be fairly consistent across each frame.
     */
    NV_IFROGL_HW_ENC_RATE_CONTROL_CONSTQP,
    /*!
     * Variable bitrate (VBR).
     * Targets the bitrate specified by the ::NV_IFROGL_HW_ENC_CONFIG::avgBitRate
     * parameter. The bitrate will fluctuate around this level, going as high
     * as the value specified by the ::NV_IFROGL_HW_ENC_CONFIG::peakBitRate field.
     * Image quality may decrease in more complex frames if this value is not
     * high enough to encode them properly. While using this method will result
     * in less stable and predictable bitrates, it will generally provide better
     * quality than CBR at the same average bitrate (if the peak bitrate is
     * sufficiently higher than the average bitrate) by allowing the encoder to
     * use more data when it is required for complex frames (and less data for
     * simpler frames).
     */
    NV_IFROGL_HW_ENC_RATE_CONTROL_VBR,
    /*!
     * Constant bitrate (CBR).
     * Targets the bitrate specified by the ::NV_IFROGL_HW_ENC_CONFIG::avgBitRate
     * field. The bitrate will remain roughly constant at this level. The image
     * quality for each frame will depend on how well it can be encoded within
     * this limit.
     */
    NV_IFROGL_HW_ENC_RATE_CONTROL_CBR,
    /*!
     * Do not use. This rate control mode is deprecated.
     * API returns an error NV_IFROGL_UNSUPPORTED_FEATURE.
     */
    NV_IFROGL_HW_ENC_RATE_CONTROL_VBR_MINQP,
    /*!
     * This rate control mode supports 2-pass encoding optimized for image
     * quality as a 2-pass encoding is a more accurate prediction model than
     * 1-pass encoding.
     */
    NV_IFROGL_HW_ENC_RATE_CONTROL_2_PASS_QUALITY,
    /*!
     * This rate control mode supports 2 pass encoding and is similar to
     * NV_IFROGL_HW_ENC_RATE_CONTROL_2_PASS_QUALITY, except when used with a single
     * frame VBV buffer size. Unlike 2_PASS_QUALITY, for a single frame VBV buffer
     * size this mode is not allowed to violate the VBV buffer size restriction.
     */
    NV_IFROGL_HW_ENC_RATE_CONTROL_2_PASS_FRAMESIZE_CAP,
    /*!
     * Deprecated. Use NV_IFROGL_HW_ENC_RATE_CONTROL_CBR instead.
     */
    NV_IFROGL_HW_ENC_RATE_CONTROL_CBR_IFRAME_2_PASS,
} NV_IFROGL_HW_ENC_RATE_CONTROL;

/*!
 * Encode configuration flags.
 */
typedef enum
{
    /*!
     * No flag set.
     */
    NV_IFROGL_HW_ENC_CONFIG_FLAG_NONE = 0x00,
    /*!
     * If set then the resolution of the encoded image can be changed dynamically
     * by adding NV_IFROGL_HW_ENC_PARAM_FLAG_DYN_RESOLUTION_CHANGE to NV_IFROGL_HW_ENC_PARAMS::flags.
     * This requires that ::NV_IFROGL_HW_ENC_CONFIG::maxDynamicWidth and
     * ::NV_IFROGL_HW_ENC_CONFIG::maxDynamicHeight are set to the maximum
     * allowed resolution.
     */
    NV_IFROGL_HW_ENC_CONFIG_FLAG_DYN_RESOLUTION_CHANGE = 0x01,
} NV_IFROGL_HW_ENC_CONFIG_FLAGS;

/*!
 * encoder slicing modes
 */
typedef enum
{
    /*!
     * Picture will be encoded as a one slice.
     */
    NV_IFROGL_HW_ENC_SLICING_MODE_DISABLED          = 0x0,
    /*!
     * Picture will be divided into slices of n MBs, where n = slicingModeParam.
     */
    NV_IFROGL_HW_ENC_SLICING_MODE_FIXED_NUM_MBS     = 0x1,
    /*!
     * Picture will be divided into slices of n Bytes, where n = slicingModeParam.
     */
    NV_IFROGL_HW_ENC_SLICING_MODE_FIXED_NUM_BYTES   = 0x2,
    /*!
     * Picture will be divided into slices of n rows of MBs, where n = slicingModeParam.
     */
    NV_IFROGL_HW_ENC_SLICING_MODE_FIXED_NUM_MB_ROWS = 0x3,
    /*!
     * Picture will be divided into n+1 slices, where n = slicingModeParam.
     */
    NV_IFROGL_HW_ENC_SLICING_MODE_FIXED_NUM_SLICES  = 0x4
} NV_IFROGL_HW_ENC_SLICING_MODE;

/*!
 * Encode parameter flags.
 */
typedef enum
{
    /*!
     * No flag set.
     */
    NV_IFROGL_HW_ENC_PARAM_FLAG_NONE                  = 0x00,
    /*!
     * If set then NV_IFROGL_HW_ENC_PARAMS::newAvgBitRate and
     * NV_IFROGL_HW_ENC_PARAMS::newPeakBitRate specify the new bitrate
     * values used from current frame onwards.
     */
    NV_IFROGL_HW_ENC_PARAM_FLAG_DYN_BITRATE_CHANGE    = 0x01,
    /*!
     * If set then the current frame will be Encoded as an IDR picture.
     */
    NV_IFROGL_HW_ENC_PARAM_FLAG_FORCEIDR              = 0x02,
    /*!
     * If set then NV_IFROGL_HW_ENC_PARAMS::newWidth and
     * NV_IFROGL_HW_ENC_PARAMS::newHeight specify the new width and
     * height of the encoded image from current frame onwards.
     */
    NV_IFROGL_HW_ENC_PARAM_FLAG_DYN_RESOLUTION_CHANGE = 0x04,
} NV_IFROGL_HW_ENC_PARAM_FLAGS;

/*!
 * Encoder preset.
 */
typedef enum
{
    /*!
     * Use for better quality than NVIFR_HW_ENC_PRESET_LOW_LATENCY_HP and higher
     * speed than NVIFR_HW_ENC_PRESET_LOW_LATENCY_HQ.
     */
    NV_IFROGL_HW_ENC_PRESET_LOW_LATENCY_DEFAULT,
    /*!
     * Use for Fastest encode, with suboptimal quality
     */
    NV_IFROGL_HW_ENC_PRESET_LOW_LATENCY_HP,
    /*!
     * Use for better overall quality, compromising on encoding speed.
     */
    NV_IFROGL_HW_ENC_PRESET_LOW_LATENCY_HQ,
    /*!
     * Use for lossless encoding at higher performance.
     * This preset works with NV_IFROGL_HW_ENC_RATE_CONTROL_CONSTQP only.
     * This preset works with NV_IFROGL_HW_ENC_H264 only.
     * Encoder profile is set to HIGH_444_INTRA (244) when this option is set.
     */
    NV_IFROGL_HW_ENC_PRESET_LOSSLESS_HP,
} NV_IFROGL_HW_ENC_PRESET;

/*!
 * Debug severity
 */
typedef enum
{
    /*!
     * Any message which is not an error or performance concern.
     */
    NV_IFROGL_DEBUG_SEVERITY_NOTIFICATION,

    /*!
     * Performance warnings.
     */
    NV_IFROGL_DEBUG_SEVERITY_LOW,

    /*!
     * Severe performance warnings.
     */
    NV_IFROGL_DEBUG_SEVERITY_MEDIUM,

    /*!
     * Any error.
     */
    NV_IFROGL_DEBUG_SEVERITY_HIGH,
} NV_IFROGL_DEBUG_SEVERITY;

/*!
 * HW_ENC input format
 */
typedef enum
{
    /*!
     * Captured OpenGL surface is converted into YUV420 format and passed as an
     * input to the hardwareencoder.
     */
    NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV420,
    /*!
     * Captured OpenGL surface is converted into YUV444 format and passed as an
     * input to the hardware encoder. This format is supported on Maxwell and higher
     * GPUs only and when the 'codecType' is set to NV_IFROGL_HW_ENC_H264.
     * Encoder profile is set to HIGH_444_INTRA (244) when this option
     * is set. NV_IFROGL_HW_ENC_CONFIG::avgBitRate should be ~20%-30% higher
     * than what is otherwise set for NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV420.
     */
    NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV444,
} NV_IFROGL_HW_ENC_INPUT_FORMAT;

/*!
 * Session handle.
 */
typedef struct NV_IFROGL_SESSION_HANDLE_REC
{
} *NV_IFROGL_SESSION_HANDLE;

/*!
 * Transfer object handle.
 */
typedef struct NV_IFROGL_TRANSFEROBJECT_HANDLE_REC
{
} *NV_IFROGL_TRANSFEROBJECT_HANDLE;

/*!
 * Defines the parameters to be used by the transfer to sys object.
 */
typedef struct
{
    /*!
     * See ::NV_IFROGL_TARGET_FORMATS for values.
     */
    NV_IFROGL_TARGET_FORMATS format;
    /*!
     * Flags which can be or'ed together. See ::NV_IFROGL_TRANSFER_OBJECT_FLAGS
     * for values.
     */
    NV_IFROGL_TRANSFER_OBJECT_FLAGS flags;
    /*!
     * Used if 'format' is set to ::NV_IFROGL_TARGET_FORMAT_CUSTOM. Sets the
     * target format for transferred data. All values accepted by the
     * glReadPixels OpenGL command 'format' parameter are valid.
     */
    GLenum customFormat;
    /*!
     * Used if 'format' is set to ::NV_IFROGL_TARGET_FORMAT_CUSTOM. Sets the
     * target type for transferred data. All values accepted by the
     * glReadPixels OpenGL command 'type' parameter are valid.
     */
    GLenum customType;
} NV_IFROGL_TO_SYS_CONFIG;

/*!
 * Defines the encoding configuration to be used by the hardware encoder.
 */
typedef struct
{
    /*!
     * Flags which will be or'ed together. See ::NV_IFROGL_HW_ENC_CONFIG_FLAGS
     * for values.
     */
    NV_IFROGL_HW_ENC_CONFIG_FLAGS flags;
    /*!
     * Selects the hardware encoder profile to use for encode.
     * For H.264 encoder choose:
     *    0: Autoselect appropriate codec profile.
     *   66: Baseline (fastest encode/decode times, lowest image quality,
     *       lowest bitrate)
     *   77: Main (balanced encode/decode times, image
     *  100: High (slowest encode/decode times, highest image quality, highest
     *       bitrate)
     *  244: High, used for lossless and YUV444 encoding.
     * For HEVC encoder choose:
     *    1:  Main Profile.
     */
    unsigned int profile;
    /*!
     * Numerator of the frame rate in frames per second (fps). For example, the
     * "33333" in 33333/1000 (for a frame rate of 33.333 fps)
     */
    unsigned int frameRateNum;
    /*!
     * Denominator of the frame rate in frames per second (fps). For example,
     * the "1000" in 33333/1000 (for a frame rate of 33.333 fps).
     */
    unsigned int frameRateDen;
    /*!
     * Specifies the width of the encoded stream. If the width of the image
     * transferred with ::NvIFROGLTransferFramebufferToHW_ENCEnc is different
     * from the width given here the image is scaled.
     * Width should be an even value when NV_IFROGL_HW_ENC_INPUT_FORMAT is set
     * to NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV420.
     */
    unsigned int width;
    /*!
     * Specifies the height of the encoded images. If the height of the image
     * transferred with ::NvIFROGLTransferFramebufferToHW_ENCEnc is different
     * from the height given here the image is scaled.
     * Height should be an even value when NV_IFROGL_HW_ENC_INPUT_FORMAT is set
     * to NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV420.
     */
    unsigned int height;
    /*!
     * Specifies the maximum width of the encoded image when dynamically changing
     * the resolution.
     * Used when flags is set to NV_IFROGL_HW_ENC_CONFIG_FLAG_DYN_RESOLUTION_CHANGE,
     * else this is ignored ignored.
     * This should be an even value when NV_IFROGL_HW_ENC_INPUT_FORMAT is set
     * to NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV420.
     */
    unsigned int maxDynamicWidth;
    /*!
     * Specifies the maximum height of the encoded image when dynamically changing
     * the resolution.
     * Used when flags is set to NV_IFROGL_HW_ENC_CONFIG_FLAG_DYN_RESOLUTION_CHANGE,
     * else this is ignored ignored.
     * This should be an even value when NV_IFROGL_HW_ENC_INPUT_FORMAT is set
     * to NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV420.
     */
    unsigned int maxDynamicHeight;
    /*!
     * Indicates the rate control method used by the encoder, see
     * ::NV_IFROGL_HW_ENC_RATE_CONTROL for values.
     */
    NV_IFROGL_HW_ENC_RATE_CONTROL rateControl;
    /*!
     * The average bitrate to be targeted by the encoder (see ::NV_IFROGL_HW_ENC_CONFIG::rateControl).
     */
    unsigned int avgBitRate;
    /*!
     * The maximum bitrate allowed (see ::NV_IFROGL_HW_ENC_CONFIG::rateControl). Used for VBR or 
     * CONSTQP rate control.
     */
    unsigned int peakBitRate;
    /*!
     * Group Of Pictures length. This is the interval between Instantaneous
     * Decoder Refresh frames (IDR-frames). Setting this higher will increase
     * encoding efficiency, but will increase the interval between points at
     * which the decoder may start decoding the stream when skipping frames.
     * If this is to zero a value of 30 is taken as default.
     */
    unsigned int GOPLength;
    /*!
     * Quantization parameter used with Constant Quantizer encoding.
     * Quantization reduces the size of the input image data in order to make
     * it more compressible. Lower values result in less quantization of the
     * input frame data, and thus result in better image quality and higher
     * bitrates, while higher values result in more quantization and thus worse
     * image quality and lower bitrates. A value of 26 is a standard default
     * which will provide good video quality at a relatively decent bitrate.
     * This value is used as an initial QP in the case of CBR/VBR and as a
     * target QP in the case of CONST QP rate control mode.
     */
    unsigned int quantizationParam;

    /*!
     * Specifies the stereo format to capture.
     */
    NV_IFROGL_HW_ENC_STEREO_FORMAT stereoFormat;

    /*!
     * Specifies the encoding preset used for fine tuning of the encoding parameters.
     */
    NV_IFROGL_HW_ENC_PRESET preset;

    /*!
     * Specifies the slicing mode used by the encode.
     * Refer to enum NV_IFROGL_HW_ENC_SLICING_MODE for usage.
     */

    NV_IFROGL_HW_ENC_SLICING_MODE slicingMode;

    /*!
     * Specifies the additional parameter used along with slicing mode by the encoder.
     * Refer to enum NV_IFROGL_HW_ENC_SLICING_MODE for usage.
     */
    unsigned int slicingModeParam;
    /*!
     * Set this to NV_IFROGL_BOOL_TRUE if client wants to enable gradual decoder refresh
     * or intra-refresh. If the GOP structure uses B frames, this setting will be ignored.
     */
    NV_IFROGL_BOOL enableIntraRefresh;
    /*!
     * Specifies VBV buffer size in bits.
     * This value is overridden by NvIFROpenGL internally to single frame size, if rate control mode being used is
     * either of NV_IFROGL_HW_ENC_RATE_CONTROL_2_PASS_QUALITY or NV_IFROGL_HW_ENC_RATE_CONTROL_2_PASS_FRAMESIZE_CAP.
     */
    unsigned int VBVBufferSize;
    /*!
     * Specifies the VBV initial delay in bits
     * This value is overridden by NvIFROpenGL internally to single frame size, if rate control mode being used is
     * either of NV_IFROGL_HW_ENC_RATE_CONTROL_2_PASS_QUALITY or NV_IFROGL_HW_ENC_RATE_CONTROL_2_PASS_FRAMESIZE_CAP.
     */
    unsigned int VBVInitialDelay;
    /*!
     * Specifies the DPB size used for encoding. Setting this to 0 will allow encoder to use the default DPB size.
     * The low latency application which wants to invalidate reference frame as an error resilience tool
     * is recommended to use a large DPB size, so that the encoder can keep a large number of old reference frames
     * which can be used in case of invalidated frames.
     */
    unsigned int maxNumRefFrames;
    /*!
     * Specifies maximum QP used by encoder rate control mode. Each of the maxRCQP[0], maxRCQP[1] and
     * maxRCQP[2] are the maximum QP values for P, B and I frame respectively
     */
    unsigned int maxRCQP[3];
    /*!
     * Set this value to NV_IFROGL_BOOL_TRUE if client wants encoder to use maxRCQP[] values
     */ 
    NV_IFROGL_BOOL  useMaxRCQP;
    /*!
     * Specifies if the client wants to fill the SPS PPS header for every IDR frame.
     */
    NV_IFROGL_BOOL  repeatSPSPPSHeader;
    /*!
     * Set this to NV_IFROGL_BOOL_TRUE if the client wants to enable adaptive quantization.
     * Adaptive quantization (AQ) improves subjective quality at the expense of a loss in
     * overall encoding performance. The exact loss in performance is content-dependent.
     * AQ allows the encoder to vary the quantization parameter within a row of macroblocks.
     */
    NV_IFROGL_BOOL enableAQ;
    /*!
     * Specifies the YUV format used as an input during hardware encode.
     */
    NV_IFROGL_HW_ENC_INPUT_FORMAT hwEncInputFormat;
    /*!
     * Specifies the hardware codec used for encoding captured surface.
     */
    NV_IFROGL_HW_ENC_TYPE codecType;
} NV_IFROGL_HW_ENC_CONFIG;

/*!
 * Defines the per-frame encoding parameters.
 */
typedef struct
{
    /*!
     * Flags which will be or'ed together. See ::NV_IFROGL_HW_ENC_PARAM_FLAGS
     * for values.
     */
    NV_IFROGL_HW_ENC_PARAM_FLAGS flags;
    /*!
     * The new average bitrate to be targeted by the encoder (see ::NV_IFROGL_HW_ENC_CONFIG::rateControl).
     * Used when flags is set to NV_IFROGL_HW_ENC_PARAM_FLAG_DYN_BITRATE_CHANGE to adjust bitrate
     * on the fly.
     */
    unsigned int newAvgBitRate;
    /*!
     * The new maximum bitrate allowed (see ::NV_IFROGL_HW_ENC_CONFIG::rateControl). Used when flags
     * is set to NV_IFROGL_HW_ENC_PARAM_FLAG_DYN_BITRATE_CHANGE.
     */
    unsigned int newPeakBitRate;
    /*!
     * Set this to NV_IFROGL_BOOL_TRUE to force an intra refresh with an intra refresh period of intraRefreshCnt
     */
    NV_IFROGL_BOOL startIntraRefresh;
    /*!
     * Specifies the number of frames over which an intra refresh will happen. This value will
     * override the intraRefreshCnt set previously.
     */
    unsigned int intraRefreshCnt;
    /*!
     * Specifies new VBV buffer size in bits. Set to one frame size for low-latency applications.
     * Client is expected to pass new appropriate VBV values.
     */
    unsigned int newVBVBufferSize;
    /*!
     * Specifies new VBV initial delay in bits. Set to one frame size for low-latency applications.
     * Client is expected to pass new appropriate VBV values.
     */
    unsigned int newVBVInitialDelay;
    /*!
     * Input timestamp to be associated with this input picture[optional].
     */
    unsigned int captureTimeStamp;
    /*!
     * Specifies number of encoder reference frames which the client wants to invalidate.
     */
    unsigned int numRefFramesToInvalidate;
    /*!
     * Set this to NV_IFROGL_BOOL_TRUE if client wants encoder reference frames to be invalidated.
     */
    NV_IFROGL_BOOL invalidateRefrenceFrames;
    /*!
     * This specifies an array of timestamps of the encoder reference frames which the client wants to invalidate.
     */
    unsigned int invalidFrameTimeStamp[NVIFROGL_MAX_REF_FRAMES];
    /*!
     * The new width of the encoded stream (see ::NV_IFROGL_HW_ENC_CONFIG::width). Used when flags
     * is set to NV_IFROGL_HW_ENC_PARAM_FLAG_DYN_RESOLUTION_CHANGE.
     * This should be an even value when NV_IFROGL_HW_ENC_INPUT_FORMAT is set
     * to NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV420.
     */
    unsigned int newWidth;
    /*!
     * The new height of the encoded stream (see ::NV_IFROGL_HW_ENC_CONFIG::height). Used when flags
     * is set to NV_IFROGL_HW_ENC_PARAM_FLAG_DYN_RESOLUTION_CHANGE.
     * This should be an even value when NV_IFROGL_HW_ENC_INPUT_FORMAT is set
     * to NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV420.
     */
    unsigned int newHeight;
} NV_IFROGL_HW_ENC_PARAMS;

/*!
 * Debug callback function
 */
typedef void (NVIFROGLAPI *NV_IFROGL_DEBUG_CALLBACK_PROC)(NV_IFROGL_DEBUG_SEVERITY severity, const char *message, void *userParam);

/*!
 * Defines session creation parameters.
 */
typedef struct
{
    /*!
     * Application specific private information passed to NvIFR session.
     */
    const void *privateData;

    /*!
     * Size of the application specific private information passed to NvIFR
     * session.
     */
    unsigned int privateDataSize;
} NV_IFROGL_SESSION_PARAMS;

/*!
 * Defines capabilities for a given encoder.
 */
typedef struct
{
    /*!
     * Client should pass NVIFROGL_VERSION.
     */
    unsigned int                   ifrOGLVersion;
    /*!
     * This specifies the hardware encoder type client passed to
     * NvIFROGLGetHwEncCaps(), fills capabilities for this codec type.
     * Refer to NV_IFROGL_HW_ENC_TYPE for possible values.
     */
    NV_IFROGL_HW_ENC_TYPE          eCodec;
    /*!
     * Specifies whether given codec is supported or not.
     */
    NV_IFROGL_BOOL                 codecSupported;
    /*!
     * Specifies whether YUV444 is supported as an input format or not.
     * Refer to NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV444 for more details.
     */
    NV_IFROGL_BOOL                 yuv444Supported;
    /*!
     * Specifies whether lossless encoding is supported or not. 
     * Refer to NV_IFROGL_HW_ENC_PRESET_LOSSLESS_HP for more details.
     */
    NV_IFROGL_BOOL                 losslessEncodingSupported;
    /*!
     * Specifies whether dynamic bitrate change is supported or not.
     */
    NV_IFROGL_BOOL                 dynamicBitRateSupported;
    /*!
     * Specifies whether dynamic resolution change is supported or not.
     */
    NV_IFROGL_BOOL                 dynamicResolutionSupported;
    /*!
     * Specifies whether custom VBV buffer size is supported or not.
     */
    NV_IFROGL_BOOL                 customVBVBufSizeSupported;
    /*!
     * Specifies whether intra refresh is supported or not.
     */
    NV_IFROGL_BOOL                 intraRefreshSupported;
    /*!
     * Specifies whether reference picture invalidation is supported or not.
     */
    NV_IFROGL_BOOL                 refPicInvalidateSupported;
    /*!
     * Specifies whether rate control mode NV_IFROGL_HW_ENC_RATE_CONTROL_CONSTQP
     * is supported.
     */
    NV_IFROGL_BOOL                 rcConstQPSupported;
    /*!
     * Specifies whether rate control mode NV_IFROGL_HW_ENC_RATE_CONTROL_VBR
     * is supported.
     */
    NV_IFROGL_BOOL                 rcVbrSupported;
    /*!
     * Specifies whether rate control mode NV_IFROGL_HW_ENC_RATE_CONTROL_CBR
     * is supported.
     */
    NV_IFROGL_BOOL                 rcCbrSupported;
    /*!
     * Specifies whether rate control mode NV_IFROGL_HW_ENC_RATE_CONTROL_2_PASS_QUALITY
     * is supported.
     */
    NV_IFROGL_BOOL                 rc2PassQualitySupported;
    /*!
     * Specifies whether rate control mode NV_IFROGL_HW_ENC_RATE_CONTROL_2_PASS_FRAMESIZE_CAP
     * is supported.
     */
    NV_IFROGL_BOOL                 rc2PassFramesizeCapSupported;
    /*!
     * Specifies maximum width of the encoded image that can be supported. 
     */
    unsigned int                   maxEncodeWidthSupported;
    /*!
     * Specifies maximum height of the encoded image that can be supported.
     */
    unsigned int                   maxEncodeHeightSupported;
    /*!
     * Specifies maximum MBs supported per frame. This is the maximum possible
     * value of slicingModeParam when using NV_IFROGL_HW_ENC_SLICING_MODE_FIXED_NUM_MBS.
     * It can be used to derive the largest value of slicingModeParam when using 
     * NV_IFROGL_HW_ENC_SLICING_MODE_FIXED_NUM__NUM_MB_ROWS.
     */
    unsigned int                   maxMBSupported;
    /*!
     * Specifies maximum aggregate throughput in MBs per sec.
     * This can be used to indicate maximum possible value that can be assigned
     * to slicingModeParam when using NV_IFROGL_HW_ENC_SLICING_MODE set as
     * NV_IFROGL_HW_ENC_SLICING_MODE_FIXED_NUM_MB_ROWS.
     */
    unsigned int                   maxMBPerSecSupported;
} NV_HW_ENC_GET_CAPS;


/*!
 * \addtogroup IFROGL_FUNC NvIFROGL Functions
 * @{
 */

/*!
 * Creates an OpenGL IFR session.
 *
 * Creates an OpenGL IFR session and returns a pointer to session handle in
 * the \p *pSessionHandle parameter. The session is associated with the current OpenGL
 * context. Multiple sessions can be created, either for one single OpenGL context
 * or for multiple contexts.
 * The client should start the IFR process by calling this API first.
 * If the IFR session fails, the client must call ::NvIFROGLDestroySession API
 * before exiting.
 *
 * \param [out] pSessionHandle
 *   Pointer to a IFR session handle.
 * \param [in] params
 *   Pointer to NV_IFROGL_SESSION_PARAMS passed to IFR session. Will
 *   be ignored if NULL.
 *
 * \return
 * ::NV_IFROGL_SUCCESS \n
 * ::NV_IFROGL_FAILURE \n
 * ::NV_IFROGL_CLIENT_VERSION_UNSUPPORTED \n
 *
 */
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLCreateSession(NV_IFROGL_SESSION_HANDLE *pSessionHandle, const NV_IFROGL_SESSION_PARAMS *params);

/*!
 * Destroy an OpenGL IFR Session
 *
 * Destroys the IFR session previously created using ::NvIFROGLCreateSession()
 * function.
 *
 * \param [in] sessionHandle
 *   IFR session handle.
 *
 * \return
 * ::NV_IFROGL_SUCCESS \n
 * ::NV_IFROGL_FAILURE
 *
 */
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLDestroySession(NV_IFROGL_SESSION_HANDLE sessionHandle);

/*!
 * Creates a object to transfer data to system memory.
 *
 * A transfer object is used to transfer data from the OpenGL. Multiple transfer
 * objects can be created per session.
 *
 * \param [in] sessionHandle
 *   IFR session handle.
 * \param [in] config
 *   Configuration parameters for the transfer to sys object.
 * \param [out] pTransferObjectHandle
 *   Pointer to a transfer handle used to store the created object.
 *
 * \return
 * ::NV_IFROGL_SUCCESS \n
 * ::NV_IFROGL_FAILURE
 *
 */
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLCreateTransferToSysObject(NV_IFROGL_SESSION_HANDLE sessionHandle,
    const NV_IFROGL_TO_SYS_CONFIG *config,
    NV_IFROGL_TRANSFEROBJECT_HANDLE *pTransferObjectHandle);

/*!
 * Creates a object to transfer data to the hardware encoder.
 *
 * A transfer object is used to transfer data from the OpenGL. Multiple transfer
 * objects can be created per session.
 *
 * \param [in] sessionHandle
 *   IFR session handle.
 * \param [in] config
 *   Configuration parameters for the hardware encoder.
 * \param [out] pTransferObjectHandle
 *   Pointer to a transfer handle used to store the created object.
 *
 * \return
 * ::NV_IFROGL_SUCCESS \n
 * ::NV_IFROGL_FAILURE
 *
 */
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLCreateTransferToHwEncObject(NV_IFROGL_SESSION_HANDLE sessionHandle,
    const NV_IFROGL_HW_ENC_CONFIG *config,
    NV_IFROGL_TRANSFEROBJECT_HANDLE *pTransferObjectHandle);

/*!
 * Destroy a transfer object.
 *
 * Destroys the transfer object previously created using the ::NvIFROGLCreateTransferToSysObject()
 * or ::NvIFROGLCreateTransferToHW_ENCEncObject function.
 *
 * \param [in] transferObjectHandle
 *   Transfer object handle.
 *
 * \return
 * ::NV_IFROGL_SUCCESS \n
 * ::NV_IFROGL_FAILURE
 *
 */
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLDestroyTransferObject(NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle);

/*!
 * Transfer the content of a framebuffer to the system memory.
 *
 * Starts the transfer of the pixel data defined by a framebuffer.
 * This is a non-blocking API and the actual transfer is done asynchronously.
 * When reading from a named (non-zero) framebuffer,
 * the attachment either has to be a render buffer object or a texture
 * of type GL_TEXTURE_RECTANGLE. An OpenGL context where the framebuffer
 * object exits must be current.
 * When reading from the default framebuffer, pixels are read from the drawing
 * surface of the current OpenGL context.
 * In case of capture from default framebuffer, the attachement is
 * Window system provided framebuffer, which can be a Window or a Pbuffer.
 * Window system drawable and an OpenGL context where this drawable exists,
 * must be current.
 *
 * \param [in] transferObjectHandle
 *   Transfer object handle.
 * \param [in] framebuffer
 *   The name of an framebuffer object.
 *   Use '0' to capture from Window system provided default framebuffer.
 * \param [in] attachment
 *   For named framebuffers, the attachment to transfer must be
 *     GL_COLOR_ATTACHMENTi where i may range from zero to the value of
 *                          GL_MAX_COLOR_ATTACHMENTS minus one.
 *   For the default framebuffer, the attachment to transfer must be one of
 *     GL_FRONT_LEFT, GL_FRONT_RIGHT, GL_BACK_LEFT or GL_BACK_RIGHT.
 * \param [in] flags
 *   Flags which can be or'ed together. See ::NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAGS
 *   for values.
 * \param [in] xOffset, yOffset
 *   When the flag ::NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAG_CROP is set, gives the
 *   offset in X and Y at which to start the transfer from the framebuffer,.
 *   When the flag ::NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAG_SCALE is set, this is
 *   ignored.
 * \param [in] width, height
 *   When the flag ::NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAG_CROP is set, gives the
 *   target width and height to grab from the framebuffer.
 *   When the flag ::NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAG_SCALE is set, gives the
 *   target height and width to scale the framebuffer to.
 *
 * \return
 * ::NV_IFROGL_SUCCESS \n
 * ::NV_IFROGL_FAILURE
 *
 */
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLTransferFramebufferToSys(NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle,
    GLuint framebuffer, GLenum attachment, NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAGS flags,
    unsigned int xOffset, unsigned int yOffset, unsigned int width, unsigned int height);

/*!
 * Transfer the content of a framebuffer to the hardware encoder.
 *
 * Starts the transfer of the pixel data defined by a framebuffer.
 * This is a non-blocking API and the actual transfer is done asynchronously.
 * When reading from a named (non-zero) framebuffer,
 * the attachment either has to be a render buffer object or a texture
 * of type GL_TEXTURE_RECTANGLE. An OpenGL context where the framebuffer
 * object exits must be current.
 * When reading from the default framebuffer, pixels are read from the drawing
 * surface of the current OpenGL context.
 * Each transfer creates a bitstream data segment which is stored in a queue.
 * The bitstream data will be retrieved by calling ::NvIFROGLLockTransferData
 * for each transfer. The number of outstanding bitstream data segments is
 * limited by free memory only.
 *
 * \param [in] transferObjectHandle
 *   Transfer object handle.
 * \param [in] encodeParams
 *   Encoding parameters for this image (e.g. dynamic bitrate change). Will
 *   be ignored if NULL.
 * \param [in] framebuffer
 *   The name of an framebuffer object.
 *   Use '0' to capture from Window system provided default framebuffer.
 * \param [in] attachment
 *   For named framebuffers, the attachment to transfer for non-stereoscopic
 *   output or the left buffer to transfer for stereoscopic output must be,
 *     GL_COLOR_ATTACHMENTi where i may range from zero to the value of
 *                          GL_MAX_COLOR_ATTACHMENTS minus one
 *   For the default framebuffer, the attachment to transfer must be one of
 *     GL_FRONT_LEFT, GL_FRONT_RIGHT, GL_BACK_LEFT or GL_BACK_RIGHT.
 * \param [in] stereoAttachmentRight
 *   The attachment to transfer for the right buffer if the transfer is
 *   configured to create stereoscopic output. For named framebuffers, must be
 *     GL_COLOR_ATTACHMENTi where i may range from zero to the value of
 *                          GL_MAX_COLOR_ATTACHMENTS minus one
 *   For the default framebuffer, the attachment to transfer must be one of
 *     GL_FRONT_LEFT, GL_FRONT_RIGHT, GL_BACK_LEFT or GL_BACK_RIGHT.
 *   Will be ignored if the transfer is not configured for stereoscopic
 *   output.
 *
 * \return
 * ::NV_IFROGL_SUCCESS \n
 * ::NV_IFROGL_FAILURE
 *
 */
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLTransferFramebufferToHwEnc(NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle,
    const NV_IFROGL_HW_ENC_PARAMS *encodeParams, GLuint framebuffer, GLenum attachment, GLenum stereoAttachmentRight);

/*!
 * Obtain a pointer to the data transferred.
 *
 * When the data pointer is no longer needed ::NvIFROGLReleaseTransferData
 * needs to be called to release the data store.
 * The data pointer is valid until either ::NvIFROGLReleaseTransferData is called
 * or the transfer object is destroyed by ::NvIFROGLDestroyTransferObject.
 * No OpenGL context needs to be current.
 *
 * \param [in] transferObjectHandle
 *   Transfer object handle.
 * \param [out] size
 *   Returns the size of the transferred data.
 * \param [out] data
 *   Will be set to the start of the transferred data buffer
 *
 * \return
 * ::NV_IFROGL_SUCCESS \n
 * ::NV_IFROGL_FAILURE
 *
 */
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLLockTransferData(NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle, uintptr_t *size, const void **data);

/*!
 * Release access to transferred data.
 *
 * No OpenGL context needs to be current.
 *
 * \param [in] transferObjectHandle
 *   Transfer object handle.
 *
 * \return
 * ::NV_IFROGL_SUCCESS \n
 * ::NV_IFROGL_FAILURE
 *
 */
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLReleaseTransferData(NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle);

/*!
* Obtain a pointer to the SPS PPS header from the hardware encoder.
*
* No OpenGL context needs to be current.
*
* \param [in] transferObjectHandle
*   Transfer object handle.
* \param [out] size
*   Returns the size of the SPS PPS header data
* \param [out] data
*   Will be set to the start of the SPS PPS header data buffer
*
* \return
* ::NV_IFROGL_SUCCESS \n
* ::NV_IFROGL_FAILURE
*
*/
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLGetHwEncSPSPPSHeader(NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle, uintptr_t *size, const void **data);

/*!
 * Returns the error log.
 *
 * This string will be null-terminated. The maximum number of characters that
 * may be written including the null terminator, is specified by \p maxLength.
 * The actual number of characters written into \p errorString, including the
 * null terminator, is returned in \p returnedLength.
 * Error string might be added asynchronously to the error log, therefore the
 * order of error message might differ from the interface call order.
 * Error messages returned in \p errorString are cleared from the internal
 * error log.
 *
 * \param errorString [out]
 *   A pointer to the memory location where the error log should be copied to
 * \param maxLength [in]
 *   The size of the memory location including the null terminator.
 * \param returnedLength [out]
 *   The actual number of characters written including the null terminator
 * \param remainingBytes [out]
 *   The total size of remaining error messages in the internal log, including
 *   the null terminator.
 *
 * \return
 * ::NV_IFROGL_SUCCESS \n
 * ::NV_IFROGL_FAILURE
 *
 */
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLGetError(char *errorString, unsigned int maxLength, unsigned int *returnedLength, unsigned int *remainingBytes);

/*!
 * Codec type to be filled in eCodec of pGetCaps to which caps are queried
 * fills capabilities for this codec type.
 * Returns the HwEnc caps in  \p pGetCaps
 *
 * \param [out] pGetCaps
 *
 * \return
 * ::NV_IFROGL_SUCCESS \n
 * ::NV_IFROGL_FAILURE
 *
 */
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLGetHwEncCaps(NV_HW_ENC_GET_CAPS* pGetCaps, NV_IFROGL_SESSION_HANDLE sessionHandle);
/*!
 * Sets the debug message callback.
 *
 * The callback function will be called each time an error is detected.
 *
 * \param [in] callback
 *   A pointer to the callback function.
 * \param [in] userParam
 *   Will be included it as one of the parameters in each call to the callback
 *   function.
 *
 * \return
 * ::NV_IFROGL_SUCCESS \n
 * ::NV_IFROGL_FAILURE
 *
 */
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLDebugMessageCallback(NV_IFROGL_DEBUG_CALLBACK_PROC callback, void *userParam);

/*!
 * \cond API PFN
 *  Defines API function pointers
 */
typedef NVIFROGLSTATUS (NVIFROGLAPI* PNVIFROGLCREATESESSION)(NV_IFROGL_SESSION_HANDLE* pSessionHandle, const NV_IFROGL_SESSION_PARAMS *params);
typedef NVIFROGLSTATUS (NVIFROGLAPI* PNVIFROGLDESTROYSESSION)(NV_IFROGL_SESSION_HANDLE sessionHandle);
typedef NVIFROGLSTATUS (NVIFROGLAPI* PNVIFROGLCREATETRANSFERTOSYSOBJECT)(NV_IFROGL_SESSION_HANDLE sessionHandle, const NV_IFROGL_TO_SYS_CONFIG *config, NV_IFROGL_TRANSFEROBJECT_HANDLE* pTransferObjectHandle);
typedef NVIFROGLSTATUS (NVIFROGLAPI* PNVIFROGLCREATETRANSFERTOHWENCOBJECT)(NV_IFROGL_SESSION_HANDLE sessionHandle, const NV_IFROGL_HW_ENC_CONFIG *config, NV_IFROGL_TRANSFEROBJECT_HANDLE* pTransferObjectHandle);
typedef NVIFROGLSTATUS (NVIFROGLAPI* PNVIFROGLGETHWENCCAPS)(NV_HW_ENC_GET_CAPS* pGetCaps,NV_IFROGL_SESSION_HANDLE sessionHandle);
typedef NVIFROGLSTATUS (NVIFROGLAPI* PNVIFROGLDESTROYTRANSFEROBJECT)(NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle);
typedef NVIFROGLSTATUS (NVIFROGLAPI* PNVIFROGLTRANSFERFRAMEBUFFERTOSYS)(NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle, GLuint framebuffer, GLenum attachment, NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAGS flags, unsigned int xOffset, unsigned int yOffset, unsigned int width, unsigned int height);
typedef NVIFROGLSTATUS (NVIFROGLAPI* PNVIFROGLTRANSFERFRAMEBUFFERTOHWENC)(NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle, const NV_IFROGL_HW_ENC_PARAMS *encodeParams, GLuint framebuffer, GLenum attachment, GLenum stereoAttachmentRight);
typedef NVIFROGLSTATUS (NVIFROGLAPI* PNVIFROGLLOCKTRANSFERDATA)(NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle, uintptr_t *size, const void **data);
typedef NVIFROGLSTATUS (NVIFROGLAPI* PNVIFROGLRELEASETRANSFERDATA)(NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle);
typedef NVIFROGLSTATUS (NVIFROGLAPI* PNVIFROGLGETHWENCSPSPPSHEADER)(NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle, uintptr_t *size, const void **data);
typedef NVIFROGLSTATUS (NVIFROGLAPI* PNVIFROGLGETERROR)(char *errorString, unsigned int maxLength, unsigned int *returnedLength, unsigned int *remainingBytes);
typedef NVIFROGLSTATUS (NVIFROGLAPI* PNVIFROGLDEBUGMESSAGECALLBACK)(NV_IFROGL_DEBUG_CALLBACK_PROC callback, void *userParam);
/// \endcond

/*! @} */ /* ND IFROGL_FUNC */

/*!
 * \ingroup IFROGL backward compatibility definitions.
 * Deprecated - for backward compatibility only - DO NOt USE.
 * NvIFROpenGL 1.0 onward supports codec agnostic interface.
 */
#define NV_IFROGL_H264_STEREO_NONE              NV_IFROGL_HW_ENC_STEREO_NONE
#define NV_IFROGL_H264_STEREO_COMPRESSED_SBS    NV_IFROGL_HW_ENC_STEREO_COMPRESSED_SBS
#define NV_IFROGL_H264_STEREO_SEI_TB            NV_IFROGL_HW_ENC_STEREO_SEI_TB
#define NV_IFROGL_H264_STEREO_MVC               NV_IFROGL_HW_ENC_STEREO_MVC
#define NV_IFROGL_H264_STEREO_FORMAT            NV_IFROGL_HW_ENC_STEREO_FORMAT

#define  NV_IFROGL_H264_RATE_CONTROL_CONSTQP    NV_IFROGL_HW_ENC_RATE_CONTROL_CONSTQP
#define  NV_IFROGL_H264_RATE_CONTROL_VBR        NV_IFROGL_HW_ENC_RATE_CONTROL_VBR
#define  NV_IFROGL_H264_RATE_CONTROL_CBR        NV_IFROGL_HW_ENC_RATE_CONTROL_CBR
#define  NV_IFROGL_H264_RATE_CONTROL_VBR_MINQP  NV_IFROGL_HW_ENC_RATE_CONTROL_VBR_MINQP
#define  NV_IFROGL_H264_RATE_CONTROL_2_PASS_QUALITY       NV_IFROGL_HW_ENC_RATE_CONTROL_2_PASS_QUALITY
#define  NV_IFROGL_H264_RATE_CONTROL_2_PASS_FRAMESIZE_CAP NV_IFROGL_HW_ENC_RATE_CONTROL_2_PASS_FRAMESIZE_CAP
#define  NV_IFROGL_H264_RATE_CONTROL_CBR_IFRAME_2_PASS    NV_IFROGL_HW_ENC_RATE_CONTROL_CBR_IFRAME_2_PASS
#define  NV_IFROGL_H264_RATE_CONTROL            NV_IFROGL_HW_ENC_RATE_CONTROL

#define  NV_IFROGL_H264_ENC_CONFIG_FLAG_NONE    NV_IFROGL_HW_ENC_CONFIG_FLAG_NONE
#define  NV_IFROGL_H264_ENC_CONFIG_FLAG_DYN_RESOLUTION_CHANGE \
             NV_IFROGL_HW_ENC_CONFIG_FLAG_DYN_RESOLUTION_CHANGE
#define  NV_IFROGL_H264_ENC_CONFIG_FLAGS        NV_IFROGL_HW_ENC_CONFIG_FLAGS

#define  NV_IFROGL_H264_ENC_SLICING_MODE_DISABLED          NV_IFROGL_HW_ENC_SLICING_MODE_DISABLED
#define  NV_IFROGL_H264_ENC_SLICING_MODE_FIXED_NUM_MBS     NV_IFROGL_HW_ENC_SLICING_MODE_FIXED_NUM_MBS
#define  NV_IFROGL_H264_ENC_SLICING_MODE_FIXED_NUM_BYTES   NV_IFROGL_HW_ENC_SLICING_MODE_FIXED_NUM_BYTES
#define  NV_IFROGL_H264_ENC_SLICING_MODE_FIXED_NUM_MB_ROWS NV_IFROGL_HW_ENC_SLICING_MODE_FIXED_NUM_MB_ROWS
#define  NV_IFROGL_H264_ENC_SLICING_MODE_FIXED_NUM_SLICES  NV_IFROGL_HW_ENC_SLICING_MODE_FIXED_NUM_SLICES
#define  NV_IFROGL_H264_ENC_SLICING_MODE                   NV_IFROGL_HW_ENC_SLICING_MODE

#define  NV_IFROGL_H264_ENC_PARAM_FLAG_NONE                  NV_IFROGL_HW_ENC_PARAM_FLAG_NONE
#define  NV_IFROGL_H264_ENC_PARAM_FLAG_DYN_BITRATE_CHANGE    NV_IFROGL_HW_ENC_PARAM_FLAG_DYN_BITRATE_CHANGE
#define  NV_IFROGL_H264_ENC_PARAM_FLAG_FORCEIDR              NV_IFROGL_HW_ENC_PARAM_FLAG_FORCEIDR
#define  NV_IFROGL_H264_ENC_PARAM_FLAG_DYN_RESOLUTION_CHANGE NV_IFROGL_HW_ENC_PARAM_FLAG_DYN_RESOLUTION_CHANGE
#define  NV_IFROGL_H264_ENC_PARAM_FLAGS                      NV_IFROGL_HW_ENC_PARAM_FLAGS

#define  NV_IFROGL_H264_PRESET_LOW_LATENCY_DEFAULT NV_IFROGL_HW_ENC_PRESET_LOW_LATENCY_DEFAULT
#define  NV_IFROGL_H264_PRESET_LOW_LATENCY_HP      NV_IFROGL_HW_ENC_PRESET_LOW_LATENCY_HP
#define  NV_IFROGL_H264_PRESET_LOW_LATENCY_HQ      NV_IFROGL_HW_ENC_PRESET_LOW_LATENCY_HQ
#define  NV_IFROGL_H264_PRESET_LOSSLESS_HP         NV_IFROGL_HW_ENC_PRESET_LOSSLESS_HP
#define  NV_IFROGL_H264_PRESET                     NV_IFROGL_HW_ENC_PRESET

#define  NV_IFROGL_H264_INPUT_FORMAT_YUV420        NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV420
#define  NV_IFROGL_H264_INPUT_FORMAT_YUV444        NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV444
#define  NV_IFROGL_H264_INPUT_FORMAT               NV_IFROGL_HW_ENC_INPUT_FORMAT

#define  NV_IFROGL_H264_ENC_PARAMS                 NV_IFROGL_HW_ENC_PARAMS
#define  NV_IFROGL_H264_ENC_CONFIG                 NV_IFROGL_HW_ENC_CONFIG

#define  h264InputFormat                           hwEncInputFormat

#define  PNVIFROGLCREATETRANSFERTOH264ENCOBJECT    PNVIFROGLCREATETRANSFERTOHWENCOBJECT
#define  PNVIFROGLTRANSFERFRAMEBUFFERTOH264ENC     PNVIFROGLTRANSFERFRAMEBUFFERTOHWENC
#define  PNVIFROGLGETH264ENCSPSPPSHEADER           PNVIFROGLGETHWENCSPSPPSHEADER

/*!
 * \ingroup IFROGL_STRUCTURE
 * NV_IFROGL_API_FUNCTION_LIST
 */
typedef struct
{
    unsigned int                    version;                            //!< [in]: Client should pass NVIFROGL_VERSION.
    unsigned int                    nvIFRLibVersion;                    //!< [out]: Returns the maximum supported interface version of the library.
    PNVIFROGLCREATESESSION          nvIFROGLCreateSession;              //!< [out]: Client should access ::NvIFROGLCreateSession() API through this pointer.
    PNVIFROGLDESTROYSESSION         nvIFROGLDestroySession;             //!< [out]: Client should access ::NvIFROGLDestroySession() API through this pointer.
    PNVIFROGLCREATETRANSFERTOSYSOBJECT nvIFROGLCreateTransferToSysObject; //!< [out]: Client should access ::NvIFROGLCreateTransferToSysObject() API through this pointer.
    PNVIFROGLCREATETRANSFERTOH264ENCOBJECT nvIFROGLCreateTransferToH264EncObject; //!< [out]: Client should access ::NvIFROGLCreateTransferToH264EncObject() API through this pointer.
    PNVIFROGLDESTROYTRANSFEROBJECT  nvIFROGLDestroyTransferObject;      //!< [out]: Client should access ::NvIFROGLDestroyTransferObject() API through this pointer.
    PNVIFROGLTRANSFERFRAMEBUFFERTOSYS nvIFROGLTransferFramebufferToSys; //!< [out]: Client should access ::NvIFROGLTransferFramebufferToSys() API through this pointer.
    PNVIFROGLTRANSFERFRAMEBUFFERTOH264ENC nvIFROGLTransferFramebufferToH264Enc; //!< [out]: Client should access ::NvIFROGLTransferFramebufferToH264Enc() API through this pointer.
    PNVIFROGLLOCKTRANSFERDATA       nvIFROGLLockTransferData;           //!< [out]: Client should access ::NvIFROGLLockTransferData() API through this pointer.
    PNVIFROGLRELEASETRANSFERDATA    nvIFROGLReleaseTransferData;        //!< [out]: Client should access ::NvIFROGLReleaseTransferData() API through this pointer.
    PNVIFROGLGETH264ENCSPSPPSHEADER nvIFROGLGetH264EncSPSPPSHeader;     //!< [out]: Client should access ::NvIFROGLGetH264EncSPSPPSHeader() API through this pointer.
    PNVIFROGLGETERROR               nvIFROGLGetError;                   //!< [out]: Client should access ::NvIFROGLGetError() API through this pointer.
    PNVIFROGLDEBUGMESSAGECALLBACK   nvIFROGLDebugMessageCallback;       //!< [out]: Client should access ::NvIFROGLDebugMessageCallback() API through this pointer.
    PNVIFROGLCREATETRANSFERTOHWENCOBJECT nvIFROGLCreateTransferToHwEncObject; //!< [out]: Client should access ::NvIFROGLCreateTransferToHwEncObject() API through this pointer.
    PNVIFROGLTRANSFERFRAMEBUFFERTOHWENC nvIFROGLTransferFramebufferToHwEnc; //!< [out]: Client should access ::NvIFROGLTransferFramebufferToHwEnc() API through this pointer.
    PNVIFROGLGETHWENCSPSPPSHEADER nvIFROGLGetHwEncSPSPPSHeader;     //!< [out]: Client should access ::NvIFROGLGetHwEncSPSPPSHeader() API through this pointer.
    PNVIFROGLGETHWENCCAPS           nvIFROGLGetHwEncCaps;               //!< [out]: Client should access ::NvIFROGLGetHwEncCaps() API through this pointer.
} NV_IFROGL_API_FUNCTION_LIST;

/*!
 * \ingroup IFROGL_FUNC
 * Entry Point to the NvIFROGL interface.
 *
 * Creates an instance of the NvIFROGL interface, and populates the
 * pFunctionList with function pointers to the API routines implemented by the
 * NvIFROGL interface.
 *
 * \param [out] functionList
 *
 * \return
 * ::NV_IFROGL_SUCCESS \n
 * ::NV_IFROGL_FAILURE
 */
NVIFROGLSTATUS NVIFROGLAPI NvIFROGLCreateInstance(NV_IFROGL_API_FUNCTION_LIST *functionList);

#ifdef __cplusplus
}
#endif

/*! @} NvIFROGL */

#endif // !_NV_NVIFROGL_H_
