#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdbool.h>

#include <shader-works/maths.h>

bool load_bmp_texture(const char *filename, u32 *out_width, u32 *out_height, u32 **out_data);

#endif // __UTIL_H__
