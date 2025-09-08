#ifndef HOT_H
#define HOT_H
#include "cengine.h"

void Draw_BMP_Subset_Hot(Game_Offscreen_Buffer* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, S32 bmp_x_offset,
                         S32 bmp_y_offset, S32 bmp_x_dim, S32 bmp_y_dim, F32 scale, B32 alpha_blend);

void Draw_Rectf_Hot(Game_Offscreen_Buffer* buffer, F32 fmin_x, F32 fmin_y, F32 fmax_x, F32 fmax_y, F32 r, F32 g, F32 b,
                    B32 wire_frame = false);
#endif
