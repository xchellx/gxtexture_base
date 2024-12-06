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

#include <gxtexture.h>

#include <stddef.h>
#include <stdlib.h>
#include <limits.h>

#include <stdext/catexit.h>
#include <octree_color_quantizer.h>

#ifndef bswap_dxt18
#define bswap_dxt18(x) ((((x) & 0x3) << 6) | (((x) & 0xC) << 2) | (((x) & 0xC0) >> 6) | (((x) & 0x30) >> 2))
#endif

FORCE_INLINE uint8_t Dat_BSwapDXT18(uint8_t value) {
    return bswap_dxt18(value);
}

GX_EXPORT size_t GX_CalcMipSz(uint16_t w, uint16_t h, uint8_t bpp) {
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

GX_EXPORT size_t GX_GetMaxPalSz(uint8_t bpp) {
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
FORCE_INLINE void Dat_GetDXT1BE(uint8_t *s, uint8_t o[8]) {
    uint8_t *sPtr = s;
    uint8_t *oPtr = o;
    // uint16_t color1;
    Dat_SetU16LE(oPtr, Dat_GetU16BE(sPtr));
    sPtr += sizeof(uint16_t);
    oPtr += sizeof(uint16_t);
    // uint16_t color2;
    Dat_SetU16LE(oPtr, Dat_GetU16BE(sPtr));
    sPtr += sizeof(uint16_t);
    oPtr += sizeof(uint16_t);
    // uint8_t lines[4];
    oPtr[0] = Dat_BSwapDXT18(sPtr[0]);
    oPtr[1] = Dat_BSwapDXT18(sPtr[1]);
    oPtr[2] = Dat_BSwapDXT18(sPtr[2]);
    oPtr[3] = Dat_BSwapDXT18(sPtr[3]);
}

FORCE_INLINE uint32_t Dat_RGBA16ToBGRA(uint8_t outb[16 * 4], size_t px, size_t py) {
    uint8_t *bl = outb + (16 * py + 4 * px);
    uint32_t b = bl[2];
    uint32_t g = bl[1];
    uint32_t r = bl[0];
    uint32_t a = bl[3];
    return (
          (b << GX_COMP_SH_B) /* B */
        | (g << GX_COMP_SH_G) /* G */
        | (r << GX_COMP_SH_R) /* R */
        | (a << GX_COMP_SH_A) /* A */
    );
}

GX_EXPORT size_t GX_DecodeI4(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *inPtr = in;
    uint8_t *inEndPtr = in + inSz;
    uint32_t *outEndPtr = out + outSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_I4_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_I4_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_I4_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_I4_BW; bx += 2) {
                    size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                    size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                    for (uint8_t n = 0; catexit_loopSafety && n < 2; n++) {
                        uint32_t *outPtr = out + (fby * w + fbx + n);
                        if (outPtr < outEndPtr && fby < h && fbx < w) {
                            if (inPtr < inEndPtr)
                                *outPtr = GX_DecodeI4Nibble(*inPtr, n, opts);
                            else
                                *outPtr = 0 | (0xFF << GX_COMP_SH_A);
                        }
                    }
                    inPtr += sizeof(uint8_t);
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(in, inPtr) : 0;
}

GX_EXPORT size_t GX_DecodeI8(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *inPtr = in;
    uint8_t *inEndPtr = in + inSz;
    uint32_t *outEndPtr = out + outSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_I8_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_I8_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_I8_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_I8_BW; bx++) {
                    size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                    size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                    uint32_t *outPtr = out + (fby * w + fbx);
                    if (outPtr < outEndPtr && fby < h && fbx < w) {
                        if (inPtr < inEndPtr)
                            *outPtr = GX_DecodeI8Pixel(*inPtr, opts);
                        else
                            *outPtr = 0 | (0xFF << GX_COMP_SH_A);
                    }
                    inPtr += sizeof(uint8_t);
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(in, inPtr) : 0;
}

GX_EXPORT size_t GX_DecodeIA4(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *inPtr = in;
    uint8_t *inEndPtr = in + inSz;
    uint32_t *outEndPtr = out + outSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_IA4_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_IA4_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_IA4_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_IA4_BW; bx++) {
                    size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                    size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                    uint32_t *outPtr = out + (fby * w + fbx);
                    if (outPtr < outEndPtr && fby < h && fbx < w) {
                        if (inPtr < inEndPtr)
                            *outPtr = GX_DecodeIA4Pixel(*inPtr, opts);
                        else
                            *outPtr = 0;
                    }
                    inPtr += sizeof(uint8_t);
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(in, inPtr) : 0;
}

GX_EXPORT size_t GX_DecodeIA8(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *inPtr = in;
    uint8_t *inEndPtr = in + inSz;
    uint32_t *outEndPtr = out + outSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_IA8_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_IA8_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_IA8_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_IA8_BW; bx++) {
                    size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                    size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                    uint32_t *outPtr = out + (fby * w + fbx);
                    if (outPtr < outEndPtr && fby < h && fbx < w) {
                        if ((inPtr + 1) < inEndPtr)
                            *outPtr = GX_DecodeIA8Pixel(Dat_GetU16BE(inPtr), opts);
                        else
                            *outPtr = 0;
                    }
                    inPtr += sizeof(uint16_t);
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(in, inPtr) : 0;
}

GX_EXPORT size_t GX_DecodeCI4(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t palSz, uint32_t *pal, size_t outSz,
uint32_t *out, GXDecodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !palSz || !pal || !outSz || !out || !opts)
        return 0;
    
    uint8_t *inPtr = in;
    uint8_t *inEndPtr = in + inSz;
    uint32_t *outEndPtr = out + outSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_CI4_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_CI4_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_CI4_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_CI4_BW; bx += 2) {
                    size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                    size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                    for (uint8_t n = 0; catexit_loopSafety && n < 2; n++) {
                        uint32_t *outPtr = out + (fby * w + fbx + n);
                        if (outPtr < outEndPtr && fby < h && fbx < w) {
                            if (inPtr < inEndPtr) {
                                uint8_t outi = GX_DecodeCI4Nibble(*inPtr, n, opts);
                                if (outi < palSz)
                                    *outPtr = pal[outi];
                                else
                                    *outPtr = 0 | (0xFF << GX_COMP_SH_A);
                            } else
                                *outPtr = 0 | (0xFF << GX_COMP_SH_A);
                        }
                    }
                    inPtr += sizeof(uint8_t);
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(in, inPtr) : 0;
}

GX_EXPORT size_t GX_DecodeCI8(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t palSz, uint32_t *pal, size_t outSz,
uint32_t *out, GXDecodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !palSz || !pal || !outSz || !out || !opts)
        return 0;
    
    uint8_t *inPtr = in;
    uint8_t *inEndPtr = in + inSz;
    uint32_t *outEndPtr = out + outSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_CI8_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_CI8_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_CI8_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_CI8_BW; bx++) {
                    size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                    size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                    uint32_t *outPtr = out + (fby * w + fbx);
                    if (outPtr < outEndPtr && fby < h && fbx < w) {
                        if (inPtr < inEndPtr) {
                            uint8_t outi = GX_DecodeCI8Index(*inPtr, opts);
                            if (outi < palSz)
                                *outPtr = pal[outi];
                            else
                                *outPtr = 0 | (0xFF << GX_COMP_SH_A);
                        } else
                            *outPtr = 0 | (0xFF << GX_COMP_SH_A);
                    }
                    inPtr += sizeof(uint8_t);
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(in, inPtr) : 0;
}

GX_EXPORT size_t GX_DecodeCI14X2(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t palSz, uint32_t *pal, size_t outSz,
uint32_t *out, GXDecodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !palSz || !pal || !outSz || !out || !opts)
        return 0;
    
    uint8_t *inPtr = in;
    uint8_t *inEndPtr = in + inSz;
    uint32_t *outEndPtr = out + outSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_CI14X2_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_CI14X2_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_CI14X2_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_CI14X2_BW; bx++) {
                    size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                    size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                    uint32_t *outPtr = out + (fby * w + fbx);
                    if (outPtr < outEndPtr && fby < h && fbx < w) {
                        if ((inPtr + 1) < inEndPtr) {
                            uint32_t outi = GX_DecodeCI14X2Index(Dat_GetU16BE(inPtr), opts);
                            if (outi < palSz)
                                *outPtr = pal[outi];
                            else
                                *outPtr = 0 | (0xFF << GX_COMP_SH_A);
                        } else
                            *outPtr = 0 | (0xFF << GX_COMP_SH_A);
                    }
                    inPtr += sizeof(uint16_t);
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(in, inPtr) : 0;
}

GX_EXPORT size_t GX_DecodeR5G6B5(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *inPtr = in;
    uint8_t *inEndPtr = in + inSz;
    uint32_t *outEndPtr = out + outSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_R5G6B5_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_R5G6B5_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_R5G6B5_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_R5G6B5_BW; bx++) {
                    size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                    size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                    uint32_t *outPtr = out + (fby * w + fbx);
                    if (outPtr < outEndPtr && fby < h && fbx < w) {
                        if ((inPtr + 1) < inEndPtr)
                            *outPtr = GX_DecodeR5G6B5Pixel(Dat_GetU16BE(inPtr), opts);
                        else
                            *outPtr = (0xFF << GX_COMP_SH_A);
                    }
                    inPtr += sizeof(uint16_t);
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(in, inPtr) : 0;
}

GX_EXPORT size_t GX_DecodeRGB5A3(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *inPtr = in;
    uint8_t *inEndPtr = in + inSz;
    uint32_t *outEndPtr = out + outSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_RGB5A3_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_RGB5A3_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_RGB5A3_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_RGB5A3_BW; bx++) {
                    size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                    size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                    uint32_t *outPtr = out + (fby * w + fbx);
                    if (outPtr < outEndPtr && fby < h && fbx < w) {
                        if ((inPtr + 1) < inEndPtr)
                            *outPtr = GX_DecodeRGB5A3Pixel(Dat_GetU16BE(inPtr), opts);
                        else
                            *outPtr = 0;
                    }
                    inPtr += sizeof(uint16_t);
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(in, inPtr) : 0;
}

GX_EXPORT size_t GX_DecodeRGBA8(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *inPtr = in;
    uint8_t *inEndPtr = in + inSz;
    uint32_t *outEndPtr = out + outSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_RGBA8_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_RGBA8_BW) {
            for (uint8_t g = 0; catexit_loopSafety && g < 2; g++) {
                for (size_t by = y; catexit_loopSafety && by < y + GX_RGBA8_BH; by++) {
                    for (size_t bx = x; catexit_loopSafety && bx < x + GX_RGBA8_BW; bx++) {
                        size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                        size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                        uint32_t *outPtr = out + (fby * w + fbx);
                        if (outPtr < outEndPtr && fby < h && fbx < w) {
                            if ((inPtr + 1) < inEndPtr)
                                *outPtr = GX_DecodeRGBA8Group(Dat_GetU16BE(inPtr), g, *outPtr, opts);
                            else if (*outPtr != 0)
                                *outPtr = 0;
                        }
                        inPtr += sizeof(uint16_t);
                    }
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(in, inPtr) : 0;
}

GX_EXPORT size_t GX_DecodeCMP(uint16_t w, uint16_t h, size_t inSz, uint8_t *in, size_t outSz, uint32_t *out,
GXDecodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *inPtr = in;
    uint8_t *inEndPtr = in + inSz;
    uint32_t *outEndPtr = out + outSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_CMP_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_CMP_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_CMP_BH; by += (GX_CMP_BH / 2)) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_CMP_BW; bx += (GX_CMP_BW / 2)) {
                    if ((inPtr + 7) < inEndPtr) {
                        uint8_t inb[8];
                        Dat_GetDXT1BE(inPtr, inb);
                        uint8_t outb[16 * 4];
                        squish_Decompress((uint8_t *) outb, (uint8_t *) inb, kDxt1);
                        for (size_t py = 0; catexit_loopSafety && py < 4; py++) {
                            for (size_t px = 0; catexit_loopSafety && px < 4; px++) {
                                size_t fby = opts->flipY ? Math_FlipSz(by + py, h) : by + py;
                                size_t fbx = opts->flipX ? Math_FlipSz(bx + px, w) : bx + px;
                                uint32_t *outPtr = out + (fby * w + fbx);
                                if (outPtr < outEndPtr && fby < h && fbx < w)
                                    *outPtr = Dat_RGBA16ToBGRA(outb, px, py);
                            }
                        }
                    } else {
                        for (size_t py = 0; catexit_loopSafety && py < 4; py++) {
                            for (size_t px = 0; catexit_loopSafety && px < 4; px++) {
                                size_t fby = opts->flipY ? Math_FlipSz(by + py, h) : by + py;
                                size_t fbx = opts->flipX ? Math_FlipSz(bx + px, w) : bx + px;
                                uint32_t *outPtr = out + (fby * w + fbx);
                                if (outPtr < outEndPtr && fby < h && fbx < w)
                                    *outPtr = 0;
                            }
                        }
                    }
                    inPtr += 8;
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(in, inPtr) : 0;
}

GX_EXPORT bool GX_DecodePaletteIA8(size_t palSz, uint16_t *pal, uint32_t *palOut, GXDecodeOptions_t *opts) {
    if (!palSz || !pal || !palOut || !opts)
        return true;
    
    for (size_t i = 0; catexit_loopSafety && i < palSz; i++)
        palOut[i] = GX_DecodeIA8Pixel(Dat_BSwapU16(pal[i]), opts);
    
    return !catexit_loopSafety;
}

GX_EXPORT bool GX_DecodePaletteR5G6B5(size_t palSz, uint16_t *pal, uint32_t *palOut, GXDecodeOptions_t *opts) {
    if (!palSz || !pal || !palOut || !opts)
        return true;
    
    for (size_t i = 0; catexit_loopSafety && i < palSz; i++)
        palOut[i] = GX_DecodeR5G6B5Pixel(Dat_BSwapU16(pal[i]), opts);
    
    return !catexit_loopSafety;
}

GX_EXPORT bool GX_DecodePaletteRGB5A3(size_t palSz, uint16_t *pal, uint32_t *palOut, GXDecodeOptions_t *opts) {
    if (!palSz || !pal || !palOut || !opts)
        return true;
    
    for (size_t i = 0; catexit_loopSafety && i < palSz; i++)
        palOut[i] = GX_DecodeRGB5A3Pixel(Dat_BSwapU16(pal[i]), opts);
    
    return !catexit_loopSafety;
}
#endif

#ifdef GX_INCLUDE_ENCODE
FORCE_INLINE void Dat_SetDXT1BE(uint8_t *s, uint8_t v[8]) {
    uint8_t *sPtr = s;
    uint8_t *vPtr = v;
    // uint16_t color1;
    Dat_SetU16BE(sPtr, Dat_GetU16LE(vPtr));
    sPtr += sizeof(uint16_t);
    vPtr += sizeof(uint16_t);
    // uint16_t color2;
    Dat_SetU16BE(sPtr, Dat_GetU16LE(vPtr));
    sPtr += sizeof(uint16_t);
    vPtr += sizeof(uint16_t);
    // uint8_t lines[4];
    sPtr[0] = Dat_BSwapDXT18(vPtr[0]);
    sPtr[1] = Dat_BSwapDXT18(vPtr[1]);
    sPtr[2] = Dat_BSwapDXT18(vPtr[2]);
    sPtr[3] = Dat_BSwapDXT18(vPtr[3]);
}

FORCE_INLINE void Dat_BGRAToRGBA16(uint8_t inb[64], uint32_t inp, size_t px, size_t py) {
    uint8_t r = (inp >> GX_COMP_SH_R) & 0xFF;
    uint8_t g = (inp >> GX_COMP_SH_G) & 0xFF;
    uint8_t b = (inp >> GX_COMP_SH_B) & 0xFF;
    uint8_t a = (inp >> GX_COMP_SH_A) & 0xFF;
    uint8_t *bl = inb + (16 * py + 4 * px);
    bl[0] = r; /* R */
    bl[1] = g; /* G */
    bl[2] = b; /* B */
    bl[3] = a; /* A */
}

FORCE_INLINE uint32_t Clr_Subtract(uint32_t clr1, uint32_t clr2) {
    double clr1B = (double) ((clr1 >> GX_COMP_SH_B) & 0xFF);
    double clr1G = (double) ((clr1 >> GX_COMP_SH_G) & 0xFF);
    double clr1R = (double) ((clr1 >> GX_COMP_SH_R) & 0xFF);
    double clr1A = (double) ((clr1 >> GX_COMP_SH_A) & 0xFF);
    
    double clr2B = (double) ((clr2 >> GX_COMP_SH_B) & 0xFF);
    double clr2G = (double) ((clr2 >> GX_COMP_SH_G) & 0xFF);
    double clr2R = (double) ((clr2 >> GX_COMP_SH_R) & 0xFF);
    double clr2A = (double) ((clr2 >> GX_COMP_SH_A) & 0xFF);
    
    return (
          (((uint32_t) Math_ClampDb(round(clr1B - clr2B), 0, UCHAR_MAX)) << GX_COMP_SH_B) /* B */
        | (((uint32_t) Math_ClampDb(round(clr1G - clr2G), 0, UCHAR_MAX)) << GX_COMP_SH_G) /* G */
        | (((uint32_t) Math_ClampDb(round(clr1R - clr2R), 0, UCHAR_MAX)) << GX_COMP_SH_R) /* R */
        | (((uint32_t) Math_ClampDb(round(clr1A - clr2A), 0, UCHAR_MAX)) << GX_COMP_SH_A) /* A */
    );
}

FORCE_INLINE uint32_t Clr_Add(uint32_t clr1, uint32_t clr2) {
    double clr1B = (double) ((clr1 >> GX_COMP_SH_B) & 0xFF);
    double clr1G = (double) ((clr1 >> GX_COMP_SH_G) & 0xFF);
    double clr1R = (double) ((clr1 >> GX_COMP_SH_R) & 0xFF);
    double clr1A = (double) ((clr1 >> GX_COMP_SH_A) & 0xFF);
    
    double clr2B = (double) ((clr2 >> GX_COMP_SH_B) & 0xFF);
    double clr2G = (double) ((clr2 >> GX_COMP_SH_G) & 0xFF);
    double clr2R = (double) ((clr2 >> GX_COMP_SH_R) & 0xFF);
    double clr2A = (double) ((clr2 >> GX_COMP_SH_A) & 0xFF);
    
    return (
          (((uint32_t) Math_ClampDb(round(clr1B + clr2B), 0, UCHAR_MAX)) << GX_COMP_SH_B) /* B */
        | (((uint32_t) Math_ClampDb(round(clr1G + clr2G), 0, UCHAR_MAX)) << GX_COMP_SH_G) /* G */
        | (((uint32_t) Math_ClampDb(round(clr1R + clr2R), 0, UCHAR_MAX)) << GX_COMP_SH_R) /* R */
        | (((uint32_t) Math_ClampDb(round(clr1A + clr2A), 0, UCHAR_MAX)) << GX_COMP_SH_A) /* A */
    );
}

FORCE_INLINE uint32_t Clr_Multiply(uint32_t clr, double scalar) {
    double clrB = (double) ((clr >> GX_COMP_SH_B) & 0xFF);
    double clrG = (double) ((clr >> GX_COMP_SH_G) & 0xFF);
    double clrR = (double) ((clr >> GX_COMP_SH_R) & 0xFF);
    double clrA = (double) ((clr >> GX_COMP_SH_A) & 0xFF);
    
    return (
          (((uint32_t) Math_ClampDb(round(clrB * scalar), 0, UCHAR_MAX)) << GX_COMP_SH_B) /* B */
        | (((uint32_t) Math_ClampDb(round(clrG * scalar), 0, UCHAR_MAX)) << GX_COMP_SH_G) /* G */
        | (((uint32_t) Math_ClampDb(round(clrR * scalar), 0, UCHAR_MAX)) << GX_COMP_SH_R) /* R */
        | (((uint32_t) Math_ClampDb(round(clrA * scalar), 0, UCHAR_MAX)) << GX_COMP_SH_A) /* A */
    );
}

GX_EXPORT size_t GX_EncodeI4(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *outPtr = out;
    uint8_t *outEndPtr = out + outSz;
    uint32_t *inEndPtr = in + inSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_I4_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_I4_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_I4_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_I4_BW; bx += 2) {
                    if (outPtr < outEndPtr) {
                        size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                        size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                        *outPtr = 0;
                        for (uint8_t n = 0; catexit_loopSafety && n < 2; n++) {
                            uint32_t *inPtr = in + (fby * w + fbx + n);
                            if (inPtr < inEndPtr && fby < h && fbx < w)
                                *outPtr |= GX_EncodeI4Nibble(*inPtr, n, *outPtr, opts);
                        }
                        outPtr += sizeof(uint8_t);
                    }
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(out, outPtr) : 0;
}

GX_EXPORT size_t GX_EncodeI8(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *outPtr = out;
    uint8_t *outEndPtr = out + outSz;
    uint32_t *inEndPtr = in + inSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_I8_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_I8_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_I8_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_I8_BW; bx++) {
                    if (outPtr < outEndPtr) {
                        size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                        size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                        uint32_t *inPtr = in + (fby * w + fbx);
                        if (inPtr < inEndPtr && by < h && bx < w)
                            *outPtr = GX_EncodeI8Pixel(*inPtr, opts);
                        else
                            *outPtr = 0;
                        outPtr += sizeof(uint8_t);
                    }
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(out, outPtr) : 0;
}

GX_EXPORT size_t GX_EncodeIA4(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *outPtr = out;
    uint8_t *outEndPtr = out + outSz;
    uint32_t *inEndPtr = in + inSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_IA4_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_IA4_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_IA4_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_IA4_BW; bx++) {
                    if (outPtr < outEndPtr) {
                        size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                        size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                        uint32_t *inPtr = in + (fby * w + fbx);
                        if (inPtr < inEndPtr && by < h && bx < w)
                            *outPtr = GX_EncodeIA4Pixel(*inPtr, opts);
                        else
                            *outPtr = 0;
                        outPtr += sizeof(uint8_t);
                    }
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(out, outPtr) : 0;
}

GX_EXPORT size_t GX_EncodeIA8(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *outPtr = out;
    uint8_t *outEndPtr = out + outSz;
    uint32_t *inEndPtr = in + inSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_IA8_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_IA8_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_IA8_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_IA8_BW; bx++) {
                    if ((outPtr + 1) < outEndPtr) {
                        size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                        size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                        uint32_t *inPtr = in + (fby * w + fbx);
                        if (inPtr < inEndPtr && by < h && bx < w)
                            Dat_SetU16BE(outPtr, GX_EncodeIA8Pixel(*inPtr, opts));
                        else
                            Dat_SetU16BE(outPtr, 0);
                        outPtr += sizeof(uint16_t);
                    }
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(out, outPtr) : 0;
}

GX_EXPORT size_t GX_EncodeCI4(uint16_t w, uint16_t h, size_t inIdxSz, uint32_t *inIdx, size_t palSz, size_t outIdxSz,
uint8_t *outIdx, GXEncodeOptions_t *opts) {
    if (!w | !h || !inIdxSz || !inIdx || !palSz || !outIdxSz || !outIdx || !opts)
        return 0;
    
    uint8_t *outIdxPtr = outIdx;
    uint8_t *outIdxEndPtr = outIdx + outIdxSz;
    uint32_t *inIdxEndPtr = inIdx + inIdxSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_CI4_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_CI4_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_CI4_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_CI4_BW; bx += 2) {
                    if (outIdxPtr < outIdxEndPtr) {
                        size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                        size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                        *outIdxPtr = 0;
                        for (uint8_t n = 0; catexit_loopSafety && n < 2; n++) {
                            uint32_t *inIdxPtr = inIdx + (fby * w + fbx + n);
                            if (inIdxPtr < inIdxEndPtr && fby < h && fbx < w) {
                                uint32_t ini = *inIdxPtr;
                                if (ini < palSz)
                                    *outIdxPtr |= GX_EncodeCI4Nibble(ini, n, *outIdxPtr, opts);
                            }
                        }
                        outIdxPtr += sizeof(uint8_t);
                    }
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(outIdx, outIdxPtr) : 0;
}

GX_EXPORT size_t GX_EncodeCI8(uint16_t w, uint16_t h, size_t inIdxSz, uint32_t *inIdx, size_t palSz, size_t outIdxSz,
uint8_t *outIdx, GXEncodeOptions_t *opts) {
    if (!w | !h || !inIdxSz || !inIdx || !palSz || !outIdxSz || !outIdx || !opts)
        return 0;
    
    uint8_t *outIdxPtr = outIdx;
    uint8_t *outIdxEndPtr = outIdx + outIdxSz;
    uint32_t *inIdxEndPtr = inIdx + inIdxSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_CI8_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_CI8_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_CI8_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_CI8_BW; bx++) {
                    if (outIdxPtr < outIdxEndPtr) {
                        size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                        size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                        uint32_t *inIdxPtr = inIdx + (fby * w + fbx);
                        if (inIdxPtr < inIdxEndPtr && fby < h && fbx < w) {
                            uint32_t ini = *inIdxPtr;
                            if (ini < palSz)
                                *outIdxPtr = GX_EncodeCI8Index(ini, opts);
                            else
                                *outIdxPtr = 0;
                        } else
                            *outIdxPtr = 0;
                        outIdxPtr += sizeof(uint8_t);
                    }
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(outIdx, outIdxPtr) : 0;
}

GX_EXPORT size_t GX_EncodeCI14X2(uint16_t w, uint16_t h, size_t inIdxSz, uint32_t *inIdx, size_t palSz, size_t outIdxSz,
uint8_t *outIdx, GXEncodeOptions_t *opts) {
    if (!w | !h || !inIdxSz || !inIdx || !palSz || !outIdxSz || !outIdx || !opts)
        return 0;
    
    uint8_t *outIdxPtr = outIdx;
    uint8_t *outIdxEndPtr = outIdx + outIdxSz;
    uint32_t *inIdxEndPtr = inIdx + inIdxSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_CI14X2_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_CI14X2_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_CI14X2_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_CI14X2_BW; bx++) {
                    if ((outIdxPtr + 1) < outIdxEndPtr) {
                        size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                        size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                        uint32_t *inIdxPtr = inIdx + (fby * w + fbx);
                        if (inIdxPtr < inIdxEndPtr && fby < h && fbx < w) {
                            uint32_t ini = *inIdxPtr;
                            if (ini < palSz)
                                Dat_SetU16BE(outIdxPtr, GX_EncodeCI14X2Index(ini, opts));
                            else
                                Dat_SetU16BE(outIdxPtr, 0);
                        } else
                            Dat_SetU16BE(outIdxPtr, 0);
                        outIdxPtr += sizeof(uint16_t);
                    }
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(outIdx, outIdxPtr) : 0;
}

GX_EXPORT size_t GX_EncodeR5G6B5(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *outPtr = out;
    uint8_t *outEndPtr = out + outSz;
    uint32_t *inEndPtr = in + inSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_R5G6B5_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_R5G6B5_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_R5G6B5_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_R5G6B5_BW; bx++) {
                    if ((outPtr + 1) < outEndPtr) {
                        size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                        size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                        uint32_t *inPtr = in + (fby * w + fbx);
                        if (inPtr < inEndPtr && by < h && bx < w)
                            Dat_SetU16BE(outPtr, GX_EncodeR5G6B5Pixel(*inPtr, opts));
                        else
                            Dat_SetU16BE(outPtr, 0);
                        outPtr += sizeof(uint16_t);
                    }
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(out, outPtr) : 0;
}

GX_EXPORT size_t GX_EncodeRGB5A3(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *outPtr = out;
    uint8_t *outEndPtr = out + outSz;
    uint32_t *inEndPtr = in + inSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_RGB5A3_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_RGB5A3_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_RGB5A3_BH; by++) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_RGB5A3_BW; bx++) {
                    if ((outPtr + 1) < outEndPtr) {
                        size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                        size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                        uint32_t *inPtr = in + (fby * w + fbx);
                        if (inPtr < inEndPtr && by < h && bx < w)
                            Dat_SetU16BE(outPtr, GX_EncodeRGB5A3Pixel(*inPtr, opts));
                        else
                            Dat_SetU16BE(outPtr, 0);
                        outPtr += sizeof(uint16_t);
                    }
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(out, outPtr) : 0;
}

GX_EXPORT size_t GX_EncodeRGBA8(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts)
        return 0;
    
    uint8_t *outPtr = out;
    uint8_t *outEndPtr = out + outSz;
    uint32_t *inEndPtr = in + inSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_RGBA8_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_RGBA8_BW) {
            for (uint8_t g = 0; catexit_loopSafety && g < 2; g++) {
                for (size_t by = y; catexit_loopSafety && by < y + GX_RGBA8_BH; by++) {
                    for (size_t bx = x; catexit_loopSafety && bx < x + GX_RGBA8_BW; bx++) {
                        if ((outPtr + 1) < outEndPtr) {
                            size_t fby = opts->flipY ? Math_FlipSz(by, h) : by;
                            size_t fbx = opts->flipX ? Math_FlipSz(bx, w) : bx;
                            uint32_t *inPtr = in + (fby * w + fbx);
                            if (inPtr < inEndPtr && by < h && bx < w)
                                Dat_SetU16BE(outPtr, GX_EncodeRGBA8Group(*inPtr, g, opts));
                            else if (Dat_GetU16BE(outPtr) != 0)
                                Dat_SetU16BE(outPtr, 0);
                            outPtr += sizeof(uint16_t);
                        }
                    }
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(out, outPtr) : 0;
}

GX_EXPORT size_t GX_EncodeCMP(uint16_t w, uint16_t h, size_t inSz, uint32_t* in, size_t outSz, uint8_t *out,
GXEncodeOptions_t *opts) {
    if (!w | !h || !inSz || !in || !outSz || !out || !opts || (opts->squishMetric && opts->squishMetricSz != 3))
        return 0;
    
    uint8_t *outPtr = out;
    uint8_t *outEndPtr = out + outSz;
    uint32_t *inEndPtr = in + inSz;
    for (size_t y = 0; catexit_loopSafety && y < h; y += GX_CMP_BH) {
        for (size_t x = 0; catexit_loopSafety && x < w; x += GX_CMP_BW) {
            for (size_t by = y; catexit_loopSafety && by < y + GX_CMP_BH; by += (GX_CMP_BH / 2)) {
                for (size_t bx = x; catexit_loopSafety && bx < x + GX_CMP_BW; bx += (GX_CMP_BW / 2)) {
                    if ((outPtr + 7) < outEndPtr) {
                        uint8_t inb[64];
                        for (size_t py = 0; catexit_loopSafety && py < 4; py++) {
                            for (size_t px = 0; catexit_loopSafety && px < 4; px++) {
                                size_t fby = opts->flipY ? Math_FlipSz(by + py, h) : by + py;
                                size_t fbx = opts->flipX ? Math_FlipSz(bx + px, w) : bx + px;
                                uint32_t *inPtr = in + (fby * w + fbx);
                                if (inPtr < inEndPtr && by < h && bx < w)
                                    Dat_BGRAToRGBA16(inb, *inPtr, px, py);
                                else
                                    Dat_BGRAToRGBA16(inb, 0, px, py);
                            }
                        }
                        
                        uint8_t outb[8];
                        squish_Compress((uint8_t *) inb, (uint8_t *) outb,
                            kDxt1 | (opts->squishFlags & ~(kDxt1 | kDxt3 | kDxt5 | kBc4 | kBc5 | kSourceBGRA)),
                            (float *) opts->squishMetric);
                        Dat_SetDXT1BE(outPtr, outb);
                        // sizeof(DXT1Block) == 8
                        outPtr += 8;
                    }
                }
            }
        }
    }
    return catexit_loopSafety ? Dat_PtrDiff(out, outPtr) : 0;
}

GX_EXPORT bool GX_EncodePaletteIA8(size_t palSz, uint32_t *pal, uint16_t *palOut, GXEncodeOptions_t *opts) {
    if (!palSz || !pal || !palOut || !opts)
        return true;
    
    for (size_t i = 0; catexit_loopSafety && i < palSz; i++)
        palOut[i] = Dat_BSwapU16(GX_EncodeIA8Pixel(pal[i], opts));
    
    return !catexit_loopSafety;
}

GX_EXPORT bool GX_EncodePaletteR5G6B5(size_t palSz, uint32_t *pal, uint16_t *palOut, GXEncodeOptions_t *opts) {
    if (!palSz || !pal || !palOut || !opts)
        return true;
    
    for (size_t i = 0; catexit_loopSafety && i < palSz; i++)
        palOut[i] = Dat_BSwapU16(GX_EncodeR5G6B5Pixel(pal[i], opts));
    
    return !catexit_loopSafety;
}

GX_EXPORT bool GX_EncodePaletteRGB5A3(size_t palSz, uint32_t *pal, uint16_t *palOut, GXEncodeOptions_t *opts) {
    if (!palSz || !pal || !palOut || !opts)
        return true;
    
    for (size_t i = 0; catexit_loopSafety && i < palSz; i++)
        palOut[i] = Dat_BSwapU16(GX_EncodeRGB5A3Pixel(pal[i], opts));
    
    return !catexit_loopSafety;
}

static size_t kernl[9] = {
    // GX_DT_THRESHOLD
    0,
    // GX_DT_FLOYD_STEINBERG
    4,
    // GX_DT_ATKINSON
    6,
    // GX_DT_JARVIS_JUDICE_NINKE
    12,
    // GX_DT_STUCKI
    12,
    // GX_DT_BURKES
    7,
    // GX_DT_TWO_ROW_SIERRA
    10,
    // GX_DT_SIERRA
    7,
    // GX_DT_SIERRA_LITE
    3
};

static int32_t *kerni[9] = {
    // GX_DT_THRESHOLD
    NULL,
    // GX_DT_FLOYD_STEINBERG
    (int32_t[4 * 2]) {
     /* y   x */
        0,  1,
        1, -1,
        1,  0,
        1,  1
    },
    // GX_DT_ATKINSON
    (int32_t[6 * 2]) {
     /* y   x */
        0,  1,
        0,  2,
        1, -1,
        1,  0,
        1,  1,
        2,  0
    },
    // GX_DT_JARVIS_JUDICE_NINKE
    (int32_t[12 * 2]) {
     /* y   x */
        0,  1,
        0,  2,
        1, -2,
        1, -1,
        1,  0,
        1,  1,
        1,  2,
        2, -2,
        2, -1,
        2,  0,
        2,  1,
        2,  2
    },
    // GX_DT_STUCKI
    (int32_t[12 * 2]) {
     /* y   x */
        0,  1,
        0,  2,
        1, -2,
        1, -1,
        1,  0,
        1,  1,
        1,  2,
        2, -2,
        2, -1,
        2,  0,
        2,  1,
        2,  2
    },
    // GX_DT_BURKES
    (int32_t[7 * 2]) {
     /* y   x */
        0,  1,
        0,  2,
        1, -2,
        1, -1,
        1,  0,
        1,  1,
        1,  2
    },
    // GX_DT_TWO_ROW_SIERRA
    (int32_t[10 * 2]) {
     /* y   x */
        0,  1,
        0,  2,
        1, -2,
        1, -1,
        1,  0,
        1,  1,
        1,  2,
        2, -1,
        2,  0,
        2,  1
    },
    // GX_DT_SIERRA
    (int32_t[7 * 2]) {
     /* y   x */
        0,  1,
        0,  2,
        1, -2,
        1, -1,
        1,  0,
        1,  1,
        1,  2
    },
    // GX_DT_SIERRA_LITE
    (int32_t[3 * 2]) {
     /* y   x */
        0,  1,
        1, -1,
        1,  0,
    }
};

static double *kernd[9] = {
    // GX_DT_THRESHOLD
    NULL,
    // GX_DT_FLOYD_STEINBERG
    (double[4]) {
        0.4375 /* 7.0 / 16.0 */,
        0.1875 /* 3.0 / 16.0 */,
        0.3125 /* 5.0 / 16.0 */,
        0.0625 /* 1.0 / 16.0 */
    },
    // GX_DT_ATKINSON
    (double[6]) {
        0.125 /* 1.0 / 8.0 */,
        0.125 /* 1.0 / 8.0 */,
        0.125 /* 1.0 / 8.0 */,
        0.125 /* 1.0 / 8.0 */,
        0.125 /* 1.0 / 8.0 */,
        0.125 /* 1.0 / 8.0 */
    },
    // GX_DT_JARVIS_JUDICE_NINKE
    (double[12]) {
        0.145833 /* 7.0 / 48.0 */,
        0.104167 /* 5.0 / 48.0 */,
        0.0625   /* 3.0 / 48.0 */,
        0.104167 /* 5.0 / 48.0 */,
        0.145833 /* 7.0 / 48.0 */,
        0.104167 /* 5.0 / 48.0 */,
        0.0625   /* 3.0 / 48.0 */,
        0.020833 /* 1.0 / 48.0 */,
        0.0625   /* 3.0 / 48.0 */,
        0.104167 /* 5.0 / 48.0 */,
        0.0625   /* 3.0 / 48.0 */,
        0.020833 /* 1.0 / 48.0 */
    },
    // GX_DT_STUCKI
    (double[12]) {
        0.190476 /* 8.0 / 42.0 */,
        0.095238 /* 4.0 / 42.0 */,
        0.047619 /* 2.0 / 42.0 */,
        0.095238 /* 4.0 / 42.0 */,
        0.190476 /* 8.0 / 42.0 */,
        0.095238 /* 4.0 / 42.0 */,
        0.047619 /* 2.0 / 42.0 */,
        0.02381  /* 1.0 / 42.0 */,
        0.047619 /* 2.0 / 42.0 */,
        0.095238 /* 4.0 / 42.0 */,
        0.047619 /* 2.0 / 42.0 */,
        0.02381  /* 1.0 / 42.0 */
    },
    // GX_DT_BURKES
    (double[7]) {
        0.25   /* 8.0 / 32.0 */,
        0.125  /* 4.0 / 32.0 */,
        0.0625 /* 2.0 / 32.0 */,
        0.125  /* 4.0 / 32.0 */,
        0.25   /* 8.0 / 32.0 */,
        0.125  /* 4.0 / 32.0 */,
        0.0625 /* 2.0 / 32.0 */
    },
    // GX_DT_TWO_ROW_SIERRA
    (double[10]) {
        0.15625 /* 5.0 / 32.0 */,
        0.09375 /* 3.0 / 32.0 */,
        0.0625  /* 2.0 / 32.0 */,
        0.125   /* 4.0 / 32.0 */,
        0.15625 /* 5.0 / 32.0 */,
        0.125   /* 4.0 / 32.0 */,
        0.0625  /* 2.0 / 32.0 */,
        0.0625  /* 2.0 / 32.0 */,
        0.09375 /* 3.0 / 32.0 */,
        0.0625  /* 2.0 / 32.0 */
    },
    // GX_DT_SIERRA
    (double[7]) {
        0.25   /* 4.0 / 16.0 */,
        0.1875 /* 3.0 / 16.0 */,
        0.0625 /* 1.0 / 16.0 */,
        0.125  /* 2.0 / 16.0 */,
        0.1875 /* 3.0 / 16.0 */,
        0.125  /* 2.0 / 16.0 */,
        0.0625 /* 1.0 / 16.0 */
    },
    // GX_DT_SIERRA_LITE
    (double[3]) {
        0.5  /* 2.0 / 4.0 */,
        0.25 /* 1.0 / 4.0 */,
        0.25 /* 1.0 / 4.0 */
    }
};

GX_EXPORT bool GX_BuildPalette(uint16_t w, uint16_t h, size_t inSz, uint32_t *in, size_t palSz, uint32_t *pal, size_t outIdxSz,
uint32_t *outIdx, size_t *outPalSz, GXEncodeOptions_t *opts) {
    if (inSz != w * h || !in || (palSz != GX_GetMaxPalSz(GX_CI4_BPP) && palSz != GX_GetMaxPalSz(GX_CI8_BPP)
    && palSz != GX_GetMaxPalSz(GX_CI14X2_BPP)) || !pal || outIdxSz != inSz || !outIdx || !outPalSz || !opts
    || opts->ditherType < GX_DT_MIN || opts->ditherType > GX_DT_MAX)
        return true;
    
    *outPalSz = 0;
    
    size_t inScrSz = inSz * sizeof(uint32_t);
    uint32_t *inScr = malloc(inScrSz);
    if (!inScr)
        return true;
    
    // Quantize by octree
    OCQOctreeQuantizer_t *octree = OCQOctreeQuantizer___init__();
    for (size_t y = 0; catexit_loopSafety && y < h; y++) {
        for (size_t x = 0; catexit_loopSafety && x < w; x++) {
            // TODO: Seperate flipping flag? Or somehow check if the original image is flipped? Or should TGA ALWAYS decode data to be upright?
            size_t fy = y; //opts->flipY ? Math_FlipSz(y, h) : y;
            size_t fx = x; //opts->flipX ? Math_FlipSz(x, w) : x;
            
            uint32_t inClr = *(in + (fy * w + fx));
            *(inScr + (fy * w + fx)) = inClr;
            OCQOctreeQuantizer_add_color_raw(octree, inClr);
        }
    }
    
    size_t paletteSz = 0;
    OCQOctreeQuantizer_make_palette_raw(octree, palSz, pal, &paletteSz);
    if (!paletteSz) {
        OCQOctreeQuantizer_free(octree);
        free(inScr);
        return true;
    }
    *outPalSz = paletteSz;
    
    for (size_t y = 0; catexit_loopSafety && y < h; y++) {
        for (size_t x = 0; catexit_loopSafety && x < w; x++) {
            // TODO: Seperate flipping flag? Or somehow check if the original image is flipped? Or should TGA ALWAYS decode data to be upright?
            size_t fy = y; //opts->flipY ? Math_FlipSz(y, h) : y;
            size_t fx = x; //opts->flipX ? Math_FlipSz(x, w) : x;
            
            uint32_t *inScrPtr = inScr + (fy * w + fx);
            uint32_t inScrClr = *inScrPtr;
            
            // Dither by error diffusion
            size_t palIdx = OCQOctreeQuantizer_get_palette_index_raw(octree, inScrClr);
            *(outIdx + (fy * w + fx)) = palIdx;
            uint32_t palClr = pal[palIdx];
            uint32_t clrErr = Clr_Subtract(inScrClr, palClr);
            *inScrPtr = palClr;
            if (opts->ditherType != GX_DT_THRESHOLD) {
                int32_t dithi = (int32_t) opts->ditherType;
                for (size_t i = 0; i < kernl[dithi]; i++) {
                    int32_t yAdd = kerni[dithi][i * 2];
                    int32_t xAdd = kerni[dithi][(i * 2) + 1];
                    // TODO: Seperate flipping flag? Or somehow check if the original image is flipped? Or should TGA ALWAYS decode data to be upright?
                    //if (opts->flipY)
                    //    yAdd = -(yAdd);
                    //if (opts->flipX)
                    //    xAdd = -(xAdd);
                    
                    size_t iy = fy + ((fy < 2 && yAdd < 0) ? 0 : yAdd);
                    size_t ix = fx + ((fx < 2 && xAdd < 0) ? 0 : xAdd);
                    if (iy < h && ix < w) {
                        inScrPtr = inScr + (iy * w + ix);
                        *inScrPtr = Clr_Add(*inScrPtr, Clr_Multiply(clrErr, kernd[dithi][i]));
                    }
                }
            }
        }
    }
    
    OCQOctreeQuantizer_free(octree);
    free(inScr);
    
    return !catexit_loopSafety;
}
#endif
