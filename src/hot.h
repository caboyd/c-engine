#ifndef HOT_H
#define HOT_H
#include "cengine.h"

#include "render_group.h"

void Change_Saturation(Loaded_Bitmap* buffer, F32 saturation);

void Draw_BMP_Subset_Hot(Loaded_Bitmap* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, S32 bmp_x_dim, S32 bmp_y_dim,
                         F32 scale, B32 alpha_blend, F32 c_alpha);

void Draw_Rectf_Hot(Loaded_Bitmap* buffer, F32 fmin_x, F32 fmin_y, F32 fmax_x, F32 fmax_y, Vec4 color);

void Draw_Rect_Slowly_Hot(Loaded_Bitmap* buffer, Vec2 origin, Vec2 x_axis, Vec2 y_axis, Vec4 color,
                          Loaded_Bitmap* texture, Loaded_Bitmap* normal_map, Environment_Map* top_env_map,
                          Environment_Map* middle_env_map, Environment_Map* bottom_env_map, F32 pixels_to_meters);

void Draw_Rect_Quickly_Hot(Loaded_Bitmap* buffer, Vec2 origin, Vec2 x_axis, Vec2 y_axis, Vec4 color,
                           Loaded_Bitmap* texture, F32 pixels_to_meters);
#endif // HOT_H
