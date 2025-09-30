#ifndef RENDER_GROUP_H
#define RENDER_GROUP_H

struct Render_Basis
{
  Vec3 pos;
};

struct Entity_Render_Piece
{
  Render_Basis* basis;
  Loaded_Bitmap* bitmap;
  S32 bmp_offset_x;
  S32 bmp_offset_y;
  Vec2 dim;

  Vec2 offset;
  F32 offset_z;
  F32 entity_cz;
  F32 scale;
  B32 wire_frame;

  Vec4 color;
};
struct Render_Group

{
  Render_Basis* default_basis;
  F32 meters_to_pixels;
  F32 sprite_scale;
  U32 count;

  U32 max_push_buffer_size;
  U32 push_buffer_size;
  U8* push_buffer_base;
};

#endif // RENDER_GROUP_H
