/*
The MIT License (MIT)

Copyright (c) 2016 Dmitry Alimov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

 */

#ifndef OCTREE_H
#define OCTREE_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <stdext/cmacros.h>

// Port of https://github.com/delimitry/octree_color_quantizer to C

// TODO: Fully port this to C at the concept level (it just recreates the python logic at this point)
// WARN: This API is not final! It may change DRASTICALLY in the future!

#ifdef OCTREE_COLOR_QUANTIZER_IS_SHARED
#define OCQ_EXPORT EXPORT
#else
#define OCQ_EXPORT
#endif

typedef struct OCQColor OCQColor_t;
typedef struct OCQColorArray OCQColorArray_t;
typedef struct OCQOctreeNode OCQOctreeNode_t;
typedef struct OCQOctreeNodeTable OCQOctreeNodeTable_t;
typedef struct OCQOctreeNodeArray OCQOctreeNodeArray_t;
typedef struct OCQOctreeQuantizer OCQOctreeQuantizer_t;

// OCQColor class
struct OCQColor {
    int32_t red;
    int32_t green;
    int32_t blue;
    int32_t alpha;
};

struct OCQColorArray {
    OCQColor_t **arr;
};

// Octree Node class for color quantization
struct OCQOctreeNode {
    OCQColor_t *color;
    int32_t pixel_count;
    size_t palette_index;
    OCQOctreeNode_t *children[16];
};

struct OCQOctreeNodeTable {
    struct {
        OCQOctreeNode_t *key;
        OCQOctreeNode_t *value;
    } *table;
};

struct OCQOctreeNodeArray {
    OCQOctreeNode_t **arr;
};

// Octree Quantizer class for image color quantization
// Use MAX_DEPTH to limit a number of levels
struct OCQOctreeQuantizer {
    OCQColor_t *tmp_color;
    OCQOctreeNodeTable_t *free_table;
    OCQOctreeNodeArray_t *levels[OCQ_MAX_DEPTH];
    // Root must always be last member/initialized
    OCQOctreeNode_t *root;
};

// Init Octree Quantizer
OCQ_EXPORT OCQOctreeQuantizer_t *OCQOctreeQuantizer___init__(void);

// Free Octree Quantizer
OCQ_EXPORT void OCQOctreeQuantizer_free(OCQOctreeQuantizer_t *self);

// Add `color` to the Octree (in raw uint32_t form)
OCQ_EXPORT void OCQOctreeQuantizer_add_color_raw(OCQOctreeQuantizer_t *self, uint32_t color);

// Make color palette with `color_count` colors maximum (in raw uint32_t form)
OCQ_EXPORT void OCQOctreeQuantizer_make_palette_raw(OCQOctreeQuantizer_t *self, size_t color_count, uint32_t *palette,
size_t *out_size);

// Get palette index for `color` (in raw size_t form)
OCQ_EXPORT size_t OCQOctreeQuantizer_get_palette_index_raw(OCQOctreeQuantizer_t *self, uint32_t color);
#endif
