#ifndef HOT_H
#define HOT_H
#include "cengine.h"

void Draw_BMP_Subset_Hot(Loaded_Bitmap* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, S32 bmp_x_offset, S32 bmp_y_offset,
                         S32 bmp_x_dim, S32 bmp_y_dim, F32 scale, B32 alpha_blend, F32 c_alpha);

void Draw_Rectf_Hot(Loaded_Bitmap* buffer, F32 fmin_x, F32 fmin_y, F32 fmax_x, F32 fmax_y, Vec4 color);
#endif
