/*
 * MIT License
 * 
 * Copyright (c) 2024 Yonder
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __TXTR_H__
#define __TXTR_H__
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>

#include <stdext/cmacros.h>
#include <gxtexture.h>
#include <stb_image_resize2.h>

#ifdef TXTR_IS_SHARED
#define TXTR_EXPORT EXPORT
#else
#define TXTR_EXPORT
#endif

#if defined(TXTR_INCLUDE_DECODE) && !defined(GX_INCLUDE_DECODE)
#error GX decoding capabilities are required
#endif

#if defined(TXTR_INCLUDE_ENCODE) && !defined(GX_INCLUDE_ENCODE)
#error GX encoding capabilities are required
#endif

// From: https://wiki.axiodl.com/w/TXTR_(Metroid_Prime)#Image_Formats
typedef enum TXTRFormat {
    TXTR_TTF_SZ_MIN = 0,
    // Intensity (4 bits intensity)
    TXTR_TTF_I4 = 0,
    // Intensity (8 bits intensity)
    TXTR_TTF_I8,
    // Intensity Alpha (4 bits intensity, 4 bits alpha)
    TXTR_TTF_IA4,
    // Intensity Alpha (8 bits intensity, 8 bits alpha)
    TXTR_TTF_IA8,
    // Color Index (4 bits color index)
    TXTR_TTF_CI4,
    // Color Index (8 bits color index)
    TXTR_TTF_CI8,
    // Color Index (2 bits ignored, 14 bits color index)
    TXTR_TTF_CI14X2,
    // RGB (5 bits RB, 6 bits G)
    TXTR_TTF_R5G6B5,
    // RGBA (1 bit mode, mode = 1: 5 bits RGB, mode = 0: 4 bits RGB, 3 bits alpha)
    TXTR_TTF_RGB5A3,
    // RGBA (2 groups, group 1: 8 bits AR, group 2: 8 bits GB)
    TXTR_TTF_RGBA8,
    // Compressed (8 bytes DXT1 block)
    TXTR_TTF_CMP,
    TXTR_TTF_INVALID = INT_MAX,
    TXTR_TTF_SZ_MAX = INT_MAX
} PACK TXTRFormat_t;

// From: https://wiki.tockdom.com/wiki/Image_Formats#Palette_Formats
typedef enum TXTRPaletteFormat {
    TXTR_TPF_SZ_MIN = 0,
    // Intensity Alpha (8 bits intensity, 8 bits alpha)
    TXTR_TPF_IA8 = 0,
    // RGB (5 bits RB, 6 bits G)
    TXTR_TPF_R5G6B5,
    // RGBA (1 bit mode, mode = 1: 5 bits RGB, mode = 0: 4 bits RGB, 3 bits alpha)
    TXTR_TPF_RGB5A3,
    TXTR_TPF_INVALID = INT_MAX,
    TXTR_TPF_SZ_MAX = INT_MAX
} PACK TXTRPaletteFormat_t;

// From: https://wiki.axiodl.com/w/TXTR_(Metroid_Prime)#Header
typedef struct TXTRHeader {
    // Texture Format
    TXTRFormat_t format; // uint32_t
    // Texture Width
    uint16_t width;
    // Texture Height
    uint16_t height;
    // Texture Mipmap Count (max: 11)
    uint32_t mipCount;
} ALIGN TXTRHeader_t;

// From: https://wiki.axiodl.com/w/TXTR_(Metroid_Prime)#Palettes
typedef struct TXTRPaletteHeader {
    // Palette Format
    TXTRPaletteFormat_t format; // uint32_t
    // Palette Width
    uint16_t width;
    // Palette Height
    uint16_t height;
} ALIGN TXTRPaletteHeader_t;

// Not a literal data structure that maps to a data format --- for API use only!
typedef struct TXTR {
    TXTRHeader_t hdr;
    TXTRPaletteHeader_t palHdr;
    size_t palSz; /* To get actual palette size: palSz * sizeof(uint16_t), uint32_t when decoding */
    uint16_t *pal;
    size_t mipsSz;
    uint8_t *mips;
    bool isIndexed;
} TXTR_t;

TXTR_EXPORT void TXTR_free(TXTR_t *txtr);

TXTR_EXPORT bool TXTR_IsIndexed(TXTRFormat_t texFmt);

TXTR_EXPORT size_t TXTR_CalcMipSz(TXTRFormat_t texFmt, uint16_t width, uint16_t height);

TXTR_EXPORT size_t TXTR_GetMaxPalSz(TXTRFormat_t texFmt);

#ifdef TXTR_INCLUDE_DECODE
typedef struct TXTRDecodeOptions {
    bool flipX;
    bool flipY;
    bool decAllMips;
} TXTRDecodeOptions_t;

// Not a literal data structure that maps to a data format --- for API use only!
typedef struct TXTRMipmap {
    uint16_t width;
    uint16_t height;
    size_t size;
    uint32_t *data; /* To get actual size: dataSz * sizeof(uint32_t) */
} TXTRMipmap_t;

typedef enum TXTRReadError {
    TXTR_RE_SUCCESS = 0,
    TXTR_RE_INVLDPARAMS,
    TXTR_RE_INVLDTEXFMT,
    TXTR_RE_INVLDTEXWIDTH,
    TXTR_RE_INVLDTEXHEIGHT,
    TXTR_RE_INVLDMIPCNT,
    TXTR_RE_INVLDPALFMT,
    TXTR_RE_INVLDPALWIDTH,
    TXTR_RE_INVLDPALHEIGHT,
    TXTR_RE_INVLDPALSZ,
    TXTR_RE_MEMFAILPAL,
    TXTR_RE_MEMFAILMIPS
} TXTRReadError_t;

typedef enum TXTRDecodeError {
    TXTR_DE_SUCCESS = 0,
    TXTR_DE_INVLDPARAMS,
    TXTR_DE_INVLDTEXPAL,
    TXTR_DE_INVLDTEXMIPS,
    TXTR_DE_INVLDTEXWIDTH,
    TXTR_DE_INVLDTEXHEIGHT,
    TXTR_DE_INVLDTEXMIPCNT,
    TXTR_DE_INVLDTEXFMT,
    TXTR_DE_INVLDPALFMT,
    TXTR_DE_MEMFAILPAL,
    TXTR_DE_MEMFAILMIP,
    TXTR_DE_INTERRUPTED,
    TXTR_DE_FAILDECPAL
} TXTRDecodeError_t;

TXTR_EXPORT void TXTRMipmap_free(TXTRMipmap_t *mip);

TXTR_EXPORT TXTRReadError_t TXTR_Read(TXTR_t *txtr, size_t dataSz, uint8_t *data);

TXTR_EXPORT TXTRDecodeError_t TXTR_Decode(TXTR_t *txtr, TXTRMipmap_t mipsOut[11], size_t *mipsOutCount,
TXTRDecodeOptions_t *opts);

TXTR_EXPORT char *TXTRReadError_ToStr(TXTRReadError_t txtrReadError);

TXTR_EXPORT char *TXTRDecodeError_ToStr(TXTRDecodeError_t txtrDecodeError);
#endif

#ifdef TXTR_INCLUDE_ENCODE
typedef struct TXTREncodeOptions {
    bool flipX;
    bool flipY;
    uint8_t mipLimit;
    uint16_t widthLimit;
    uint16_t heightLimit;
    GXAvgType_t avgType;
    int squishFlags;
    size_t squishMetricSz;
    float *squishMetric;
    stbir_edge stbirEdge;
    stbir_filter stbirFilter;
    GXDitherType_t ditherType;
} TXTREncodeOptions_t;

// Not a literal data structure that maps to a data format --- for API use only!
typedef struct TXTRRawMipmap {
    size_t size;
    uint8_t *data;
} TXTRRawMipmap_t;

typedef enum TXTREncodeError {
    TXTR_EE_SUCCESS = 0,
    TXTR_EE_INVLDPARAMS,
    TXTR_EE_INVLDTEXWIDTH,
    TXTR_EE_INVLDTEXHEIGHT,
    TXTR_EE_INVLDTEXMIPLMT,
    TXTR_EE_INVLDTEXWIDTHLMT,
    TXTR_EE_INVLDTEXHEIGHTLMT,
    TXTR_EE_INVLDGXAVGTYPE,
    TXTR_EE_INVLDSTBIREDGEMODE,
    TXTR_EE_INVLDSTBIRFILTER,
    TXTR_EE_MEMFAILSRCPXS,
    TXTR_EE_MEMFAILMIP,
    TXTR_EE_INVLDTEXFMT,
    TXTR_EE_INVLDPALFMT,
    TXTR_EE_MEMFAILPAL,
    TXTR_EE_FAILBUILDPAL,
    TXTR_EE_RESIZEFAIL,
    TXTR_EE_TRYMIPPALFMT,
    TXTR_EE_INTERRUPTED,
    TXTR_EE_FAILENCPAL,
    TXTR_EE_INVLDSQUISHMETRICSZ,
    TXTR_EE_INVLDGXDITHERTYPE
} TXTREncodeError_t;

typedef enum TXTRWriteError {
    TXTR_WE_SUCCESS = 0,
    TXTR_WE_INVLDPARAMS,
    TXTR_WE_INVLDTEXFMT,
    TXTR_WE_INVLDTEXWIDTH,
    TXTR_WE_INVLDTEXHEIGHT,
    TXTR_WE_INVLDMIPCNT,
    TXTR_WE_INVLDPALFMT,
    TXTR_WE_INVLDPALWIDTH,
    TXTR_WE_INVLDPALHEIGHT,
    TXTR_WE_INVLDPALSZ,
    TXTR_WE_INVLDTEXPAL,
    TXTR_WE_INVLDTEXMIPS,
    TXTR_WE_MEMFAILMIPS,
    TXTR_WE_INTERRUPTED
} TXTRWriteError_t;

TXTR_EXPORT void TXTRRawMipmap_free(TXTRRawMipmap_t *mip);

TXTR_EXPORT TXTREncodeError_t TXTR_Encode(TXTRFormat_t texFmt, TXTRPaletteFormat_t palFmt, uint16_t width,
uint16_t height, size_t dataSz, uint32_t *data, TXTR_t *txtr, TXTRRawMipmap_t txtrMips[11],
TXTREncodeOptions_t *opts);

TXTR_EXPORT TXTRWriteError_t TXTR_Write(TXTR_t *txtr, TXTRRawMipmap_t mips[11], size_t *txtrDataSz,
uint8_t **txtrData);

TXTR_EXPORT char *TXTREncodeError_ToStr(TXTREncodeError_t txtrEncodeError);

TXTR_EXPORT char *TXTRWriteError_ToStr(TXTRWriteError_t txtrWriteError);
#endif

#endif
