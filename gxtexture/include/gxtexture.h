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

#ifndef __GXTEXTURE_H__
#define __GXTEXTURE_H__
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>

#include <stdext/cmacros.h>
#include <stdext/cdata.h>
#include <stdext/cmath.h>
#include <squish.h>

// GX texture data transcoding library
// Based off the work and info from:
// - Custom Mario Kart Wiiki: https://wiki.tockdom.com/wiki/Image_Formats
// - Metaforce: https://github.com/AxioDL/metaforce/blob/a413a010b50a53ebc6b0c726203181fc179d3370/DataSpec/DNACommon/TXTR.cpp
// - noclip.website: https://github.com/magcius/noclip.website/blob/6c47bb40e2c00ab22612f7f488eda0090c7a3be9/rust/src/gx_texture.rs
// - Luigis-Mansion-Blender-Toolkit: https://github.com/Astral-C/Luigis-Mansion-Blender-Toolkit/blob/3c596b1f64d7d506ed37e131cec6f95ec8df3766/gx_texture.py
// - PrimeWorldEditor: https://github.com/AxioDL/PrimeWorldEditor/blob/e5d1678ff626fd16c7160332e1e59003804f970a/src/Core/Resource/Factory/CTextureDecoder.cpp

#define GX_I4_BW 8
#define GX_I4_BH 8
#define GX_I4_BPP 4

#define GX_I8_BW 8
#define GX_I8_BH 4
#define GX_I8_BPP 8

#define GX_IA4_BW 8
#define GX_IA4_BH 4
#define GX_IA4_BPP 8

#define GX_IA8_BW 4
#define GX_IA8_BH 4
#define GX_IA8_BPP 16

#define GX_CI4_BW 8
#define GX_CI4_BH 8
#define GX_CI4_BPP 4
#define GX_CI4_PMUL 4

#define GX_CI8_BW 8
#define GX_CI8_BH 4
#define GX_CI8_BPP 8
#define GX_CI8_PMUL 8

#define GX_CI14X2_BW 4
#define GX_CI14X2_BH 4
#define GX_CI14X2_BPP 16
#define GX_CI14X2_PMUL 14

#define GX_R5G6B5_BW 4
#define GX_R5G6B5_BH 4
#define GX_R5G6B5_BPP 16

#define GX_RGB5A3_BW 4
#define GX_RGB5A3_BH 4
#define GX_RGB5A3_BPP 16

#define GX_RGBA8_BW 4
#define GX_RGBA8_BH 4
#define GX_RGBA8_BPP 32

#define GX_CMP_BW 8
#define GX_CMP_BH 8
#define GX_CMP_BPP 4

FORCE_INLINE size_t GX_CalcMipSz(uint16_t w, uint16_t h, uint8_t bpp) {
    switch (bpp) {
        case 4: // 4bpp, 8bw, 8bh
            return (Dat_AlignU32(w, 8) * Dat_AlignU32(h, 8)) / 2;
        case 8: // 8bpp, 8bw, 4bh
            return  Dat_AlignU32(w, 8) * Dat_AlignU32(h, 4);
        case 16: // 16bpp, 4bw, 4bh
            return (Dat_AlignU32(w, 4) * Dat_AlignU32(h, 4)) * 2;
        case 32: // 32bpp, 4bw, 4bh
            return (Dat_AlignU32(w, 4) * Dat_AlignU32(h, 4)) * 4;
        default:
            return 0;
    }
}

FORCE_INLINE size_t GX_GetMaxPalSz(uint8_t bpp) {
    switch (bpp) {
        case GX_CI4_BPP:
            return (1 << GX_CI4_PMUL);
        case GX_CI8_BPP:
            return (1 << GX_CI8_PMUL);
        case GX_CI14X2_BPP:
            return (1 << GX_CI14X2_PMUL);
        default:
            return 0;
    }
}

#ifdef GX_INCLUDE_DECODE
typedef struct GXDecodeOptions {
    bool flipX;
    bool flipY;
} GXDecodeOptions_t;

typedef size_t (*GX_Decode)(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts);

typedef size_t (*GX_DecodeCI)(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t palSz, uint32_t *pal,
size_t outSz, uint32_t *out, GXDecodeOptions_t *opts);

typedef bool (*GX_DecodePalette)(size_t palSz, uint16_t *pal, uint32_t *palOut, GXDecodeOptions_t *opts);

// From: https://github.com/AxioDL/metaforce/blob/a413a010b50a53ebc6b0c726203181fc179d3370/DataSpec/DNACommon/TXTR.cpp#L71
/* Swizzle bits: 00000123 -> 12312312 */
FORCE_INLINE uint32_t Dat_Convert3To8(uint32_t _v) {
    uint32_t v = _v & 0x7;
    return (v << 5) | (v << 2) | (v >> 1);
}

/* Swizzle bits: 00001234 -> 12341234 */
FORCE_INLINE uint32_t Dat_Convert4To8(uint32_t _v) {
    uint32_t v = _v & 0xF;
    return (v << 4) | v;
}

/* Swizzle bits: 00012345 -> 12345123 */
FORCE_INLINE uint32_t Dat_Convert5To8(uint32_t _v) {
    uint32_t v = _v & 0x1F;
    return (v << 3) | (v >> 2);
}

/* Swizzle bits: 00123456 -> 12345612 */
FORCE_INLINE uint32_t Dat_Convert6To8(uint32_t _v) {
    uint32_t v = _v & 0x3F;
    return (v << 2) | (v >> 4);
}

FORCE_INLINE uint32_t GX_DecodeI4Nibble(uint8_t inp, uint8_t n, GXDecodeOptions_t *opts) {
    FAKEREF(opts);
    
    uint32_t p = inp;
    uint32_t i = Dat_Convert4To8((p >> (n ? 0 : 4)));
    uint32_t a = 0xFF;
    return (
          (i << GX_COMP_SH_B) /* B */
        | (i << GX_COMP_SH_G) /* G */
        | (i << GX_COMP_SH_R) /* R */
        | (a << GX_COMP_SH_A) /* A */
    );
}

FORCE_INLINE uint32_t GX_DecodeI8Pixel(uint8_t inp, GXDecodeOptions_t *opts) {
    FAKEREF(opts);
    
    uint32_t i = inp;
    uint32_t a = 0xFF;
    return (
          (i << GX_COMP_SH_B) /* B */
        | (i << GX_COMP_SH_G) /* G */
        | (i << GX_COMP_SH_R) /* R */
        | (a << GX_COMP_SH_A) /* A */
    );
}

FORCE_INLINE uint32_t GX_DecodeIA4Pixel(uint8_t inp, GXDecodeOptions_t *opts) {
    FAKEREF(opts);
    
    uint32_t p = inp;
    uint32_t i = Dat_Convert4To8((p >> 0));
    uint32_t a = Dat_Convert4To8((p >> 4));
    return (
          (i << GX_COMP_SH_B) /* B */
        | (i << GX_COMP_SH_G) /* G */
        | (i << GX_COMP_SH_R) /* R */
        | (a << GX_COMP_SH_A) /* A */
    );
}

FORCE_INLINE uint32_t GX_DecodeIA8Pixel(uint16_t inp, GXDecodeOptions_t *opts) {
    FAKEREF(opts);
    
    uint32_t p = inp;
    uint32_t i = (p >> 0) & 0xFF;
    uint32_t a = (p >> 8) & 0xFF;
    return (
          (i << GX_COMP_SH_B) /* B */
        | (i << GX_COMP_SH_G) /* G */
        | (i << GX_COMP_SH_R) /* R */
        | (a << GX_COMP_SH_A) /* A */
    );
}

FORCE_INLINE uint32_t GX_DecodeCI4Nibble(uint8_t ini, uint8_t n, GXDecodeOptions_t *opts) {
    FAKEREF(opts);
    
    return (n ? ini : (ini >> 4)) & 0xF; /* Index */
}

FORCE_INLINE uint32_t GX_DecodeCI8Index(uint8_t ini, GXDecodeOptions_t *opts) {
    FAKEREF(opts);
    
    return ini & 0xFF; /* Index */
}

FORCE_INLINE uint32_t GX_DecodeCI14X2Index(uint16_t ini, GXDecodeOptions_t *opts) {
    FAKEREF(opts);
    
    return ini & 0x3FFF; /* Index */
}

FORCE_INLINE uint32_t GX_DecodeR5G6B5Pixel(uint16_t inp, GXDecodeOptions_t *opts) {
    FAKEREF(opts);
    
    uint32_t p = inp;
    uint32_t b = Dat_Convert5To8((p >> 0 ));
    uint32_t g = Dat_Convert6To8((p >> 5 ));
    uint32_t r = Dat_Convert5To8((p >> 11));
    uint32_t a = 0xFF;
    return (
          (b << GX_COMP_SH_B) /* B */
        | (g << GX_COMP_SH_G) /* G */
        | (r << GX_COMP_SH_R) /* R */
        | (a << GX_COMP_SH_A) /* A */
    );
}

FORCE_INLINE uint32_t GX_DecodeRGB5A3Pixel(uint16_t inp, GXDecodeOptions_t *opts) {
    FAKEREF(opts);
    
    uint32_t p = inp;
    if ((p >> 15) & 0x1) {
        /* R5G5B5 */
        uint32_t b = Dat_Convert5To8((p >> 0 ));
        uint32_t g = Dat_Convert5To8((p >> 5 ));
        uint32_t r = Dat_Convert5To8((p >> 10));
        uint32_t a = 0xFF;
        return (
              (b << GX_COMP_SH_B) /* B */
            | (g << GX_COMP_SH_G) /* G */
            | (r << GX_COMP_SH_R) /* R */
            | (a << GX_COMP_SH_A) /* A */
        );
    } else {
        /* R4G4B4A3 */
        uint32_t b = Dat_Convert4To8((p >> 0 ));
        uint32_t g = Dat_Convert4To8((p >> 4 ));
        uint32_t r = Dat_Convert4To8((p >> 8 ));
        uint32_t a = Dat_Convert3To8((p >> 12));
        return (
              (b << GX_COMP_SH_B) /* B */
            | (g << GX_COMP_SH_G) /* G */
            | (r << GX_COMP_SH_R) /* R */
            | (a << GX_COMP_SH_A) /* A */
        );
    }
}

FORCE_INLINE uint32_t GX_DecodeRGBA8Group(uint16_t inp, uint8_t grp, uint32_t prv, GXDecodeOptions_t *opts) {
    FAKEREF(opts);
    
    uint32_t p = inp;
    if (grp) {
        /* GB */
        uint32_t b = (p >> 0) & 0xFF;
        uint32_t g = (p >> 8) & 0xFF;
        return prv | (
              (b << GX_COMP_SH_B) /* B */
            | (g << GX_COMP_SH_G) /* G */
              /* N/A */           /* R */
              /* N/A */           /* A */
        );
    } else {
        /* AR */
        uint32_t r = (p >> 0) & 0xFF;
        uint32_t a = (p >> 8) & 0xFF;
        return 0 | (
              /* N/A */           /* B */
              /* N/A */           /* G */
              (r << GX_COMP_SH_R) /* R */
            | (a << GX_COMP_SH_A) /* A */
        );
    }
}

size_t GX_DecodeI4(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts);

size_t GX_DecodeI8(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts);

size_t GX_DecodeIA4(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts);

size_t GX_DecodeIA8(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts);

size_t GX_DecodeCI4(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t palSz, uint32_t *pal, size_t outSz,
uint32_t *out, GXDecodeOptions_t *opts);

size_t GX_DecodeCI8(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t palSz, uint32_t *pal, size_t outSz,
uint32_t *out, GXDecodeOptions_t *opts);

size_t GX_DecodeCI14X2(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t palSz, uint32_t *pal, size_t outSz,
uint32_t *out, GXDecodeOptions_t *opts);

size_t GX_DecodeR5G6B5(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts);

size_t GX_DecodeRGB5A3(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts);

size_t GX_DecodeRGBA8(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts);

size_t GX_DecodeCMP(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts);

bool GX_DecodePaletteIA8(size_t palSz, uint16_t *pal, uint32_t *palOut, GXDecodeOptions_t *opts);

bool GX_DecodePaletteR5G6B5(size_t palSz, uint16_t *pal, uint32_t *palOut, GXDecodeOptions_t *opts);

bool GX_DecodePaletteRGB5A3(size_t palSz, uint16_t *pal, uint32_t *palOut, GXDecodeOptions_t *opts);
#endif

#ifdef GX_INCLUDE_ENCODE
typedef enum GXAvgType {
    GX_AT_MIN = 0,
    GX_AT_AVERAGE = GX_AT_MIN,
    GX_AT_SQUARED,
    GX_AT_W3C,
    GX_AT_SRGB,
    GX_AT_MAX = GX_AT_SRGB,
    GX_AT_INVALID = GX_AT_MAX + 1
} GXAvgType_t;

typedef enum GXDitherType {
    GX_DT_MIN = 0,
    GX_DT_THRESHOLD = GX_DT_MIN,
    GX_DT_FLOYD_STEINBERG,
    GX_DT_ATKINSON,
    GX_DT_JARVIS_JUDICE_NINKE,
    GX_DT_STUCKI,
    GX_DT_BURKES,
    GX_DT_TWO_ROW_SIERRA,
    GX_DT_SIERRA,
    GX_DT_SIERRA_LITE,
    GX_DT_MAX = GX_DT_SIERRA_LITE,
    GX_DT_INVALID = GX_DT_MAX + 1
} GXDitherType_t;

typedef struct GXEncodeOptions {
    bool flipX;
    bool flipY;
    GXAvgType_t avgType;
    GXDitherType_t ditherType;
    int squishFlags;
    size_t squishMetricSz;
    float *squishMetric;
} GXEncodeOptions_t;

typedef size_t (*GX_Encode)(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts);

typedef size_t (*GX_EncodeCI)(uint16_t w, uint16_t h, size_t inIdxSz, uint32_t *inIdx, size_t palSz, size_t outIdxSz,
uint8_t *outIdx, GXEncodeOptions_t *opts);

typedef bool (*GX_EncodePalette)(size_t palSz, uint32_t *pal, uint16_t *palOut, GXEncodeOptions_t *opts);

// From: https://github.com/AxioDL/metaforce/blob/a413a010b50a53ebc6b0c726203181fc179d3370/DataSpec/DNACommon/TXTR.cpp#L71
/* Swizzle bits: 00000123 <- 12312312 */
FORCE_INLINE uint16_t Dat_Convert8To3(uint32_t _v) {
    uint16_t v = _v & 0xFF;
    return (v >> 5);
}

/* Swizzle bits: 00001234 <- 12341234 */
FORCE_INLINE uint8_t Dat_Convert8To4(uint32_t _v) {
    uint16_t v = _v & 0xFF;
    return (v >> 4);
}

/* Swizzle bits: 00012345 <- 12345123 */
FORCE_INLINE uint16_t Dat_Convert8To5(uint32_t _v) {
    uint16_t v = _v & 0xFF;
    return (v >> 3);
}

/* Swizzle bits: 00123456 <- 12345612 */
FORCE_INLINE uint16_t Dat_Convert8To6(uint32_t _v) {
    uint16_t v = _v & 0xFF;
    return (v >> 2);
}

FORCE_INLINE uint8_t Clr_Average(uint32_t p, GXEncodeOptions_t *opts) {
    uint32_t b = (p >> GX_COMP_SH_B) & 0xFF;
    uint32_t g = (p >> GX_COMP_SH_G) & 0xFF;
    uint32_t r = (p >> GX_COMP_SH_R) & 0xFF;
    uint32_t i = 0;
    switch (opts->avgType)
    {
        case GX_AT_SRGB:
            i = ((r * 2126) + (g * 7152) + (b * 722)) / 10000;
            break;
        case GX_AT_W3C:
            i = ((r * 299) + (g * 587) + (b * 114)) / 1000;
            break;
        case GX_AT_SQUARED:
            i =  (r * r) + (b * b) + (g * g);
            i /= 3;
            i = (uint32_t) sqrt(i);
            break;
        case GX_AT_AVERAGE:
        default:
            i = r + b + g;
            i /= 3;
            break;
    }
    return (uint8_t) Math_MinU32(i, (uint32_t) 0xFF);
}

FORCE_INLINE uint8_t GX_EncodeI4Nibble(uint32_t inp, uint8_t n, uint8_t prv, GXEncodeOptions_t *opts) {
    uint8_t i = Dat_Convert8To4(Clr_Average(inp, opts));
    if (n)
        return prv | i; /* RGB */
    else
        return 0 | (i << 4); /* RGB */
}

FORCE_INLINE uint8_t GX_EncodeI8Pixel(uint32_t inp, GXEncodeOptions_t *opts) {
    uint8_t i = Clr_Average(inp, opts);
    return i; /* RGB */
}

FORCE_INLINE uint8_t GX_EncodeIA4Pixel(uint32_t inp, GXEncodeOptions_t *opts) {
    uint8_t a = Dat_Convert8To4((inp >> GX_COMP_SH_A) & 0xFF);
    uint8_t i = Dat_Convert8To4(Clr_Average(inp, opts));
    return (
          (a << 4) /* A */
        | (i << 0) /* RGB */
    ); 
}

FORCE_INLINE uint16_t GX_EncodeIA8Pixel(uint32_t inp, GXEncodeOptions_t *opts) {
    uint16_t a = (inp >> GX_COMP_SH_A) & 0xFF;
    uint16_t i = Clr_Average(inp, opts);
    return (
          (a << 8) /* A */
        | (i << 0) /* RGB */
    ); 
}

FORCE_INLINE uint8_t GX_EncodeCI4Nibble(uint32_t ini, uint8_t n, uint8_t prv, GXEncodeOptions_t *opts) {
    FAKEREF(opts);
    
    uint8_t i = ini & 0xF;
    if (n)
        return prv | i; /* Index */
    else
        return 0 | (i << 4); /* Index */
}

FORCE_INLINE uint8_t GX_EncodeCI8Index(uint32_t ini, GXEncodeOptions_t *opts) {
    FAKEREF(opts);
    
    return ini & 0xFF; /* Index */
}

FORCE_INLINE uint16_t GX_EncodeCI14X2Index(uint32_t ini, GXEncodeOptions_t *opts) {
    FAKEREF(opts);
    
    return ini & 0x3FFF; /* Index */
}

FORCE_INLINE uint16_t GX_EncodeR5G6B5Pixel(uint32_t inp, GXEncodeOptions_t *opts) {
    FAKEREF(opts);
    
    uint16_t r = Dat_Convert8To5((inp >> GX_COMP_SH_R) & 0xFF);
    uint16_t g = Dat_Convert8To6((inp >> GX_COMP_SH_G) & 0xFF);
    uint16_t b = Dat_Convert8To5((inp >> GX_COMP_SH_B) & 0xFF);
    return (
          /* None */ /* A */
          (r << 11)  /* R */
        | (g << 5 )  /* G */
        | (b << 0 )  /* B */
    ); 
}

FORCE_INLINE uint16_t GX_EncodeRGB5A3Pixel(uint32_t inp, GXEncodeOptions_t *opts) {
    FAKEREF(opts);
    
    // Everything >= 7 alpha will decode as 255
    uint32_t m = (inp >> GX_COMP_SH_A) & 0xFF;
    if (m >= 7) {
        // RGB5
        uint16_t r = Dat_Convert8To5((inp >> GX_COMP_SH_R) & 0xFF);
        uint16_t g = Dat_Convert8To5((inp >> GX_COMP_SH_G) & 0xFF);
        uint16_t b = Dat_Convert8To5((inp >> GX_COMP_SH_B) & 0xFF);
        return 0x8000 | (
              /* None */ /* A */
              (r << 10)  /* R */
            | (g << 5 )  /* G */
            | (b << 0 )  /* B */
        ); 
    } else {
        // RGB4A3
        uint16_t a = Dat_Convert8To3((inp >> GX_COMP_SH_A) & 0xFF);
        uint16_t r = Dat_Convert8To4((inp >> GX_COMP_SH_R) & 0xFF);
        uint16_t g = Dat_Convert8To4((inp >> GX_COMP_SH_G) & 0xFF);
        uint16_t b = Dat_Convert8To4((inp >> GX_COMP_SH_B) & 0xFF);
        return 0x0000 | (
              (a << 12) /* A */
            | (r << 8 ) /* R */
            | (g << 4 ) /* G */
            | (b << 0 ) /* B */
        ); 
    }
}

FORCE_INLINE uint16_t GX_EncodeRGBA8Group(uint32_t inp, uint8_t g, GXEncodeOptions_t *opts) {
    FAKEREF(opts);
    
    if (g) {
        /* GB */
        uint16_t b = (inp >> GX_COMP_SH_B) & 0xFF;
        uint16_t g = (inp >> GX_COMP_SH_G) & 0xFF;
        return (
              /* None */ /* A */
              /* None */ /* R */
              (g << 8)   /* G */
            | (b << 0)   /* B */
        );
    } else {
        /* AR */
        uint16_t a = (inp >> GX_COMP_SH_A) & 0xFF;
        uint16_t r = (inp >> GX_COMP_SH_R) & 0xFF;
        return (
              (a << 8) /* A */
            | (r << 0) /* R */
            /* None */ /* G */
            /* None */ /* B */
        );
    }
}

size_t GX_EncodeI4(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts);

size_t GX_EncodeI8(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts);

size_t GX_EncodeIA4(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts);

size_t GX_EncodeIA8(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts);

size_t GX_EncodeCI4(uint16_t w, uint16_t h, size_t inIdxSz, uint32_t *inIdx, size_t palSz, size_t outIdxSz,
uint8_t *outIdx, GXEncodeOptions_t *opts);

size_t GX_EncodeCI8(uint16_t w, uint16_t h, size_t inIdxSz, uint32_t *inIdx, size_t palSz, size_t outIdxSz,
uint8_t *outIdx, GXEncodeOptions_t *opts);

size_t GX_EncodeCI14X2(uint16_t w, uint16_t h, size_t inIdxSz, uint32_t *inIdx, size_t palSz, size_t outIdxSz,
uint8_t *outIdx, GXEncodeOptions_t *opts);

size_t GX_EncodeR5G6B5(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts);

size_t GX_EncodeRGB5A3(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts);

size_t GX_EncodeRGBA8(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts);

size_t GX_EncodeCMP(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts);

bool GX_EncodePaletteIA8(size_t palSz, uint32_t *pal, uint16_t *palOut, GXEncodeOptions_t *opts);

bool GX_EncodePaletteR5G6B5(size_t palSz, uint32_t *pal, uint16_t *palOut, GXEncodeOptions_t *opts);

bool GX_EncodePaletteRGB5A3(size_t palSz, uint32_t *pal, uint16_t *palOut, GXEncodeOptions_t *opts);

bool GX_BuildPalette(uint16_t w, uint16_t h, size_t inSz, uint32_t *in, size_t palSz, uint32_t *pal, size_t outIdxSz,
uint32_t *outIdx, size_t *outPalSz, GXEncodeOptions_t *opts);
#endif
#endif
