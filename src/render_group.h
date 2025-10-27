#ifndef RENDER_GROUP_H
#define RENDER_GROUP_H

/*
 * NOTE:
 * 1) Everywhere outside the renderer, Y always goes upward, X to the right
 * 2) All bitmaps including the render target are assumes to be bottom-up
 *      meaning the first row pointer points to the bottom-most row when
 *      viewed on the screen.
 * 3) Unless otherwise specified all inputs to the renderer wer in world
 *      coordinate ("meters"), NOT pixels. Anything in pixel values will
 *      be explicitly marked as such.
 * 4) Z is a special coordinate because it is broken up into discretre slices,
 *      and the renderer actually understand these slices. Z slices are what
 *      control the _scaling_ of things, whereas Z offsets inside a slice affect
 *      Y offets.
 * 5) All color values specified to the render in Vec4 are in NON-premultiplied alpha.
 *
 * TODO: ZHANDLING
 */
struct Loaded_Bitmap
{
  Vec2 align_percentage;
  F32 width_over_height;

  S32 width;
  S32 height;
  S32 pitch_in_bytes;
  void* memory;
};

struct Environment_Map
{

  Loaded_Bitmap lod[4];
  F32 pos_z;
};

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
  Vec3 offset;
  Vec2 dim;

  Vec4 color;
};

struct Render_Entry_Coordinate_System
{
  Loaded_Bitmap* texture;
  Loaded_Bitmap* normal_map;
  Environment_Map* top_env_map;
  Environment_Map* middle_env_map;
  Environment_Map* bottom_env_map;
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

struct Render_Group_Camera
{
  // NOTE: Camera parameters
  F32 focal_length;
  F32 distance_above_target;
};

struct Render_Group

{
  Render_Group_Camera game_camera;
  Render_Group_Camera render_camera;
  F32 meters_to_pixels; // meters on the monitor to pixels on the monitor
  Vec2 monitor_half_dim_meters;

  Render_Basis* default_basis;

  F32 global_alpha;

  U32 max_push_buffer_size;
  U32 push_buffer_size;
  U8* push_buffer_base;
};

#endif // RENDER_GROUP_H
