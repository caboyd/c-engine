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
  E_RENDER_GROUP_ENTRY_Render_Entry_Bitmap,
};

// NOTE: aligned as 8 to not crash when used in push buffer
struct __attribute__((aligned(8))) Render_Group_Entry_Header
{
  E_Render_Group_Entry_Type type;
};

struct Render_Entry_Clear
{
  Render_Group_Entry_Header header;
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
struct Render_Entry_Bitmap
{
  Render_Group_Entry_Header header;
  Render_Group_Entry_Base base;
  Loaded_Bitmap* bitmap;
  S32 bmp_offset_x;
  S32 bmp_offset_y;
};
struct Render_Entry_Rectangle
{
  Render_Group_Entry_Header header;
  Render_Group_Entry_Base base;
  B32 wire_frame;
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
