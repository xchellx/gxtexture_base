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

#include <octree_color_quantizer.h>

#include <stdlib.h>
#include <limits.h>

#include <stdext/cmacros.h>
#include <stdext/cmath.h>
#include <stb_ds.h>

// TODO: Fully port this to C at the concept level (it just recreates the python logic at this point)

static OCQColorArray_t *OCQColorArray___init__(void) {
    OCQColorArray_t *self = malloc(sizeof(OCQColorArray_t));
    self->arr = NULL;
    return self;
}

static void OCQColorArray_free(OCQColorArray_t *self) {
    if (!self)
        return;
    
    arrfree(self->arr);
    free(self->arr);
    free(self);
}

static OCQOctreeNodeTable_t *OCQOctreeNodeTable___init__(void) {
    OCQOctreeNodeTable_t *self = malloc(sizeof(OCQOctreeNodeTable_t));
    self->table = NULL;
    return self;
}

static void OCQOctreeNodeTable_free(OCQOctreeNodeTable_t *self) {
    if (!self)
        return;
    
    hmfree(self->table);
    free(self->table);
    free(self);
}

static OCQOctreeNodeArray_t *OCQOctreeNodeArray___init__(void) {
    OCQOctreeNodeArray_t *self = malloc(sizeof(OCQOctreeNodeArray_t));
    self->arr = NULL;
    return self;
}

static void OCQOctreeNodeArray_free(OCQOctreeNodeArray_t *self) {
    if (!self)
        return;
    
    arrfree(self->arr);
    free(self->arr);
    free(self);
}

// Get index of `color` for next `level`
static int32_t get_color_index_for_level(OCQColor_t *color, int32_t level) {
    int32_t index = 0;
    int32_t mask = 0x80 >> level;
    if (color->alpha & mask)
        index |= 8;
    if (color->red & mask)
        index |= 4;
    if (color->green & mask)
        index |= 2;
    if (color->blue & mask)
        index |= 1;
    return index;
}

// Initialize color
static OCQColor_t *OCQColor___init__(int32_t red, int32_t green, int32_t blue, int32_t alpha) {
    OCQColor_t *self = malloc(sizeof(OCQColor_t));
    self->red = red;
    self->green = green;
    self->blue = blue;
    self->alpha = alpha;
    return self;
}

// Free color
static void OCQColor_free(OCQColor_t *self) {
    free(self);
}


void OCQOctreeQuantizer_add_level_node(OCQOctreeQuantizer_t *self, int32_t level, OCQOctreeNode_t *node);
// Init new Octree Node
static OCQOctreeNode_t *OCQOctreeNode___init__(int32_t level, OCQOctreeQuantizer_t *owner) {
    if (level < 0 || level > OCQ_MAX_DEPTH || !owner)
        return NULL;
    
    OCQOctreeNode_t *self = malloc(sizeof(OCQOctreeNode_t));
    self->color = OCQColor___init__(0, 0, 0, 0);
    self->pixel_count = 0;
    self->palette_index = 0;
    for (int32_t i = 0; i < 16; i++)
        self->children[i] = NULL;
    hmput(owner->free_table->table, self, self);
    // add node to current level
    if (level < OCQ_MAX_DEPTH - 1)
        OCQOctreeQuantizer_add_level_node(owner, level, self);
    return self;
}

// Free Octree Node
static void OCQOctreeNode_free(OCQOctreeNode_t *self, bool recursive, OCQOctreeQuantizer_t *owner) {
    if (!self || !owner || hmgeti(owner->free_table->table, self) == -1)
        return;
    
    OCQOctreeNodeArray_t *stack = OCQOctreeNodeArray___init__();
    arrpush(stack->arr, self);
    while (arrlenu(stack->arr)) {
        OCQOctreeNode_t *node = arrpop(stack->arr);
        
        if (recursive) {
            for (int32_t i = 0; i < 16; i++) {
                OCQOctreeNode_t *child = node->children[i];
                if (child && hmgeti(owner->free_table->table, child) != -1)
                    arrpush(stack->arr, child);
            }
        }
        
        hmdel(owner->free_table->table, node);
        OCQColor_free(node->color);
        free(node);
    }
    OCQOctreeNodeArray_free(stack);
}

// Check that node is leaf
static bool OCQOctreeNode_is_leaf(OCQOctreeNode_t *self, OCQOctreeQuantizer_t *owner) {
    if (!self || !owner || hmgeti(owner->free_table->table, self) == -1)
        return false;
    
    return self->pixel_count > 0;
}

// Get all leaf nodes
static OCQOctreeNodeArray_t *OCQOctreeNode_get_leaf_nodes(OCQOctreeNode_t *self, OCQOctreeQuantizer_t *owner) {
    if (!self || !owner || hmgeti(owner->free_table->table, self) == -1)
        return NULL;
    
    OCQOctreeNodeArray_t *leaf_nodes = OCQOctreeNodeArray___init__();
    
    OCQOctreeNodeArray_t *stack = OCQOctreeNodeArray___init__();
    arrpush(stack->arr, self);
    while (arrlenu(stack->arr)) {
        OCQOctreeNode_t *node = arrpop(stack->arr);
        
        if (OCQOctreeNode_is_leaf(node, owner))
            arrput(leaf_nodes->arr, node);
        else {
            for (int32_t i = 16; i >= 0; i--) {
                OCQOctreeNode_t *child = node->children[i];
                if (child && hmgeti(owner->free_table->table, child) != -1)
                    arrpush(stack->arr, child);
            }
        }
    }
    OCQOctreeNodeArray_free(stack);
    
    return leaf_nodes;
}

// Get a sum of pixel count for node and its children
static USED int32_t OCQOctreeNode_get_nodes_pixel_count(OCQOctreeNode_t *self, OCQOctreeQuantizer_t *owner) {
    if (!self || !owner || hmgeti(owner->free_table->table, self) == -1)
        return 0;
    
    int32_t sum_count = self->pixel_count;
    for(int32_t i = 0; i < 16; i++) {
        OCQOctreeNode_t *child = self->children[i];
        if (child && hmgeti(owner->free_table->table, child) != -1)
            sum_count += child->pixel_count;
    }
    return sum_count;
}

// Add `color` to the tree
static void OCQOctreeNode_add_color(OCQOctreeNode_t *self, OCQColor_t *color, int32_t level,
OCQOctreeQuantizer_t *owner) {
    if (!self || !color || !owner || hmgeti(owner->free_table->table, self) == -1)
        return;
    
    OCQOctreeNode_t *node = self;
    while (level < OCQ_MAX_DEPTH) {
        int32_t index = get_color_index_for_level(color, level);
        if (!node->children[index] || hmgeti(owner->free_table->table, node->children[index]) == -1)
            node->children[index] = OCQOctreeNode___init__(level, owner);
        node = node->children[index];
        level++;
    }
    
    node->color->red += color->red;
    node->color->green += color->green;
    node->color->blue += color->blue;
    node->color->alpha += color->alpha;
    node->pixel_count++;
}

// Get palette index for `color`
// Uses `level` to go one level deeper if the node is not a leaf
static size_t OCQOctreeNode_get_palette_index(OCQOctreeNode_t *self, OCQColor_t *color, int32_t level,
OCQOctreeQuantizer_t *owner) {
    if (!self ||!color || level < 0 || level > OCQ_MAX_DEPTH || !owner || hmgeti(owner->free_table->table, self) == -1)
        return 0;
    
    size_t palette_index = SIZE_MAX;
    
    OCQOctreeNodeArray_t *stack = OCQOctreeNodeArray___init__();
    arrpush(stack->arr, self);
    while (arrlenu(stack->arr)) {
        OCQOctreeNode_t *node = arrpop(stack->arr);
        if (OCQOctreeNode_is_leaf(node, owner)) {
            palette_index = node->palette_index;
            break;
        }
        
        OCQOctreeNode_t *newNode = NULL;
        int32_t index = get_color_index_for_level(color, level);
        if (node->children[index] && hmgeti(owner->free_table->table, node->children[index]) != -1)
            newNode = node->children[index];
        else
            // get palette index for a first found child node
            for(int32_t i = 0; i < 16; i++)
                if (node->children[i] && hmgeti(owner->free_table->table, node->children[i]) != -1)
                    newNode = node->children[i];
        
        if (newNode) {
            arrpush(stack->arr, newNode);
            level++;
        }
    }
    OCQOctreeNodeArray_free(stack);
    
    return palette_index;
}

// Add all children pixels count and color channels to parent node 
// Return the number of removed leaves
static int32_t OCQOctreeNode_remove_leaves(OCQOctreeNode_t *self, OCQOctreeQuantizer_t *owner) {
    if (!self || !owner || hmgeti(owner->free_table->table, self) == -1)
        return 0;
    
    int32_t result = 0;
    for(int32_t i = 0; i < 16; i++) {
        OCQOctreeNode_t *node = self->children[i];
        if (node && hmgeti(owner->free_table->table, node) != -1) {
            self->color->red += node->color->red;
            self->color->green += node->color->green;
            self->color->blue += node->color->blue;
            self->color->alpha += node->color->alpha;
            self->pixel_count += node->pixel_count;
            result++;
        }
    }
    return result - 1;
}

// Get average color
static OCQColor_t *OCQOctreeNode_get_color(OCQOctreeNode_t *self, OCQOctreeQuantizer_t *owner) {
    if (!self || !owner || hmgeti(owner->free_table->table, self) == -1)
        return NULL;
    
    return OCQColor___init__(
        self->color->red / self->pixel_count,
        self->color->green / self->pixel_count,
        self->color->blue / self->pixel_count,
        self->color->alpha / self->pixel_count
    );
}

// Init Octree Quantizer
OCQOctreeQuantizer_t *OCQOctreeQuantizer___init__(void) {
    OCQOctreeQuantizer_t *self = malloc(sizeof(OCQOctreeQuantizer_t));
    self->tmp_color = OCQColor___init__(0, 0, 0, 0);
    self->free_table = OCQOctreeNodeTable___init__();
    for (int32_t i = 0; i < OCQ_MAX_DEPTH; i++)
        self->levels[i] = OCQOctreeNodeArray___init__();
    // Root must always be last member/initialized
    self->root = OCQOctreeNode___init__(0, self);
    return self;
}

// Free Octree Quantizer
void OCQOctreeQuantizer_free(OCQOctreeQuantizer_t *self) {
    if (!self)
        return;
    
    OCQColor_free(self->tmp_color);
    
    for (int32_t i = 0; i < OCQ_MAX_DEPTH; i++) {
        OCQOctreeNodeArray_free(self->levels[i]);
        self->levels[i] = NULL;
    }
    
    OCQOctreeNodeArray_t *free_queue = OCQOctreeNodeArray___init__();
    for (size_t i = 0; i < hmlenu(self->free_table->table); i++)
        arrput(free_queue->arr, self->free_table->table[i].key);
    
    for (size_t i = 0; i < arrlenu(free_queue->arr); i++)
        OCQOctreeNode_free(free_queue->arr[i], false, self);
    OCQOctreeNodeArray_free(free_queue);
    
    OCQOctreeNodeTable_free(self->free_table);
    
    free(self);
}

// Get all leaves
static OCQOctreeNodeArray_t *OCQOctreeQuantizer_get_leaves(OCQOctreeQuantizer_t *self) {
    if (!self)
        return NULL;
    
    return OCQOctreeNode_get_leaf_nodes(self->root, self);
}

// Add `node` to the nodes at `level`
void OCQOctreeQuantizer_add_level_node(OCQOctreeQuantizer_t *self, int32_t level, OCQOctreeNode_t *node) {
    if (!self || level < 0 || level > OCQ_MAX_DEPTH || !node || hmgeti(self->free_table->table, node) == -1)
        return;
    
    if (self->levels[level])
        arrput(self->levels[level]->arr, node);
}

// Add `color` to the Octree
static void OCQOctreeQuantizer_add_color(OCQOctreeQuantizer_t *self, OCQColor_t *color) {
    if (!self || !color)
        return;
    
    // passes self value as `parent` to save nodes to levels dict
    OCQOctreeNode_add_color(self->root, color, 0, self);
}

// Add `color` to the Octree (in raw uint32_t form)
void OCQOctreeQuantizer_add_color_raw(OCQOctreeQuantizer_t *self, uint32_t color) {
    if (!self)
        return;
    
    self->tmp_color->red = (color >> OCQ_COMP_SH_R) & 0xFF;
    self->tmp_color->green = (color >> OCQ_COMP_SH_G) & 0xFF;
    self->tmp_color->blue = (color >> OCQ_COMP_SH_B) & 0xFF;
    self->tmp_color->alpha = (color >> OCQ_COMP_SH_A) & 0xFF;
    
    OCQOctreeQuantizer_add_color(self, self->tmp_color);
}

// Make color palette with `color_count` colors maximum
static OCQColorArray_t *OCQOctreeQuantizer_make_palette(OCQOctreeQuantizer_t *self, size_t color_count) {
    if (!self || !color_count || color_count > 65536)
        return NULL;
    
    OCQOctreeNodeArray_t *leaf_nodes = OCQOctreeQuantizer_get_leaves(self);
    size_t leaf_count = arrlenu(leaf_nodes->arr);
    OCQOctreeNodeArray_free(leaf_nodes);
    
    // reduce nodes
    // up to 16 leaves can be reduced here and the palette will have
    // only 248 colors (in worst case) instead of expected 256 colors
    for (int32_t level = OCQ_MAX_DEPTH - 1; level >= 0; level--) {
        if (self->levels[level] && self->levels[level]->arr) {
            for (size_t i = 0; i < arrlenu(self->levels[level]->arr); i++) {
                OCQOctreeNode_t *node = self->levels[level]->arr[i];
                leaf_count -= OCQOctreeNode_remove_leaves(node, self);
                if (leaf_count <= color_count)
                    break;
            }
            if (leaf_count <= color_count)
                break;
            OCQOctreeNodeArray_free(self->levels[level]);
            self->levels[level] = NULL;
        }
    }
    
    // build palette
    OCQColorArray_t *palette = OCQColorArray___init__();
    leaf_nodes = OCQOctreeQuantizer_get_leaves(self);
    size_t palette_index = 0;
    for (size_t i = 0; i < arrlenu(leaf_nodes->arr); i++) {
        OCQOctreeNode_t *node = leaf_nodes->arr[i];
        if (palette_index >= color_count)
            break;
        if (OCQOctreeNode_is_leaf(node, self))
            arrput(palette->arr, OCQOctreeNode_get_color(node, self));
        node->palette_index = palette_index++;
    }
    OCQOctreeNodeArray_free(leaf_nodes);
    return palette;
}

// Make color palette with `color_count` colors maximum (in raw uint32_t form)
void OCQOctreeQuantizer_make_palette_raw(OCQOctreeQuantizer_t *self, size_t color_count, uint32_t *palette,
size_t *out_size) {
    if (!self || !color_count || color_count > 65536 || !palette || !out_size)
        return;
    
    *out_size = 0;
    
    OCQColorArray_t *ocq_palette = OCQOctreeQuantizer_make_palette(self, color_count);
    size_t ocq_palette_size = arrlenu(ocq_palette->arr);
    *out_size = ocq_palette_size;
    for (size_t i = 0; i < ocq_palette_size; i++) {
        OCQColor_t *color = ocq_palette->arr[i];
        palette[i] = (
              (((uint32_t) Math_ClampS32(color->red, 0, 255))   << OCQ_COMP_SH_R)
            | (((uint32_t) Math_ClampS32(color->green, 0, 255)) << OCQ_COMP_SH_G)
            | (((uint32_t) Math_ClampS32(color->blue, 0, 255))  << OCQ_COMP_SH_B)
            | (((uint32_t) Math_ClampS32(color->alpha, 0, 255)) << OCQ_COMP_SH_A)
        );
        OCQColor_free(color);
    }
    OCQColorArray_free(ocq_palette);
}

// Get palette index for `color`
static size_t OCQOctreeQuantizer_get_palette_index(OCQOctreeQuantizer_t *self, OCQColor_t *color) {
    if (!self || !color)
        return 0;
    
    return OCQOctreeNode_get_palette_index(self->root, color, 0, self);
}

// Get palette index for `color` (in raw size_t form)
size_t OCQOctreeQuantizer_get_palette_index_raw(OCQOctreeQuantizer_t *self, uint32_t color) {
    if (!self || !color)
        return 0;
    
    self->tmp_color->red = (color >> OCQ_COMP_SH_R) & 0xFF;
    self->tmp_color->green = (color >> OCQ_COMP_SH_G) & 0xFF;
    self->tmp_color->blue = (color >> OCQ_COMP_SH_B) & 0xFF;
    self->tmp_color->alpha = (color >> OCQ_COMP_SH_A) & 0xFF;
    
    return OCQOctreeQuantizer_get_palette_index(self, self->tmp_color);
}
