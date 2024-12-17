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

#ifndef __TGA_H__
#define __TGA_H__
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <stdext/cmacros.h>

// From: https://www.dca.fee.unicamp.br/~martino/disciplinas/ea978/tgaffs.pdf
// From: http://www.paulbourke.net/dataformats/tga/
// From: https://docs.fileformat.com/image/tga/
// From: https://wikipedia.org/wiki/Truevision_TGA
// From: http://fileformats.archiveteam.org/wiki/TGA

#ifdef TGA_IS_SHARED
#define TGA_EXPORT EXPORT
#else
#define TGA_EXPORT
#endif

#define TGA_FOOTERSIG "TRUEVISION-XFILE."
// Starting from end of file and subtracting length of "TRUEVISION-XFILE.\0"
#define TGA_FOOTERSIGOFFS 18

// Note: DEVELOPERS ARE NOT REQUIRED TO READ, WRITE OR USE THE EXTENSION OR DEVELOPER AREAS; THEY ARE OPTIONAL.
// EVEN IF THESE AREAS ARE NOT USED, IT IS RECOMMENDED THAT A TGA FILE FOOTER STILL BE INCLUDED WITH THE FILE.

/* Types *********************************************************************************************************** */
typedef enum TGAColorMapType {
    TGA_CMT_SZ_MIN = 0,
    // Color Map data not present - Field 2 (value 0)
    TGA_CMT_NOCOLORMAP = 0,
    // Color Map data present - Field 2 (value 1)
    TGA_CMT_COLORMAP,
    // Reserved - Field 2 (values 3-127)
    // Not Applicable
    // Developer Extensions - Field 2 (values 128-255)
    // Not Applicable
    TGA_CMT_SZ_MAX = CHAR_MAX
} PACK TGAColorMapType_t;

typedef enum TGAImageType {
    TGA_IMT_SZ_MIN = 0,
    // Image data not present - Field 3 (value 0)
    TGA_IMT_NONE = 0,
    // Uncompressed Color Map data - Field 3 (value 1)
    TGA_IMT_COLORMAP,
    // Uncompressed Color data - Field 3 (value 2)
    TGA_IMT_COLOR,
    // Uncompressed Grayscale data - Field 3 (value 3)
    TGA_IMT_GRAYSCALE,
    // Reserved - Field 3 (values 4-8)
    // Not Applicable
    // Compressed Color Map data - Field 3 (value 9)
    TGA_IMT_COLORMAP_COMPRESSED = 9,
    // Compressed Color data - Field 3 (value 10)
    TGA_IMT_COLOR_COMPRESSED,
    // Compressed Grayscale data - Field 3 (value 11)
    TGA_IMT_GRAYSCALE_COMPRESSED,
    // Reserved - Field 3 (values 12-127)
    // Not Applicable
    // Developer Extensions - Field 3 (values 128-255)
    // Not Applicable
    TGA_IMT_SZ_MAX = CHAR_MAX
} PACK TGAImageType_t;

typedef struct TGAColorMapSpecification {
    // First Entry Index - Field 4.1 (2 bytes)
    uint16_t firstEntryIndex;
    // Color Map Length - Field 4.2 (2 bytes)
    uint16_t colorMapLength;
    // Color Map Entry Size - Field 4.3 (1 byte)
    uint8_t colorMapEntrySize;
} ALIGN TGAColorMapSpecification_t;

typedef uint8_t TGAImageDescriptor_t;

// Macro trickery instead because:
// - va_list rounds up anything smaller than int (-Wvarargs).
// - Interfacing pasted macros to functions would require the setter to need a pointer.
// - Using GNU compound statements is not portable and has its own scope (making it unusable inline).
// - Using bitfields in a struct is not portable, see: https://lwn.net/Articles/478657/
// - Seperate _set and _get functions is ugly and error prone.
// FIXME: Figure out a workaround for the `gnu-zero-variadic-macro-arguments` and `Wc23-extensions` error for these
//        bitfields

// Alpha Bits Per Pixel - Field 5.6 (bits 0 to 3)
#define _TGAImageDescriptor_alphaBitLength_g(d) ((uint8_t) ((d) & 0xF))
#define _TGAImageDescriptor_alphaBitLength_gs(d, v) (d = (d & ~0xF) | ((uint8_t) ((v) & 0xF)))
#define _TGAImageDescriptor_alphaBitLength(d, ...) _TGAImageDescriptor_alphaBitLength_g ## __VA_OPT__(s) ((d) __VA_OPT__(,) __VA_ARGS__)
#define TGAImageDescriptor_alphaBitLength(...) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"") \
    _Pragma("GCC diagnostic ignored \"-WWc23-extensions\"") \
    _TGAImageDescriptor_alphaBitLength(__VA_ARGS__) \
    _Pragma("GCC diagnostic pop")

// Flip X Origin - Field 5.6 (bit 4)
#define _TGAImageDescriptor_flipXOrigin_g(d) ((bool) !!(((d) >> 4) & 0x1))
#define _TGAImageDescriptor_flipXOrigin_gs(d, v) (d = (d & ~0x10) | (((uint8_t) !!(v)) << 4))
#define _TGAImageDescriptor_flipXOrigin(d, ...) _TGAImageDescriptor_flipXOrigin_g ## __VA_OPT__(s) ((d) __VA_OPT__(,) __VA_ARGS__)
#define TGAImageDescriptor_flipXOrigin(...) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"") \
    _Pragma("GCC diagnostic ignored \"-WWc23-extensions\"") \
    _TGAImageDescriptor_flipXOrigin(__VA_ARGS__) \
    _Pragma("GCC diagnostic pop")

// Flip Y Origin - Field 5.6 (bit 5)
#define _TGAImageDescriptor_flipYOrigin_g(d) ((bool) !!(((d) >> 5) & 0x1))
#define _TGAImageDescriptor_flipYOrigin_gs(d, v) (d = (d & ~0x20) | (((uint8_t) !!(v)) << 5))
#define _TGAImageDescriptor_flipYOrigin(d, ...) _TGAImageDescriptor_flipYOrigin_g ## __VA_OPT__(s) ((d) __VA_OPT__(,) __VA_ARGS__)
#define TGAImageDescriptor_flipYOrigin(...) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"") \
    _Pragma("GCC diagnostic ignored \"-WWc23-extensions\"") \
    _TGAImageDescriptor_flipYOrigin(__VA_ARGS__) \
    _Pragma("GCC diagnostic pop")

// Reserved - Field 5.6 (bit 6 and 7)
// Must be 0
#define _TGAImageDescriptor_reserved_g(d) ((uint8_t) (((d) >> 6) & 0x3))
#define _TGAImageDescriptor_reserved_gs(d, v) (d = (d & ~0xC0) | (((uint8_t) ((v) & 0x3)) << 6))
#define _TGAImageDescriptor_reserved(d, ...) _TGAImageDescriptor_reserved_g ## __VA_OPT__(s) ((d) __VA_OPT__(,) __VA_ARGS__)
#define TGAImageDescriptor_reserved(...) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"") \
    _Pragma("GCC diagnostic ignored \"-WWc23-extensions\"") \
    _TGAImageDescriptor_reserved(__VA_ARGS__) \
    _Pragma("GCC diagnostic pop")

FORCE_INLINE TGAImageDescriptor_t TGAImageDescriptor(uint8_t alphaBitLength, bool flipXOrigin, bool flipYOrigin,
uint8_t reserved) {
    TGAImageDescriptor_t imageDesc = (TGAImageDescriptor_t) 0;
    TGAImageDescriptor_alphaBitLength(imageDesc, alphaBitLength);
    TGAImageDescriptor_flipXOrigin(imageDesc, flipXOrigin);
    TGAImageDescriptor_flipYOrigin(imageDesc, flipYOrigin);
    TGAImageDescriptor_reserved(imageDesc, reserved);
    return imageDesc;
}

typedef struct TGAImageSpecification {
    // X-origin of Image - Field 5.1 (2 bytes)
    uint16_t xOrigin;
    // Y-origin of Image - Field 5.2 (2 bytes)
    uint16_t yOrigin;
    // Image Width - Field 5.3 (2 bytes)
    uint16_t width;
    // Image Height - Field 5.4 (2 bytes)
    uint16_t height;
    // Pixel Depth - Field 5.5 (1 byte)
    uint8_t pixelDepth;
    // Image Descriptor - Field 5.6 (1 byte)
    TGAImageDescriptor_t imageDesc;
} ALIGN TGAImageSpecification_t;

typedef struct TGADeveloperEntry {
    // ... TAG is a SHORT value in the range of 0 to 65535. Values from 0 - 32767 are available for developer use,
    // while values from 32768 - 65535 are reserved for Truevision.
    uint16_t tag;
    // ... OFFSET is a LONG value which specifies the number of bytes from the beginning of the file to the start of
    // the field referenced by the tag.
    uint32_t offset;
    // ... FIELD SIZE is a LONG value which specifies the number of bytes in the field.
    uint32_t size;
} ALIGN TGADeveloperEntry_t;

typedef struct TGADeveloperDirectory {
    // The first SHORT in the directory specifies the number of tags which are currently in the directory.
    uint16_t tagCount;
    // The rest of the directory contains a series of TAG, OFFSET and SIZE combinations.
    // Not Applicable
} ALIGN TGADeveloperDirectory_t;

typedef struct TGAAuthorComments {
    // ... four lines of 80 characters, each followed by a null terminator.
    char line1[81];
    char line2[81];
    char line3[81];
    char line4[81];
} ALIGN TGAAuthorComments_t;

typedef struct TGADateTimeStamp {
    // SHORT 0: Month (1 - 12)
    uint16_t month;
    // SHORT 1: Day (1 - 31)
    uint16_t day;
    // SHORT 2: Year (4 digit, ie. 1989)
    uint16_t year;
    // SHORT 3: Hour (0 - 23)
    uint16_t hour;
    // SHORT 4: Minute (0 - 59)
    uint16_t minute;
    // SHORT 5: Second (0 - 59)
    uint16_t second;
} ALIGN TGADateTimeStamp_t;

typedef struct TGAJobTime {
    // SHORT 0: Hours (0 - 65535)
    uint16_t hour;
    // SHORT 1: Minutes (0 - 59)
    uint16_t minute;
    // SHORT 2: Seconds (0 - 59)
    uint16_t second;
} ALIGN TGAJobTime_t;

typedef struct TGASoftwareVersion {
    // SHORT (Bytes 0 - 1): Version Number * 100
    uint16_t number;
    // BYTE (Byte 2): Version Letter
    char letter;
} ALIGN TGASoftwareVersion_t;

typedef struct TGAKeyColor {
    // ... A:R:G:B where 'A' (most significant byte) is the alpha channel key color
    uint8_t alpha;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} ALIGN TGAKeyColor_t;

typedef struct TGAPixelAspectRatio {
    // SHORT 0: Pixel Ratio Numerator (pixel width)
    uint16_t numerator;
    // SHORT 1: Pixel Ratio Denominator (pixel height)
    uint16_t denominator;
} ALIGN TGAPixelAspectRatio_t;

typedef struct TGAGamma {
    // SHORT 0: Gamma Numerator
    uint16_t numerator;
    // SHORT 1: Gamma Denominator
    uint16_t denominator;
} ALIGN TGAGama_t;

typedef enum TGAAttributesType {
    TGA_ATT_SZ_MIN = 0,
    // no Alpha data included (bits 3-0 of field 5.6 should also be set to zero) - Field 24 (value 0)
    TGA_ATT_NOALPHA = 0,
    // undefined data in the Alpha field, can be ignored - Field 24 (value 1)
    TGA_ATT_IGNORABLEUNKNOWN,
    // undefined data in the Alpha field, but should be retained - Field 24 (value 2)
    TGA_ATT_UNKNOWN,
    // useful Alpha channel data is present - Field 24 (value 3)
    TGA_ATT_ALPHA,
    // pre-multiplied Alpha (see description below) - Field 24 (value 4)
    TGA_ATT_PREMULTIPLIEDALPHA,
    // Reserved - Field 24 (values 5-127)
    // Not Applicable
    // Un-assigned - Field 24 (values 128-255)
    // Not Applicable
    TGA_ATT_SZ_MAX = CHAR_MAX
} PACK TGAAttributesType_t;

/* Main ************************************************************************************************************ */
// TGA FILE HEADER
typedef struct TGAFileHeader {
    // ID Length - Field 1 (1 byte)
    uint8_t idLength;
    // Color Map Type - Field 2 (1 byte)
    TGAColorMapType_t colorMapType;
    // Image Type - Field 3 (1 byte)
    TGAImageType_t imageType;
    // Color Map Specification - Field 4 (5 bytes)
    TGAColorMapSpecification_t colorMapSpec;
    // Image Specification - Field 5 (10 bytes)
    TGAImageSpecification_t imageSpec;
} ALIGN TGAFileHeader_t;

// IMAGE/COLOR MAP DATA
// Image ID - Field 6 (variable)
// Not Applicable

// Color Map Data - Field 7 (variable)
// Not Applicable

// Image Data - Field 8 (variable)
// Not Applicable

// DEVELOPER AREA
// Developer Data - Field 9 (variable)
typedef struct TGADeveloperArea {
    // First is a directory of tags that specify where each developer field is at in the file.
    TGADeveloperDirectory_t directory;
    // Then the actual data of each developer field follows based on each tag's offset and size
    // Not Applicable
} ALIGN TGADeveloperArea_t;

// EXTENSION AREA
typedef struct TGAExtensionArea {
    // Extension Size - Field 10 (2 Bytes)
    // For Version 2.0 of the TGA File Format, this number should be set to 495.
    uint16_t size;
    // Author Name - Field 11 (41 Bytes)
    char author[41];
    // Author Comments - Field 12 (324 Bytes)
    TGAAuthorComments_t authComments;
    // Date/Time Stamp - Field 13 (12 Bytes)
    TGADateTimeStamp_t dateTime;
    // Job Name/ID - Field 14 (41 Bytes)
    char job[41];
    // Job Time - Field 15 (6 Bytes)
    TGAJobTime_t jobTime;
    // Software ID - Field 16 (41 Bytes)
    char softId[41];
    // Software Version - Field 17 (3 Bytes)
    TGASoftwareVersion_t softVersion;
    // Key Color - Field 18 (4 Bytes)
    TGAKeyColor_t keyColor;
    // Pixel Aspect Ratio - Field 19 (4 Bytes)
    TGAPixelAspectRatio_t pixAspRatio;
    // Gamma Value - Field 20 (4 Bytes)
    TGAGama_t gamma;
    // Color Correction Offset - Field 21 (4 Bytes)
    uint32_t clrCorctOffs;
    // Postage Stamp Offset - Field 22 (4 Bytes)
    uint32_t postStmpOffs;
    // Scan Line Offset - Field 23 (4 Bytes)
    uint32_t scanLineOffs;
    // Attributes Type - Field 24 (1 Byte)
    TGAAttributesType_t attrsType;
} ALIGN TGAExtensionArea_t;

// Scan Line Table - Field 25 (Variable)
// Not Applicable

// Postage Stamp Image - Field 26 (Variable)
// Not Applicable

// Color Correction Table - Field 27 (2K Bytes)
// Not Applicable

// TGA FILE FOOTER
typedef struct TGAFileFooter {
    // Byte 0-3 - Extension Area Offset - Field 28
    uint32_t extAreaOffs;
    // Byte 4-7 - Developer Directory Offset - Field 29
    uint32_t devAreaOffs;
    // Byte 8-23 - Signature - Field 30
    // Byte 24 - Reserved Character - Field 31
    // Byte 25 - Binary Zero String Terminator - Field 32
    char signature[18];
} ALIGN TGAFileFooter_t;

// Not a literal data structure that maps to a data format --- for API use only!
typedef struct TGA {
    TGAFileHeader_t hdr;
    char *id;
    size_t dataSz; /* To get actual size: dataSz * sizeof(uint32_t) */
    uint32_t *data;
    bool isNewFmt;
    TGAFileFooter_t ftr;
} TGA_t;

#ifdef TGA_INCLUDE_DECODE
typedef enum TGAReadError {
    TGA_RE_SUCCESS = 0,
    TGA_RE_INVLDPARAMS,
    TGA_RE_CLRMAPPRESENT,
    TGA_RE_NOTACOLORTGA,
    TGA_RE_INVLDXORIGIN,
    TGA_RE_INVLDYORIGIN,
    TGA_RE_INVLDWIDTH,
    TGA_RE_INVLDHEIGHT,
    TGA_RE_INVLDPXLDEP,
    TGA_RE_INVLDALPHBITSZ,
    TGA_RE_MEMFAILID,
    TGA_RE_MEMFAILDATA
} TGAReadError_t;
#endif

#ifdef TGA_INCLUDE_ENCODE
typedef enum TGAWriteError {
    TGA_WE_SUCCESS = 0,
    TGA_WE_INVLDPARAMS,
    TGA_WE_CLRMAPPRESENT,
    TGA_WE_NOTACOLORTGA,
    TGA_WE_INVLDXORIGIN,
    TGA_WE_INVLDYORIGIN,
    TGA_WE_INVLDWIDTH,
    TGA_WE_INVLDHEIGHT,
    TGA_WE_INVLDPXLDEP,
    TGA_WE_INVLDALPHBITSZ,
    TGA_WE_MEMFAILDATA,
    TGA_WE_INVLDDATA,
    TGA_WE_INVLDID,
    TGA_WE_INVLDSIGNATURE
} TGAWriteError_t;
#endif

TGA_EXPORT void TGA_free(TGA_t *tga);

TGA_EXPORT bool TGA_IsNewFormat(size_t dataSz, uint8_t *data);

#ifdef TGA_INCLUDE_DECODE
TGA_EXPORT TGAReadError_t TGA_Read(TGA_t *tga, size_t dataSz, uint8_t *data);

TGA_EXPORT char *TGAReadError_ToStr(TGAReadError_t tgaReadError);
#endif

#ifdef TGA_INCLUDE_ENCODE
TGA_EXPORT TGAWriteError_t TGA_Write(TGA_t *tga, size_t *tgaDataSz, uint8_t **tgaData);

TGA_EXPORT char *TGAWriteError_ToStr(TGAWriteError_t tgaWriteError);
#endif

#endif
