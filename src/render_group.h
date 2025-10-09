#ifndef RENDER_GROUP_H
#define RENDER_GROUP_H

struct Render_Basis
{
  Vec3 pos;
};

enum E_Render_Group_Entry_Type
{
  E_RENDER_GROUP_ENTRY_Render_Entry_Clear,
  E_RENDER_GROUP_ENTRY_Render_Entry_Rectangle,
  E_RENDER_GROUP_ENTRY_Render_Entry_Rectangle_Outline,
  E_RENDER_GROUP_ENTRY_Render_Entry_Bitmap,
  E_RENDER_GROUP_ENTRY_Render_Entry_Coordinate_System,
};

// NOTE: aligned as 8 to not crash when used in push buffer
struct __attribute__((aligned(8))) Render_Group_Entry_Header
{
  E_Render_Group_Entry_Type type;
};

struct Render_Entry_Clear
{
  Vec4 color;
};

struct Render_Group_Entry_Base
{
  Render_Basis* basis;
  Vec2 offset;
  F32 offset_z;
  Vec2 dim;
  F32 entity_cz;
  F32 scale;
  Vec4 color;
};

struct Render_Entry_Coordinate_System
{
  Loaded_Bitmap* texture;
  Vec2 origin;
  Vec2 x_axis;
  Vec2 y_axis;
  Vec4 color;

  Vec2 points[16];
};

struct Render_Entry_Bitmap
{
  Render_Group_Entry_Base base;
  Loaded_Bitmap* bitmap;
  S32 bmp_offset_x;
  S32 bmp_offset_y;
};

struct Render_Entry_Rectangle
{
  Render_Group_Entry_Base base;
};

struct Render_Entry_Rectangle_Outline
{
  Render_Group_Entry_Base base;
  F32 outline_pixel_thickness;
};

struct Render_Group

{
  Render_Basis* default_basis;
  F32 meters_to_pixels;
  F32 sprite_scale;

  U32 max_push_buffer_size;
  U32 push_buffer_size;
  U8* push_buffer_base;
};

#endif // RENDER_GROUP_H
