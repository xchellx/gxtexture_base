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

#include <txtr.h>

#include <math.h>
#include <float.h>

#include <stdext/cmath.h>
#include <stdext/catexit.h>
#include <stb_image_resize2.h>

TXTR_EXPORT void TXTR_free(TXTR_t *txtr) {
    free(txtr->pal);
    txtr->pal = NULL;
    free(txtr->mips);
    txtr->mips = NULL;
}

TXTR_EXPORT bool TXTR_IsIndexed(TXTRFormat_t texFmt) {
    return texFmt == TXTR_TTF_CI4 || texFmt == TXTR_TTF_CI8 || texFmt == TXTR_TTF_CI14X2;
}

TXTR_EXPORT size_t TXTR_CalcMipSz(TXTRFormat_t texFmt, uint16_t width, uint16_t height) {
    switch (texFmt) {
        case TXTR_TTF_I4:
            return GX_CalcMipSz(width, height, GX_I4_BPP);
        case TXTR_TTF_I8:
            return GX_CalcMipSz(width, height, GX_I8_BPP);
        case TXTR_TTF_IA4:
            return GX_CalcMipSz(width, height, GX_IA4_BPP);
        case TXTR_TTF_IA8:
            return GX_CalcMipSz(width, height, GX_IA8_BPP);
        case TXTR_TTF_CI4:
            return GX_CalcMipSz(width, height, GX_CI4_BPP);
        case TXTR_TTF_CI8:
            return GX_CalcMipSz(width, height, GX_CI8_BPP);
        case TXTR_TTF_CI14X2:
            return GX_CalcMipSz(width, height, GX_CI14X2_BPP);
        case TXTR_TTF_R5G6B5:
            return GX_CalcMipSz(width, height, GX_R5G6B5_BPP);
        case TXTR_TTF_RGB5A3:
            return GX_CalcMipSz(width, height, GX_RGB5A3_BPP);
        case TXTR_TTF_RGBA8:
            return GX_CalcMipSz(width, height, GX_RGBA8_BPP);
        case TXTR_TTF_CMP:
            return GX_CalcMipSz(width, height, GX_CMP_BPP);
        default:
            return 0;
    }
}

TXTR_EXPORT size_t TXTR_GetMaxPalSz(TXTRFormat_t texFmt) {
    switch (texFmt) {
        case TXTR_TTF_CI4:
            return GX_GetMaxPalSz(GX_CI4_BPP);
        case TXTR_TTF_CI8:
            return GX_GetMaxPalSz(GX_CI8_BPP);
        case TXTR_TTF_CI14X2:
            return GX_GetMaxPalSz(GX_CI14X2_BPP);
        default:
            return 0;
    }
}

#ifdef TXTR_INCLUDE_DECODE
TXTR_EXPORT void TXTRMipmap_free(TXTRMipmap_t *mip) {
    free(mip->data);
    mip->data = NULL;
}

TXTR_EXPORT TXTRReadError_t TXTR_Read(TXTR_t *txtr, size_t dataSz, uint8_t *data) {
    if (txtr) {
        txtr->pal = NULL;
        txtr->mips = NULL;
    }
    
    if (!txtr || !dataSz || !data)
        return TXTR_RE_INVLDPARAMS;
    
    uint8_t *dEndPtr = data + dataSz;
    uint8_t *dPtr = data;
    
    txtr->hdr.format = (TXTRFormat_t) Dat_GetU32BE(dPtr);
    if (txtr->hdr.format < TXTR_TTF_I4 || txtr->hdr.format > TXTR_TTF_CMP)
        return TXTR_RE_INVLDTEXFMT;
    dPtr += sizeof(uint32_t);
    
    txtr->hdr.width = Dat_GetU16BE(dPtr);
    if (!txtr->hdr.width)
        return TXTR_RE_INVLDTEXHEIGHT;
    dPtr += sizeof(uint16_t);
    
    txtr->hdr.height = Dat_GetU16BE(dPtr);
    if (!txtr->hdr.height)
        return TXTR_RE_INVLDTEXWIDTH;
    dPtr += sizeof(uint16_t);
    
    txtr->hdr.mipCount = Dat_GetU32BE(dPtr);
    if (!txtr->hdr.mipCount || txtr->hdr.mipCount > 11)
        return TXTR_RE_INVLDMIPCNT;
    dPtr += sizeof(uint32_t);
    
    txtr->isIndexed = TXTR_IsIndexed(txtr->hdr.format);
    if (txtr->isIndexed) {
        txtr->palHdr.format = (TXTRPaletteFormat_t) Dat_GetU32BE(dPtr);
        if (txtr->palHdr.format < TXTR_TPF_IA8 || txtr->palHdr.format > TXTR_TPF_RGB5A3)
            return TXTR_RE_INVLDPALFMT;
        dPtr += sizeof(uint32_t);
        
        txtr->palHdr.width = Dat_GetU16BE(dPtr);
        if (!txtr->palHdr.width)
            return TXTR_RE_INVLDPALWIDTH;
        dPtr += sizeof(uint16_t);
        
        txtr->palHdr.height = Dat_GetU16BE(dPtr);
        if (!txtr->palHdr.height)
            return TXTR_RE_INVLDPALHEIGHT;
        dPtr += sizeof(uint16_t);
        
        txtr->palSz = txtr->palHdr.width * txtr->palHdr.height;
        if (txtr->palSz > TXTR_GetMaxPalSz(txtr->hdr.format))
            return TXTR_RE_INVLDPALSZ;
        
        size_t aPalSz = txtr->palSz * sizeof(uint16_t);
        txtr->pal = malloc(aPalSz);
        if (!txtr->pal) {
            txtr->palSz = 0;
            return TXTR_RE_MEMFAILPAL;
        }
        memcpy(txtr->pal, dPtr, aPalSz);
        dPtr += aPalSz;
    } else {
        txtr->palSz = 0;
        txtr->pal = NULL;
    }
    
    // Although GX_CalcMipSz can be used here, it may fail in certain cases such as I8 textures, where Retro had a bug
    // in their TXTR cooker.
    txtr->mipsSz = (size_t) (((uintptr_t) dEndPtr) - ((uintptr_t) dPtr));
    txtr->mips = malloc(txtr->mipsSz);
    if (!txtr->mips) {
        if (txtr->isIndexed) {
            txtr->palSz = 0;
            free(txtr->pal);
            txtr->pal = NULL;
        }
        txtr->mipsSz = 0;
        return TXTR_RE_MEMFAILMIPS;
    }
    memcpy(txtr->mips, dPtr, txtr->mipsSz);
    
    return TXTR_RE_SUCCESS;
}

TXTR_EXPORT TXTRDecodeError_t TXTR_Decode(TXTR_t *txtr, TXTRMipmap_t mipsOut[11], size_t *mipsOutCount,
TXTRDecodeOptions_t *opts) {
    if (!txtr || !mipsOut || !mipsOutCount || !opts || (txtr->isIndexed && !TXTR_IsIndexed(txtr->hdr.format)))
        return TXTR_DE_INVLDPARAMS;
    
    if (txtr->isIndexed && (!txtr->pal || !txtr->palSz))
        return TXTR_DE_INVLDTEXPAL;
    
    if (!txtr->mips || !txtr->mipsSz)
        return TXTR_DE_INVLDTEXMIPS;
    
    if (!txtr->hdr.width)
        return TXTR_DE_INVLDTEXWIDTH;
    
    if (!txtr->hdr.height)
        return TXTR_DE_INVLDTEXHEIGHT;
    
    if (!txtr->hdr.mipCount || txtr->hdr.mipCount > 11)
        return TXTR_DE_INVLDTEXMIPCNT;
    
    if (txtr->hdr.format < TXTR_TTF_I4 || txtr->hdr.format > TXTR_TTF_CMP)
        return TXTR_DE_INVLDTEXFMT;
    
    if (txtr->isIndexed && (txtr->palHdr.format < TXTR_TPF_IA8 || txtr->palHdr.format > TXTR_TPF_RGB5A3))
        return TXTR_DE_INVLDPALFMT;
    
    GXDecodeOptions_t gxOpts = {
        .flipX = opts->flipX,
        .flipY = opts->flipY
    };
    
    uint32_t *palette = NULL;
    if (txtr->isIndexed) {
        palette = malloc(txtr->palSz * sizeof(uint32_t));
        if (!palette)
            return TXTR_DE_MEMFAILPAL;
        
        bool palFail = false;
        switch (txtr->palHdr.format) {
            case TXTR_TPF_IA8:
                palFail = GX_DecodePaletteIA8(txtr->palSz, txtr->pal, palette, &gxOpts);
                break;
            case TXTR_TPF_R5G6B5:
                palFail = GX_DecodePaletteR5G6B5(txtr->palSz, txtr->pal, palette, &gxOpts);
                break;
            case TXTR_TPF_RGB5A3:
                palFail = GX_DecodePaletteRGB5A3(txtr->palSz, txtr->pal, palette, &gxOpts);
                break;
            default:
                free(palette);
                return TXTR_DE_INVLDPALFMT;
        }
        
        if (palFail) {
            free(palette);
            return TXTR_DE_FAILDECPAL;
        }
    }
    
    uint8_t *mipsPtr = txtr->mips;
    size_t mipsSzRem = txtr->mipsSz;
    uint16_t mipWidth = txtr->hdr.width;
    uint16_t mipHeight = txtr->hdr.height;
    size_t m = 0;
    for (size_t l = opts->decAllMips ? txtr->hdr.mipCount : 1; catexit_loopSafety && m < l; m++) {
        size_t curOutSz = mipWidth * mipHeight;
        size_t dataSz = curOutSz * sizeof(uint32_t);
        mipsOut[m].width = mipWidth;
        mipsOut[m].height = mipHeight;
        mipsOut[m].size = curOutSz;
        mipsOut[m].data = malloc(dataSz);
        if (!mipsOut[m].data) {
            free(palette);
            for (size_t m2 = 0, l2 = m + 1; m2 < l2; m2++)
                TXTRMipmap_free(&mipsOut[m2]);
            return TXTR_DE_MEMFAILMIP;
        }
        
        size_t pixsrd = 0;
        switch (txtr->hdr.format) {
            case TXTR_TTF_I4:
                pixsrd = GX_DecodeI4(mipWidth, mipHeight, mipsSzRem, mipsPtr, curOutSz, mipsOut[m].data, &gxOpts);
                break;
            case TXTR_TTF_I8:
                pixsrd = GX_DecodeI8(mipWidth, mipHeight, mipsSzRem, mipsPtr, curOutSz, mipsOut[m].data, &gxOpts);
                break;
            case TXTR_TTF_IA4:
                pixsrd = GX_DecodeIA4(mipWidth, mipHeight, mipsSzRem, mipsPtr, curOutSz, mipsOut[m].data, &gxOpts);
                break;
            case TXTR_TTF_IA8:
                pixsrd = GX_DecodeIA8(mipWidth, mipHeight, mipsSzRem, mipsPtr, curOutSz, mipsOut[m].data, &gxOpts);
                break;
            case TXTR_TTF_CI4:
                pixsrd = GX_DecodeCI4(mipWidth, mipHeight, mipsSzRem, mipsPtr, txtr->palSz, palette, curOutSz,
                    mipsOut[m].data, &gxOpts);
                break;
            case TXTR_TTF_CI8:
                pixsrd = GX_DecodeCI8(mipWidth, mipHeight, mipsSzRem, mipsPtr, txtr->palSz, palette, curOutSz,
                    mipsOut[m].data, &gxOpts);
                break;
            case TXTR_TTF_CI14X2:
                pixsrd = GX_DecodeCI14X2(mipWidth, mipHeight, mipsSzRem, mipsPtr, txtr->palSz, palette, curOutSz,
                    mipsOut[m].data, &gxOpts);
                break;
            case TXTR_TTF_R5G6B5:
                pixsrd = GX_DecodeR5G6B5(mipWidth, mipHeight, mipsSzRem, mipsPtr, curOutSz, mipsOut[m].data, &gxOpts);
                break;
            case TXTR_TTF_RGB5A3:
                pixsrd = GX_DecodeRGB5A3(mipWidth, mipHeight, mipsSzRem, mipsPtr, curOutSz, mipsOut[m].data, &gxOpts);
                break;
            case TXTR_TTF_RGBA8:
                pixsrd = GX_DecodeRGBA8(mipWidth, mipHeight, mipsSzRem, mipsPtr, curOutSz, mipsOut[m].data, &gxOpts);
                break;
            case TXTR_TTF_CMP:
                pixsrd = GX_DecodeCMP(mipWidth, mipHeight, mipsSzRem, mipsPtr, curOutSz, mipsOut[m].data, &gxOpts);
                break;
            default:
                free(palette);
                for (size_t m2 = 0, l2 = m + 1; m2 < l2; m2++)
                    TXTRMipmap_free(&mipsOut[m2]);
                return TXTR_DE_INVLDTEXFMT;
        }
        
        mipsSzRem -= pixsrd;
        mipsPtr += pixsrd;
        
        mipWidth /= 2;
        mipHeight /= 2;
    }
    free(palette);
    
    if (!catexit_loopSafety) {
        for (size_t m2 = 0, l2 = m + 1; m2 < l2; m2++)
            TXTRMipmap_free(&mipsOut[m2]);
        return TXTR_DE_INTERRUPTED;
    }
    
    *mipsOutCount = m;
    
    return TXTR_DE_SUCCESS;
}

TXTR_EXPORT char *TXTRReadError_ToStr(TXTRReadError_t txtrReadError) {
    switch (txtrReadError) {
        case TXTR_RE_SUCCESS:
            return "TXTR_RE_SUCCESS"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Operation was successful."
#endif
            ;
        case TXTR_RE_INVLDPARAMS:
            return "TXTR_RE_INVLDPARAMS"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid parameter(s) were passed to the function."
#endif
            ;
        case TXTR_RE_INVLDTEXFMT:
            return "TXTR_RE_INVLDTEXFMT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture format."
#endif
            ;
        case TXTR_RE_INVLDTEXWIDTH:
            return "TXTR_RE_INVLDTEXWIDTH"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture width. Must be greater than 0."
#endif
            ;
        case TXTR_RE_INVLDTEXHEIGHT:
            return "TXTR_RE_INVLDTEXHEIGHT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture height. Must be greater than 0."
#endif
            ;
        case TXTR_RE_INVLDMIPCNT:
            return "TXTR_RE_INVLDMIPCNT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid mipmap count. Must be greater than 0 and less than 12."
#endif
            ;
        case TXTR_RE_INVLDPALFMT:
            return "TXTR_RE_INVLDPALFMT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid palette format."
#endif
            ;
        case TXTR_RE_INVLDPALWIDTH:
            return "TXTR_RE_INVLDPALWIDTH"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid palette width."
#endif
            ;
        case TXTR_RE_INVLDPALHEIGHT:
            return "TXTR_RE_INVLDPALHEIGHT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid palette height."
#endif
            ;
        case TXTR_RE_INVLDPALSZ:
            return "TXTR_RE_INVLDPALSZ"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Palette size exceeded max size for the specified palette format."
#endif
            ;
        case TXTR_RE_MEMFAILPAL:
            return "TXTR_RE_MEMFAILPAL"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Failed to allocate memory for palette."
#endif
            ;
        case TXTR_RE_MEMFAILMIPS:
            return "TXTR_RE_MEMFAILMIPS"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Failed to allocate memory for input mipmap(s)."
#endif
            ;
        default:
            return "UNKNOWN"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid error code."
#endif
            ;
    }
}

TXTR_EXPORT char *TXTRDecodeError_ToStr(TXTRDecodeError_t txtrDecodeError) {
    switch (txtrDecodeError) {
        case TXTR_DE_SUCCESS:
            return "TXTR_DE_SUCCESS"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Operation was successful."
#endif
            ;
        case TXTR_DE_INVLDPARAMS:
            return "TXTR_DE_INVLDPARAMS"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid parameter(s) were passed to the function."
#endif
            ;
        case TXTR_DE_INVLDTEXPAL:
            return "TXTR_DE_INVLDTEXPAL"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": The palette is of a null pointer."
#endif
            ;
        case TXTR_DE_INVLDTEXMIPS:
            return "TXTR_DE_INVLDTEXMIPS"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": The mipmaps is of a null pointer."
#endif
            ;
        case TXTR_DE_INVLDTEXWIDTH:
            return "TXTR_DE_INVLDTEXWIDTH"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture width. Must be greater than 0."
#endif
            ;
        case TXTR_DE_INVLDTEXHEIGHT:
            return "TXTR_DE_INVLDTEXHEIGHT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture height. Must be greater than 0."
#endif
            ;
        case TXTR_DE_INVLDTEXMIPCNT:
            return "TXTR_DE_INVLDTEXMIPCNT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid mipmap count. Must be greater than 0 and less than 12."
#endif
            ;
        case TXTR_DE_INVLDTEXFMT:
            return "TXTR_DE_INVLDTEXFMT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture format."
#endif
            ;
        case TXTR_DE_INVLDPALFMT:
            return "TXTR_DE_INVLDPALFMT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid palette format."
#endif
            ;
        case TXTR_DE_MEMFAILPAL:
            return "TXTR_DE_MEMFAILPAL"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Failed to allocate memory for the palette."
#endif
            ;
        case TXTR_DE_MEMFAILMIP:
            return "TXTR_DE_MEMFAILMIP"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Failed to allocate memory for a mipmap."
#endif
            ;
        case TXTR_DE_INTERRUPTED:
            return "TXTR_DE_INTERRUPTED"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Execution was interrupted by a signal."
#endif
            ;
        case TXTR_DE_FAILDECPAL:
            return "TXTR_DE_FAILDECPAL"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Failed to decode palette."
#endif
            ;
        default:
            return "UNKNOWN"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid error code."
#endif
            ;
    }
}
#endif

#ifdef TXTR_INCLUDE_ENCODE
TXTR_EXPORT void TXTRRawMipmap_free(TXTRRawMipmap_t *mip) {
    free(mip->data);
    mip->data = NULL;
}

TXTR_EXPORT TXTREncodeError_t TXTR_Encode(TXTRFormat_t texFmt, TXTRPaletteFormat_t palFmt, uint16_t width,
uint16_t height, size_t dataSz, uint32_t *data, TXTR_t *txtr, TXTRRawMipmap_t txtrMips[11],
TXTREncodeOptions_t *opts) {
    if (txtr) {
        txtr->pal = NULL;
        txtr->mips = NULL;
    }
    
    if (!dataSz || !data || !txtr || !txtrMips || !opts)
        return TXTR_EE_INVLDPARAMS;
    
    if (!width)
        return TXTR_EE_INVLDTEXWIDTH;
    
    if (!height)
        return TXTR_EE_INVLDTEXHEIGHT;
    
    if (opts->mipLimit > 11)
        return TXTR_EE_INVLDTEXMIPLMT;
    
    if (!opts->widthLimit || opts->widthLimit > width)
        return TXTR_EE_INVLDTEXWIDTHLMT;
    
    if (!opts->heightLimit || opts->heightLimit > height)
        return TXTR_EE_INVLDTEXHEIGHTLMT;
    
    if (opts->avgType < GX_AT_MIN || opts->avgType > GX_AT_MAX)
        return TXTR_EE_INVLDGXAVGTYPE;
    
    if (opts->stbirEdge < STBIR_EDGE_CLAMP || opts->stbirEdge > STBIR_EDGE_ZERO)
        return TXTR_EE_INVLDSTBIREDGEMODE;
    
    if (opts->stbirFilter < STBIR_FILTER_DEFAULT || opts->stbirFilter > STBIR_FILTER_POINT_SAMPLE)
        return TXTR_EE_INVLDSTBIRFILTER;
    
    if (opts->ditherType < GX_DT_MIN || opts->ditherType > GX_DT_MAX)
        return TXTR_EE_INVLDGXDITHERTYPE;
    
    if (opts->squishMetric && opts->squishMetricSz != 3)
        return TXTR_EE_INVLDSQUISHMETRICSZ;
    
    if (texFmt < TXTR_TTF_I4 || texFmt > TXTR_TTF_CMP)
        return TXTR_EE_INVLDTEXFMT;
    
    bool isIndexed = TXTR_IsIndexed(texFmt);
    if (isIndexed && (palFmt < TXTR_TPF_IA8 || palFmt > TXTR_TPF_RGB5A3))
        return TXTR_EE_INVLDPALFMT;
    
    // Mipmaps make no sense in palette formats --- the palette would enlarge past any feasable size!
    if (isIndexed && opts->mipLimit > 1)
        return TXTR_EE_TRYMIPPALFMT;
    
    txtr->hdr.format = texFmt;
    txtr->hdr.width = width;
    txtr->hdr.height = height;
    
    txtr->palHdr.format = palFmt;
    
    txtr->mipsSz = 0;
    
    txtr->isIndexed = isIndexed;
    
    GXEncodeOptions_t gxOpts = {
        .flipX = opts->flipX,
        .flipY = opts->flipY,
        .avgType = opts->avgType,
        .ditherType = opts->ditherType,
        .squishFlags = opts->squishFlags,
        .squishMetricSz = opts->squishMetricSz,
        .squishMetric = opts->squishMetric
    };
    
    uint32_t *srcPixsPtr = data;
    if (txtr->isIndexed) {
        size_t palMaxSz = TXTR_GetMaxPalSz(txtr->hdr.format);;
        
        uint32_t *palette = malloc(palMaxSz * sizeof(uint32_t));
        if (!palette)
            return TXTR_EE_MEMFAILPAL;
        
        size_t srcPxsSz = dataSz * sizeof(uint32_t);
        srcPixsPtr = malloc(srcPxsSz);
        if (!srcPixsPtr) {
            free(palette);
            return TXTR_EE_MEMFAILSRCPXS;
        }
        
        if (GX_BuildPalette(width, height, width * height, data, palMaxSz, palette, width * height, srcPixsPtr,
        &txtr->palSz, &gxOpts)) {
            free(srcPixsPtr);
            free(palette);
            return TXTR_EE_FAILBUILDPAL;
        }
        
        txtr->pal = malloc(txtr->palSz * sizeof(uint16_t));
        if (!txtr->pal) {
            free(srcPixsPtr);
            free(palette);
            return TXTR_EE_MEMFAILPAL;
        }
        
        bool palFail = false;
        switch (txtr->palHdr.format) {
            case TXTR_TPF_IA8:
                palFail = GX_EncodePaletteIA8(txtr->palSz, palette, txtr->pal, &gxOpts);
                break;
            case TXTR_TPF_R5G6B5:
                palFail = GX_EncodePaletteR5G6B5(txtr->palSz, palette, txtr->pal, &gxOpts);
                break;
            case TXTR_TPF_RGB5A3:
                palFail = GX_EncodePaletteRGB5A3(txtr->palSz, palette, txtr->pal, &gxOpts);
                break;
            default:
                free(srcPixsPtr);
                free(palette);
                free(txtr->pal);
                txtr->pal = NULL;
                return TXTR_EE_INVLDPALFMT;
        }
        
        free(palette);
        
        if (palFail) {
            free(srcPixsPtr);
            free(txtr->pal);
            txtr->pal = NULL;
            return TXTR_EE_FAILENCPAL;
        }
        
        // TODO: Palette dimensions based on imperfect square root (as an option? this could go with handling TGA
        // palette)
        txtr->palHdr.width = txtr->palSz;
        txtr->palHdr.height = 1;
    }
    
    TXTRRawMipmap_t *mipsPtr = txtrMips;
    uint16_t mipWidth = width;
    uint16_t mipHeight = height;
    size_t m = 0;
    size_t l = 0;
    for (m = 0, l = !opts->mipLimit ? (!txtr->isIndexed ? 11 : 1) : opts->mipLimit; catexit_loopSafety && m < l; m++) {
        if (mipWidth < opts->widthLimit || mipHeight < opts->heightLimit)
            break;
        
        if (m) {
            if (srcPixsPtr == data) {
                size_t srcPxsSz = dataSz * sizeof(uint32_t);
                srcPixsPtr = malloc(srcPxsSz);
                if (!srcPixsPtr) {
                    free(txtr->pal);
                    txtr->pal = NULL;
                    for (size_t m2 = 0, l2 = m; m2 < l2; m2++)
                        TXTRRawMipmap_free(&txtrMips[m2]);
                    return TXTR_EE_MEMFAILSRCPXS;
                }
            }
            
            if (!stbir_resize(data, width, height, 0, srcPixsPtr, mipWidth, mipHeight, 0,
            // TODO: Premultiplied alpha support
#ifdef TXTR_COMP_RGBA
            STBIR_RGBA,
#elif defined(TXTR_COMP_ARGB)
            STBIR_ARGB,
#elif defined(TXTR_COMP_ABGR)
            STBIR_ABGR,
#else // TXTR_COMP_BGRA
            STBIR_BGRA,
#endif
            STBIR_TYPE_UINT8, opts->stbirEdge, opts->stbirFilter) || !catexit_loopSafety) {
                free(txtr->pal);
                txtr->pal = NULL;
                if (srcPixsPtr != data)
                    free(srcPixsPtr);
                for (size_t m2 = 0, l2 = m; m2 < l2; m2++)
                    TXTRRawMipmap_free(&txtrMips[m2]);
                return catexit_loopSafety ? TXTR_EE_RESIZEFAIL : TXTR_EE_INTERRUPTED;
            }
        }
        
        size_t mipSz = TXTR_CalcMipSz(txtr->hdr.format, mipWidth, mipHeight);
        txtr->mipsSz += mipSz;
        (*mipsPtr).size = mipSz;
        (*mipsPtr).data = malloc(mipSz);
        if (!(*mipsPtr).data) {
            free(txtr->pal);
            txtr->pal = NULL;
            if (srcPixsPtr != data)
                free(srcPixsPtr);
            for (size_t m2 = 0, l2 = m; m2 < l2; m2++)
                TXTRRawMipmap_free(&txtrMips[m2]);
            return TXTR_EE_MEMFAILMIP;
        }
        
        switch (txtr->hdr.format) {
            case TXTR_TTF_I4:
                GX_EncodeI4(mipWidth, mipHeight, mipWidth * mipHeight, srcPixsPtr, mipSz, (*mipsPtr).data, &gxOpts);
                break;
            case TXTR_TTF_I8:
                GX_EncodeI8(mipWidth, mipHeight, mipWidth * mipHeight, srcPixsPtr, mipSz, (*mipsPtr).data, &gxOpts);
                break;
            case TXTR_TTF_IA4:
                GX_EncodeIA4(mipWidth, mipHeight, mipWidth * mipHeight, srcPixsPtr, mipSz, (*mipsPtr).data, &gxOpts);
                break;
            case TXTR_TTF_IA8:
                GX_EncodeIA8(mipWidth, mipHeight, mipWidth * mipHeight, srcPixsPtr, mipSz, (*mipsPtr).data, &gxOpts);
                break;
            case TXTR_TTF_CI4:
                GX_EncodeCI4(mipWidth, mipHeight, mipWidth * mipHeight, srcPixsPtr, txtr->palSz, mipSz,
                    (*mipsPtr).data, &gxOpts);
                break;
            case TXTR_TTF_CI8:
                GX_EncodeCI8(mipWidth, mipHeight, mipWidth * mipHeight, srcPixsPtr, txtr->palSz, mipSz,
                    (*mipsPtr).data, &gxOpts);
                break;
            case TXTR_TTF_CI14X2:
                GX_EncodeCI14X2(mipWidth, mipHeight, mipWidth * mipHeight, srcPixsPtr, txtr->palSz, mipSz,
                    (*mipsPtr).data, &gxOpts);
                break;
            case TXTR_TTF_R5G6B5:
                GX_EncodeR5G6B5(mipWidth, mipHeight, mipWidth * mipHeight, srcPixsPtr, mipSz, (*mipsPtr).data,
                    &gxOpts);
                break;
            case TXTR_TTF_RGB5A3:
                GX_EncodeRGB5A3(mipWidth, mipHeight, mipWidth * mipHeight, srcPixsPtr, mipSz, (*mipsPtr).data,
                    &gxOpts);
                break;
            case TXTR_TTF_RGBA8:
                GX_EncodeRGBA8(mipWidth, mipHeight, mipWidth * mipHeight, srcPixsPtr, mipSz, (*mipsPtr).data,
                    &gxOpts);
                break;
            case TXTR_TTF_CMP:
                GX_EncodeCMP(mipWidth, mipHeight, mipWidth * mipHeight, srcPixsPtr, mipSz, (*mipsPtr).data, &gxOpts);
                break;
            default:
                free(txtr->pal);
                txtr->pal = NULL;
                if (srcPixsPtr != data)
                    free(srcPixsPtr);
                for (size_t m2 = 0, l2 = m + 1; m2 < l2; m2++)
                    TXTRRawMipmap_free(&txtrMips[m2]);
                return TXTR_EE_INVLDTEXFMT;
        }
        
        mipsPtr++;
        
        mipWidth /= 2;
        mipHeight /= 2;
    }
    if (srcPixsPtr != data)
        free(srcPixsPtr);
    
    if (!catexit_loopSafety) {
        free(txtr->pal);
        txtr->pal = NULL;
        for (size_t m2 = 0, l2 = m + 1; m2 < l2; m2++)
            TXTRRawMipmap_free(&txtrMips[m2]);
        return TXTR_EE_INTERRUPTED;
    }
    
    txtr->hdr.mipCount = m;
    
    return  TXTR_EE_SUCCESS;
}

TXTR_EXPORT TXTRWriteError_t TXTR_Write(TXTR_t *txtr, TXTRRawMipmap_t mips[11], size_t *txtrDataSz,
uint8_t **txtrData) {
    if (!txtr || !mips || (txtr->isIndexed && !TXTR_IsIndexed(txtr->hdr.format)))
        return TXTR_WE_INVLDPARAMS;
    
    if (!txtr->hdr.height)
        return TXTR_WE_INVLDTEXWIDTH;
    
    if (!txtr->hdr.width)
        return TXTR_WE_INVLDTEXHEIGHT;
    
    if (!txtr->hdr.mipCount || txtr->hdr.mipCount > 11)
        return TXTR_WE_INVLDMIPCNT;
    
    if (txtr->hdr.format < TXTR_TTF_I4 || txtr->hdr.format > TXTR_TTF_CMP)
        return TXTR_WE_INVLDTEXFMT;
    
    if (!txtr->mipsSz)
        return TXTR_WE_INVLDTEXMIPS;
    
    size_t dataSz = sizeof(TXTRHeader_t) + txtr->mipsSz;
    if (txtr->isIndexed)
        dataSz += sizeof(TXTRPaletteHeader_t) + (txtr->palSz * sizeof(uint16_t));
    uint8_t *data = malloc(dataSz);
    if (!data)
        return TXTR_WE_MEMFAILMIPS;
    
    uint8_t *dPtr = data;
    
    Dat_SetU32BE(dPtr, (uint32_t) txtr->hdr.format);
    dPtr += sizeof(uint32_t);
    
    Dat_SetU16BE(dPtr, txtr->hdr.width);
    dPtr += sizeof(uint16_t);
    
    Dat_SetU16BE(dPtr, txtr->hdr.height);
    dPtr += sizeof(uint16_t);
    
    Dat_SetU32BE(dPtr, txtr->hdr.mipCount);
    dPtr += sizeof(uint32_t);
    
    if (txtr->isIndexed) {
        if (txtr->palHdr.format < TXTR_TPF_IA8 || txtr->palHdr.format > TXTR_TPF_RGB5A3)
            return TXTR_WE_INVLDPALFMT;
        
        if (!txtr->palHdr.width)
            return TXTR_WE_INVLDPALWIDTH;
        
        if (!txtr->palHdr.height)
            return TXTR_WE_INVLDPALHEIGHT;
        
        Dat_SetU32BE(dPtr, (uint32_t) txtr->palHdr.format);
        dPtr += sizeof(uint32_t);
        
        Dat_SetU16BE(dPtr, txtr->palHdr.width);
        dPtr += sizeof(uint16_t);
        
        Dat_SetU16BE(dPtr, txtr->palHdr.height);
        dPtr += sizeof(uint16_t);
        
        if (!txtr->pal)
            return TXTR_WE_INVLDTEXPAL;
        
        if (!txtr->palSz || txtr->palSz > TXTR_GetMaxPalSz(txtr->hdr.format))
            return TXTR_WE_INVLDPALSZ;
        
        size_t aPalSz = txtr->palSz * sizeof(uint16_t);
        memcpy(dPtr, txtr->pal, aPalSz);
        dPtr += aPalSz;
    }
    
    for (size_t m = 0; catexit_loopSafety && m < txtr->hdr.mipCount; m++) {
        memcpy(dPtr, mips[m].data, mips[m].size);
        dPtr += mips[m].size;
    }
    
    if (!catexit_loopSafety) {
        free(data);
        return TXTR_WE_INTERRUPTED;
    }
    
    *txtrDataSz = dataSz;
    *txtrData = data;
    
    return TXTR_WE_SUCCESS;
}

TXTR_EXPORT char *TXTREncodeError_ToStr(TXTREncodeError_t txtrEncodeError) {
    switch (txtrEncodeError) {
        case TXTR_EE_SUCCESS:
            return "TXTR_EE_SUCCESS"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Operation was successful."
#endif
            ;
        case TXTR_EE_INVLDPARAMS:
            return "TXTR_EE_INVLDPARAMS"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid parameter(s) were passed to the function."
#endif
            ;
        case TXTR_EE_INVLDTEXWIDTH:
            return "TXTR_EE_INVLDTEXWIDTH"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture width. Must be greater than 0."
#endif
            ;
        case TXTR_EE_INVLDTEXHEIGHT:
            return "TXTR_EE_INVLDTEXHEIGHT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture height. Must be greater than 0."
#endif
            ;
        case TXTR_EE_INVLDTEXMIPLMT:
            return "TXTR_EE_INVLDTEXMIPLMT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid mipmap limit. This must be less than 12."
#endif
            ;
        case TXTR_EE_INVLDTEXWIDTHLMT:
            return "TXTR_EE_INVLDTEXWIDTHLMT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture width limit. Must be greater than 0 and less than or equal to image width."
#endif
            ;
        case TXTR_EE_INVLDTEXHEIGHTLMT:
            return "TXTR_EE_INVLDTEXHEIGHTLMT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture height height. Must be greater than 0 and less than or equal to image height."
#endif
            ;
        case TXTR_EE_INVLDGXAVGTYPE:
            return "TXTR_EE_INVLDGXAVGTYPE"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid GX average type."
#endif
            ;
        case TXTR_EE_INVLDSTBIREDGEMODE:
            return "TXTR_EE_INVLDSTBIREDGEMODE"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid image resize edge mode."
#endif
            ;
        case TXTR_EE_INVLDSTBIRFILTER:
            return "TXTR_EE_INVLDSTBIRFILTER"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid image resize filter."
#endif
            ;
        case TXTR_EE_MEMFAILSRCPXS:
            return "TXTR_EE_MEMFAILSRCPXS"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Failed to allocate memory for a image."
#endif
            ;
        case TXTR_EE_MEMFAILMIP:
            return "TXTR_EE_MEMFAILMIP"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Failed to allocate memory for a raw mipmap."
#endif
            ;
        case TXTR_EE_INVLDTEXFMT:
            return "TXTR_EE_INVLDTEXFMT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture format."
#endif
            ;
        case TXTR_EE_INVLDPALFMT:
            return "TXTR_EE_INVLDPALFMT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid palette format."
#endif
            ;
        case TXTR_EE_MEMFAILPAL:
            return "TXTR_EE_MEMFAILPAL"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Failed to allocate memory for output palette."
#endif
            ;
        case TXTR_EE_FAILBUILDPAL:
            return "TXTR_EE_FAILBUILDPAL"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Failed to build output palette."
#endif
            ;
        case TXTR_EE_RESIZEFAIL:
            return "TXTR_EE_RESIZEFAIL"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Failed to resize the image data while mipmapping."
#endif
            ;
        case TXTR_EE_TRYMIPPALFMT:
            return "TXTR_EE_TRYMIPPALFMT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Palette formats cannot have mipmaps."
#endif
            ;
        case TXTR_EE_INTERRUPTED:
            return "TXTR_EE_INTERRUPTED"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Execution was interrupted by a signal."
#endif
            ;
        case TXTR_EE_FAILENCPAL:
            return "TXTR_EE_FAILENCPAL"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Failed to encode palette."
#endif
            ;
        case TXTR_EE_INVLDSQUISHMETRICSZ:
            return "TXTR_EE_INVLDSQUISHMETRICSZ"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid size for squish metric."
#endif
            ;
        case TXTR_EE_INVLDGXDITHERTYPE:
            return "TXTR_EE_INVLDGXDITHERTYPE"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid GX dither tpye."
#endif
            ;
        default:
            return "UNKNOWN"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid error code."
#endif
            ;
    }
}

TXTR_EXPORT char *TXTRWriteError_ToStr(TXTRWriteError_t txtrWriteError) {
    switch (txtrWriteError) {
        case TXTR_WE_SUCCESS:
            return "TXTR_WE_SUCCESS"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Operation was successful."
#endif
            ;
        case TXTR_WE_INVLDPARAMS:
            return "TXTR_WE_INVLDPARAMS"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid parameter(s) were passed to the function."
#endif
            ;
        case TXTR_WE_INVLDTEXFMT:
            return "TXTR_WE_INVLDTEXFMT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture format."
#endif
            ;
        case TXTR_WE_INVLDTEXWIDTH:
            return "TXTR_WE_INVLDTEXWIDTH"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture width. Must be greater than 0."
#endif
            ;
        case TXTR_WE_INVLDTEXHEIGHT:
            return "TXTR_WE_INVLDTEXHEIGHT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid texture height. Must be greater than 0."
#endif
            ;
        case TXTR_WE_INVLDMIPCNT:
            return "TXTR_WE_INVLDMIPCNT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid mipmap count. Must be greater than 0 and less than 12."
#endif
            ;
        case TXTR_WE_INVLDPALFMT:
            return "TXTR_WE_INVLDPALFMT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid palette format."
#endif
            ;
        case TXTR_WE_INVLDPALWIDTH:
            return "TXTR_WE_INVLDPALWIDTH"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid palette width."
#endif
            ;
        case TXTR_WE_INVLDPALHEIGHT:
            return "TXTR_WE_INVLDPALHEIGHT"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid palette height."
#endif
            ;
        case TXTR_WE_INVLDPALSZ:
            return "TXTR_WE_INVLDPALSZ"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Palette size exceeded max size for the specified palette format."
#endif
            ;
        case TXTR_WE_INVLDTEXPAL:
            return "TXTR_WE_INVLDTEXPAL"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": The palette is of a null pointer."
#endif
            ;
        case TXTR_WE_INVLDTEXMIPS:
            return "TXTR_WE_INVLDTEXMIPS"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": The mipmaps is of a null pointer."
#endif
            ;
        case TXTR_WE_MEMFAILMIPS:
            return "TXTR_WE_MEMFAILMIPS"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Failed to allocate memory for output mipmap(s)."
#endif
            ;
        case TXTR_WE_INTERRUPTED:
            return "TXTR_WE_INTERRUPTED"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Execution was interrupted by a signal."
#endif
            ;
        default:
            return "UNKNOWN"
#ifdef TXTR_INCLUDE_ERROR_STRINGS
                ": Invalid error code."
#endif
            ;
    }
}
#endif

#undef STB_IMAGE_RESIZE_IMPLEMENTATION
