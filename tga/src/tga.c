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

#include <tga.h>

#include <stdext/cdata.h>
#include <stdext/catexit.h>
#include <stdext/cmath.h>

TGA_EXPORT void TGA_free(TGA_t *tga) {
    free(tga->id);
    tga->id = NULL;
    free(tga->data);
    tga->data = NULL;
}

TGA_EXPORT bool TGA_IsNewFormat(size_t dataSz, uint8_t *data) {
    return dataSz && data && !memcmp((data + dataSz) - TGA_FOOTERSIGOFFS, TGA_FOOTERSIG, TGA_FOOTERSIGOFFS);
}

#ifdef TGA_INCLUDE_DECODE
TGAReadError_t TGA_Read(TGA_t *tga, size_t dataSz, uint8_t *data) {
    if (!tga || !dataSz || !data)
        return TGA_RE_INVLDPARAMS;
    
    uint8_t *dPtr = data;
    
    tga->hdr.idLength = *dPtr;
    dPtr++;
    
    tga->hdr.colorMapType = (TGAColorMapType_t) *dPtr;
    if (tga->hdr.colorMapType != TGA_CMT_NOCOLORMAP)
        return TGA_RE_CLRMAPPRESENT;
    dPtr++;
    
    tga->hdr.imageType = (TGAImageType_t) *dPtr;
    if (tga->hdr.imageType != TGA_IMT_COLOR)
        return TGA_RE_NOTACOLORTGA;
    dPtr++;
    
    tga->hdr.colorMapSpec.firstEntryIndex = Dat_GetU16LE(dPtr);
    if (tga->hdr.colorMapSpec.firstEntryIndex != 0)
        return TGA_RE_CLRMAPPRESENT;
    dPtr += sizeof(uint16_t);
    
    tga->hdr.colorMapSpec.colorMapLength = Dat_GetU16LE(dPtr);
    if (tga->hdr.colorMapSpec.colorMapLength != 0)
        return TGA_RE_CLRMAPPRESENT;
    dPtr += sizeof(uint16_t);
    
    tga->hdr.colorMapSpec.colorMapEntrySize = *dPtr;
    if (tga->hdr.colorMapSpec.colorMapEntrySize != 0)
        return TGA_RE_CLRMAPPRESENT;
    dPtr++;
    
    tga->hdr.imageSpec.xOrigin = Dat_GetU16LE(dPtr);
    if (tga->hdr.imageSpec.xOrigin != 0)
        return TGA_RE_INVLDXORIGIN;
    dPtr += sizeof(uint16_t);
    
    tga->hdr.imageSpec.yOrigin = Dat_GetU16LE(dPtr);
    if (tga->hdr.imageSpec.yOrigin != 0)
        return TGA_RE_INVLDYORIGIN;
    dPtr += sizeof(uint16_t);
    
    tga->hdr.imageSpec.width = Dat_GetU16LE(dPtr);
    if (tga->hdr.imageSpec.width == 0)
        return TGA_RE_INVLDWIDTH;
    dPtr += sizeof(uint16_t);
    
    tga->hdr.imageSpec.height = Dat_GetU16LE(dPtr);
    if (tga->hdr.imageSpec.height == 0)
        return TGA_RE_INVLDHEIGHT;
    dPtr += sizeof(uint16_t);
    
    tga->hdr.imageSpec.pixelDepth = *dPtr;
    if (tga->hdr.imageSpec.pixelDepth != 32)
        return TGA_RE_INVLDPXLDEP;
    dPtr++;
    
    tga->hdr.imageSpec.imageDesc = *dPtr;
    if (TGAImageDescriptor_alphaBitLength(tga->hdr.imageSpec.imageDesc) != 8)
        return TGA_RE_INVLDALPHBITSZ;
    dPtr++;
    
    if (tga->hdr.idLength) {
        tga->id = malloc(tga->hdr.idLength);
        if (!tga->id) {
            tga->hdr.idLength = 0;
            return TGA_RE_MEMFAILID;
        }
        memcpy(tga->id, dPtr, tga->hdr.idLength);
        dPtr += tga->hdr.idLength;
    } else
        tga->id = NULL;
    
    tga->dataSz = tga->hdr.imageSpec.width * tga->hdr.imageSpec.height;
    tga->data = malloc(tga->dataSz * sizeof(uint32_t));
    if (!tga->data) {
        if (tga->hdr.idLength) {
            tga->hdr.idLength = 0;
            free(tga->id);
            tga->id = NULL;
        }
        tga->dataSz = 0;
        return TGA_RE_MEMFAILDATA;
    }
    for (size_t y = 0; catexit_loopSafety && y < tga->hdr.imageSpec.height; y++) {
        for (size_t x = 0; catexit_loopSafety && x < tga->hdr.imageSpec.width; x++) {
            size_t fy = !TGAImageDescriptor_flipYOrigin(tga->hdr.imageSpec.imageDesc) ? Math_FlipSz(y, tga->hdr.imageSpec.height) : y;
            size_t fx = TGAImageDescriptor_flipXOrigin(tga->hdr.imageSpec.imageDesc) ? Math_FlipSz(x, tga->hdr.imageSpec.width) : x;
            
            *(tga->data + (y * tga->hdr.imageSpec.width + x)) = Dat_GetU32LE(dPtr + ((fy * tga->hdr.imageSpec.width + fx) * 4));
        }
    }
    
    tga->isNewFmt = TGA_IsNewFormat(dataSz, data);
    if (tga->isNewFmt) {
        dPtr = (data + dataSz) - sizeof(TGAFileFooter_t);
        
        tga->ftr.extAreaOffs = Dat_GetU32LE(dPtr);
        dPtr += sizeof(uint32_t);
        
        tga->ftr.devAreaOffs = Dat_GetU32LE(dPtr);
        dPtr += sizeof(uint32_t);
        
        memcpy(tga->ftr.signature, dPtr, TGA_FOOTERSIGOFFS);
    }
    
    return TGA_RE_SUCCESS;
}

char *TGAReadError_ToStr(TGAReadError_t tgaReadError) {
    switch (tgaReadError) {
        case TGA_RE_SUCCESS:
            return "TGA_RE_SUCCESS"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Operation was successful."
#endif
            ;
        case TGA_RE_INVLDPARAMS:
            return "TGA_RE_INVLDPARAMS"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid parameter(s) were passed to the function."
#endif
            ;
        case TGA_RE_CLRMAPPRESENT:
            return "TGA_RE_CLRMAPPRESENT"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": TGAs that have a color map are not supported."
#endif
            ;
        case TGA_RE_NOTACOLORTGA:
            return "TGA_RE_NOTACOLORTGA"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": TGAs that are not 32-bit true color are not supported"
#endif
            ;
        case TGA_RE_INVLDXORIGIN:
            return "TGA_RE_INVLDXORIGIN"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid X origin. Must be 0."
#endif
            ;
        case TGA_RE_INVLDYORIGIN:
            return "TGA_RE_INVLDYORIGIN"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid Y origin. Must be 0."
#endif
            ;
        case TGA_RE_INVLDWIDTH:
            return "TGA_RE_INVLDWIDTH"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid width. Must be greater than 0 ."
#endif
            ;
        case TGA_RE_INVLDHEIGHT:
            return "TGA_RE_INVLDHEIGHT"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid height. Must be greater than 0."
#endif
            ;
        case TGA_RE_INVLDPXLDEP:
            return "TGA_RE_INVLDPXLDEP"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid pixel depth. Must be 32."
#endif
            ;
        case TGA_RE_INVLDALPHBITSZ:
            return "TGA_RE_INVLDALPHBITSZ"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid alpha bit length. Must be 8."
#endif
            ;
        case TGA_RE_MEMFAILID:
            return "TGA_RE_MEMFAILID"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Failed to allocate memory for ID."
#endif
            ;
        case TGA_RE_MEMFAILDATA:
            return "TGA_RE_MEMFAILDATA"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Failed to allocate memory for data."
#endif
            ;
        default:
            return "UNKNOWN"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid error code."
#endif
            ;
    }
}
#endif

#ifdef TGA_INCLUDE_ENCODE
TGAWriteError_t TGA_Write(TGA_t *tga, size_t *tgaDataSz, uint8_t **tgaData) {
    if (!tga || !tgaDataSz || !tgaData)
        return TGA_WE_INVLDPARAMS;
    
    if (tga->hdr.colorMapType != TGA_CMT_NOCOLORMAP)
        return TGA_WE_CLRMAPPRESENT;
    
    if (tga->hdr.imageType != TGA_IMT_COLOR)
        return TGA_WE_NOTACOLORTGA;
    
    if (tga->hdr.colorMapSpec.firstEntryIndex != 0)
        return TGA_WE_CLRMAPPRESENT;
    
    if (tga->hdr.colorMapSpec.colorMapLength != 0)
        return TGA_WE_CLRMAPPRESENT;
    
    if (tga->hdr.colorMapSpec.colorMapEntrySize != 0)
        return TGA_WE_CLRMAPPRESENT;
    
    if (tga->hdr.imageSpec.xOrigin != 0)
        return TGA_WE_INVLDXORIGIN;
    
    if (tga->hdr.imageSpec.yOrigin != 0)
        return TGA_WE_INVLDYORIGIN;
    
    if (tga->hdr.imageSpec.width == 0)
        return TGA_WE_INVLDWIDTH;
    
    if (tga->hdr.imageSpec.height == 0)
        return TGA_WE_INVLDHEIGHT;
    
    if (tga->hdr.imageSpec.pixelDepth != 32)
        return TGA_WE_INVLDPXLDEP;
    
    if (TGAImageDescriptor_alphaBitLength(tga->hdr.imageSpec.imageDesc) != 8)
        return TGA_WE_INVLDALPHBITSZ;
    
    if (!tga->dataSz || !tga->data)
        return TGA_WE_INVLDDATA;
    
    if (tga->hdr.idLength && !tga->id)
        return TGA_WE_INVLDID;
    
    if (tga->isNewFmt && memcmp(tga->ftr.signature, TGA_FOOTERSIG, TGA_FOOTERSIGOFFS))
        return TGA_WE_INVLDSIGNATURE;
    
    size_t aDataSz = tga->dataSz * sizeof(uint32_t);
    size_t outDataSz = sizeof(TGAFileHeader_t) + tga->hdr.idLength + aDataSz;
    if (tga->isNewFmt)
        outDataSz += sizeof(TGAFileFooter_t);
    
    uint8_t *outData = malloc(outDataSz);
    if (!outData)
        return TGA_WE_MEMFAILDATA;
    
    uint8_t *dPtr = outData;
    
    *dPtr = tga->hdr.idLength;
    dPtr++;
    
    *dPtr = (uint8_t) tga->hdr.colorMapType;
    dPtr++;
    
    *dPtr = (uint8_t) tga->hdr.imageType;
    dPtr++;
    
    Dat_SetU16LE(dPtr, tga->hdr.colorMapSpec.firstEntryIndex);
    dPtr += sizeof(uint16_t);
    
    Dat_SetU16LE(dPtr, tga->hdr.colorMapSpec.colorMapLength);
    dPtr += sizeof(uint16_t);
    
    *dPtr = tga->hdr.colorMapSpec.colorMapEntrySize;
    dPtr++;
    
    Dat_SetU16LE(dPtr, tga->hdr.imageSpec.xOrigin);
    dPtr += sizeof(uint16_t);
    
    Dat_SetU16LE(dPtr, tga->hdr.imageSpec.yOrigin);
    dPtr += sizeof(uint16_t);
    
    Dat_SetU16LE(dPtr, tga->hdr.imageSpec.width);
    dPtr += sizeof(uint16_t);
    
    Dat_SetU16LE(dPtr, tga->hdr.imageSpec.height);
    dPtr += sizeof(uint16_t);
    
    *dPtr = tga->hdr.imageSpec.pixelDepth;
    dPtr++;
    
    *dPtr = tga->hdr.imageSpec.imageDesc;
    dPtr++;
    
    if (tga->hdr.idLength) {
        memcpy(dPtr, tga->id, tga->hdr.idLength);
        dPtr += tga->hdr.idLength;
    }
    
    memcpy(dPtr, tga->data, aDataSz);
    dPtr += aDataSz;
    
    if (tga->isNewFmt) {
        Dat_SetU32LE(dPtr, tga->ftr.extAreaOffs);
        dPtr += sizeof(uint32_t);
        Dat_SetU32LE(dPtr, tga->ftr.devAreaOffs);
        dPtr += sizeof(uint32_t);
        memcpy(dPtr, tga->ftr.signature, TGA_FOOTERSIGOFFS);
    }
    
    // TODO: Extension area, color map, developer area, black and white formats, etc.
    // Essentially: TGA -> BGRA and BGRA -> TGA
    
    *tgaDataSz = outDataSz;
    *tgaData = outData;
    
    return TGA_WE_SUCCESS;
}

char *TGAWriteError_ToStr(TGAWriteError_t tgaWriteError) {
    switch (tgaWriteError) {
        case TGA_WE_SUCCESS:
            return "TGA_WE_SUCCESS"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Operation was successful."
#endif
            ;
        case TGA_WE_INVLDPARAMS:
            return "TGA_WE_INVLDPARAMS"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid parameter(s) were passed to the function."
#endif
            ;
        case TGA_WE_CLRMAPPRESENT:
            return "TGA_WE_CLRMAPPRESENT"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": TGAs that have a color map are not supported."
#endif
            ;
        case TGA_WE_NOTACOLORTGA:
            return "TGA_WE_NOTACOLORTGA"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": TGAs that are not 32-bit true color are not supported"
#endif
            ;
        case TGA_WE_INVLDXORIGIN:
            return "TGA_WE_INVLDXORIGIN"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid X origin. Must be 0."
#endif
            ;
        case TGA_WE_INVLDYORIGIN:
            return "TGA_WE_INVLDYORIGIN"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid Y origin. Must be 0."
#endif
            ;
        case TGA_WE_INVLDWIDTH:
            return "TGA_WE_INVLDWIDTH"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid width. Must be greater than 0 ."
#endif
            ;
        case TGA_WE_INVLDHEIGHT:
            return "TGA_WE_INVLDHEIGHT"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid height. Must be greater than 0."
#endif
            ;
        case TGA_WE_INVLDPXLDEP:
            return "TGA_WE_INVLDPXLDEP"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid pixel depth. Must be 32."
#endif
            ;
        case TGA_WE_INVLDALPHBITSZ:
            return "TGA_WE_INVLDALPHBITSZ"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid alpha bit length. Must be 8."
#endif
            ;
        case TGA_WE_MEMFAILDATA:
            return "TGA_WE_MEMFAILDATA"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Failed to allocate memory for data."
#endif
            ;
        case TGA_WE_INVLDDATA:
            return "TGA_WE_INVLDDATA"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": The data is of a null pointer."
#endif
            ;
        case TGA_WE_INVLDID:
            return "TGA_WE_INVLDID"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": The ID is of a null pointer."
#endif
            ;
        case TGA_WE_INVLDSIGNATURE:
            return "TGA_WE_INVLDSIGNATURE"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": The signature is of a null pointer or does not equal \"" TGA_FOOTERSIG "\"."
#endif
            ;
        default:
            return "UNKNOWN"
#ifdef TGA_INCLUDE_ERROR_STRINGS
                ": Invalid error code."
#endif
            ;
    }
}
#endif
