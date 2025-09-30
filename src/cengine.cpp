
#undef CLANGD
#include "cengine.h"
#include "hot.h"
#include "render_group.h"
#include "render_group.cpp"
#include "world.cpp"
#include "sim_region.cpp"
#include "entity.cpp"

internal void Game_Output_Sound(Game_State* game_state, Game_Output_Sound_Buffer* sound_buffer)
{

  // TODO: fix: apply smoothing (interpolation) to volume. Example (linear ramp per sample)
  // Alternative: update volume gradually over N samples instead of instantly.
  F32 volume = game_state->volume;
  F32 frequency = 261;
  volume *= volume;

  if (sound_buffer->sample_count == 0)
  {
    return;
  }

  S32 channels = sound_buffer->channel_count;
  S32 sample_format_bytes = sound_buffer->bytes_per_sample;

  S32 bits_per_sample = sample_format_bytes * 8;

  F64 phase_increment = 2.0 * M_PI * frequency / sound_buffer->samples_per_second;

  for (S32 frame = 0; frame < sound_buffer->sample_count; ++frame)
  {
    F32 sample_value = (F32)Sin(game_state->sine_phase);

    sample_value *= volume;

    game_state->sine_phase += phase_increment;
    if (game_state->sine_phase > 2.0 * M_PI)
      game_state->sine_phase -= 2.0 * M_PI;

    U8* frame_ptr = sound_buffer->sample_buffer + (frame * channels * sample_format_bytes);

    for (S32 ch = 0; ch < channels; ++ch)
    {
      U8* sample_ptr = frame_ptr + (ch * sample_format_bytes);
      if (bits_per_sample == 32)
      {
        *((F32*)(void*)sample_ptr) = sample_value;
      }
      else if (bits_per_sample == 16)
      {
        S16 sample = INT16_MAX * (S16)(sample_value);
        *((S16*)(void*)sample_ptr) = sample;
      }
      else if (bits_per_sample == 24)
      {
        S32 sample = (S32)(8388607 * sample_value);
        S8* temp_ptr = (S8*)sample_ptr;
        *((S8*)temp_ptr++) = sample & 0xFF;
        *((S8*)temp_ptr++) = (sample >> 8) & 0xFF;
        *((S8*)temp_ptr++) = (sample >> 16) & 0xFF;
      }
    }
  }
}

internal void Draw_BMP_Subset(Loaded_Bitmap* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, S32 bmp_x_offset,
                              S32 bmp_y_offset, S32 bmp_x_dim, S32 bmp_y_dim, F32 scale, B32 alpha_blend, F32 c_alpha)
{
#if 1
  Draw_BMP_Subset_Hot(buffer, bitmap, x, y, bmp_x_offset, bmp_y_offset, bmp_x_dim, bmp_y_dim, scale, alpha_blend,
                      c_alpha);
#else
  c_alpha = CLAMP(c_alpha, 0.f, 1.f);
  if (!bitmap || !bitmap->memory)
  {
    // TODO: Maybe draw pink checkerboard if no texture
    return;
  };
  ASSERT(scale > 0);

  S32 min_x = Round_F32_S32(x);
  S32 min_y = Round_F32_S32(y);
  S32 max_x = Round_F32_S32(x + (F32)bmp_x_dim * scale);
  S32 max_y = Round_F32_S32(y + (F32)bmp_y_dim * scale);
  S32 x_draw_offset = Round_F32_S32((F32)bmp_x_offset * scale);
  if (min_x < 0)
  {
    x_draw_offset += (-min_x) % Round_F32_S32(scale * (F32)bmp_x_dim);
  }
  S32 y_draw_offset = Round_F32_S32((F32)bmp_y_offset * scale);
  if (min_y < 0)
  {
    y_draw_offset += (-min_y) % Round_F32_S32(scale * (F32)bmp_y_dim);
  }
  min_x = CLAMP(min_x, 0, buffer->width);
  max_x = CLAMP(max_x, 0, buffer->width);

  min_y = CLAMP(min_y, 0, buffer->height);
  max_y = CLAMP(max_y, 0, buffer->height);

  U8* dest_row_in_bytes = (U8*)buffer->memory + (min_y * buffer->pitch_in_bytes) + (min_x * BITMAP_BYTES_PER_PIXEL);
  for (S32 y_index = min_y; y_index < max_y; y_index++)
  {
    U8* pixel = dest_row_in_bytes;
    S32 y_src_offset = Trunc_F32_S32((F32)(y_index - min_y + y_draw_offset) / scale);
    // NOTE: flip the bmp to render into buffer top to bottom
    S32 y_src = y_src_offset;
    // y_src = CLAMP(y_src, (bitmap->height - 1) - bmp_y_dim - bmp_y_offset, (bitmap->height - 1) - bmp_y_offset);
    y_src = CLAMP(y_src, bmp_y_offset, bmp_y_offset + bmp_y_dim - 1);
    ASSERT(y_src < bitmap->height);

    for (S32 x_index = min_x; x_index < max_x; x_index++)
    {
      S32 x_src = Trunc_F32_S32((F32)(x_index - min_x + x_draw_offset) / scale);
      x_src = CLAMP(x_src, bmp_x_offset, bmp_x_offset + bmp_x_dim - 1);
      ASSERT(x_src < bitmap->width);

      U8* src = (U8*)(void*)((U32*)bitmap->memory + y_src * (bitmap->pitch_in_bytes / BITMAP_BYTES_PER_PIXEL) + x_src);
      Color4 out = *(Color4*)(void*)src;

      if (alpha_blend)
      {
        out = blend_normal_Color4(*(Color4*)(void*)pixel, *(Color4*)(void*)src, c_alpha);
      }

      // Note: BMP may not be 4 byte aligned so assign byte by byte
      *pixel++ = out.argb.b; // B
      *pixel++ = out.argb.g; // G
      *pixel++ = out.argb.r; // R
      *pixel++ = out.argb.a; // A
    }
    dest_row_in_bytes += buffer->pitch_in_bytes;
  }

#endif
}
internal void Draw_Sprite_Sheet_Sprite(Loaded_Bitmap* buffer, Sprite_Sheet* sprite_sheet, U32 sprite_index, F32 x,
                                       F32 y, F32 scale, B32 alpha_blend = true, F32 c_alpha = 1.0f)
{
  Sprite sprite = sprite_sheet->sprites[sprite_index];

  Draw_BMP_Subset(buffer, &sprite_sheet->bitmap, x, y, sprite.x, sprite.y, sprite.width, sprite.height, scale,
                  alpha_blend, c_alpha);
}
// internal void Draw_Player_Sprite(Game_Offscreen_Buffer* buffer, Entity_Sprite* entity_sprite, F32 x, F32 y, F32
// scale,
//                                  B32 alpha_blend = false, F32 c_alpha = 1.0f)
// {
//   Sprite sprite = entity_sprite->sprite_sheet->sprites[entity_sprite->sprite_index];
//
//   Draw_BMP_Subset(buffer, &entity_sprite->sprite_sheet->bitmap, x + (scale * entity_sprite->offset.x),
//
//                   y + (scale * entity_sprite->offset.y), sprite.x, sprite.y, sprite.width, sprite.height, scale,
//                   alpha_blend, c_alpha);
// }

internal void Draw_BMP(Loaded_Bitmap* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, F32 scale = 1.f,
                       B32 alpha_blend = true, F32 c_alpha = 1.0f)
{
  Draw_BMP_Subset(buffer, bitmap, x, y, 0, 0, bitmap->width, bitmap->height, scale, alpha_blend, c_alpha);
}

// internal void Draw_BMP_Align(Game_Offscreen_Buffer* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, F32 align_x,
//
//                              F32 align_y, F32 scale, B32 alpha_blend = false, F32 c_alpha = 1.0f)
// {
//   Draw_BMP_Subset(buffer, bitmap, x + (scale * align_x), y + (scale * align_y), 0, 0, bitmap->width, bitmap->height,
//                   scale, alpha_blend, c_alpha);
// }
// internal void Draw_BMP_Centered(Game_Offscreen_Buffer* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, F32 scale,
//                                 B32 alpha_blend = false, F32 c_alpha = 1.0)
// {
//   ASSERT(bitmap);
//   Draw_BMP_Subset(buffer, bitmap, x - (scale * ((F32)bitmap->width / 2.0f)), y - (scale * ((F32)bitmap->height
//   / 2.0f)),
//                   0, 0, bitmap->width, bitmap->height, scale, alpha_blend, c_alpha);
// }

internal void Draw_Rectf(Loaded_Bitmap* buffer, F32 fmin_x, F32 fmin_y, F32 fmax_x, F32 fmax_y, F32 r, F32 g, F32 b)
{
#if 1
  Draw_Rectf_Hot(buffer, fmin_x, fmin_y, fmax_x, fmax_y, r, g, b);
#else
  fmin_x = CLAMP(fmin_x, 0, (F32)buffer->width);
  fmin_y = CLAMP(fmin_y, 0, (F32)buffer->width);
  fmax_x = CLAMP(fmax_x, 0, (F32)buffer->width);
  fmax_y = CLAMP(fmax_y, 0, (F32)buffer->width);

  S32 min_x = Round_F32_S32(fmin_x);
  S32 min_y = Round_F32_S32(fmin_y);
  S32 max_x = Round_F32_S32(fmax_x);
  S32 max_y = Round_F32_S32(fmax_y);

  min_x = CLAMP(min_x, 0, buffer->width);
  max_x = CLAMP(max_x, 0, buffer->width);

  min_y = CLAMP(min_y, 0, buffer->height);
  max_y = CLAMP(max_y, 0, buffer->height);

  U32 color = (255u << 24) | (F32_to_U32_255(r) << 16) | (F32_to_U32_255(g) << 8) | (F32_to_U32_255(b) << 0);

  U8* row_in_bytes = (U8*)buffer->memory + (min_y * buffer->pitch_in_bytes) + (min_x * BITMAP_BYTES_PER_PIXEL);

  for (S32 y = min_y; y < max_y; y++)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;
    for (S32 x = min_x; x < max_x; x++)
    {
      *pixel++ = color;
    }
    row_in_bytes += buffer->pitch_in_bytes;
  }
#endif
}
internal void Draw_Rect(Loaded_Bitmap* buffer, Rect2 rect, Vec3 color)

{
  Draw_Rectf(buffer, rect.min.x, rect.min.y, rect.max.x, rect.max.y, color.r, color.g, color.b);
}

internal void Draw_Rect_Outline(Loaded_Bitmap* buffer, Rect2 rect, Vec3 color, F32 pixel_thickness = 1.f)
{
  // HACK: to fix float rounding when straddling 0.5px boundary
  F32 epsilon = 0.0001f;
  rect.min += vec2(epsilon);
  rect.max += vec2(epsilon);
  F32 t = 0.5f * pixel_thickness;
  // TOP/BOTTOM
  Draw_Rectf(buffer, rect.min.x - t, rect.min.y - t, rect.max.x + t, rect.min.y + t, color.r, color.g, color.b);
  Draw_Rectf(buffer, rect.min.x - t, rect.max.y - t, rect.max.x + t, rect.max.y + t, color.r, color.g, color.b);

  // LEFT/RIGHT
  Draw_Rectf(buffer, rect.min.x - t, rect.min.y - t, rect.min.x + t, rect.max.y + t, color.r, color.g, color.b);
  Draw_Rectf(buffer, rect.max.x - t, rect.min.y - t, rect.max.x + t, rect.max.y + t, color.r, color.g, color.b);
}

internal void Render_Weird_Gradient(Game_Offscreen_Buffer* buffer, S32 blue_offset, S32 green_offset)
{

  U32* row = (U32*)buffer->memory;
  for (int y = 0; y < buffer->height; ++y)
  {
    U32* pixel = row;
    for (int x = 0; x < buffer->width; ++x)
    {
      // NOTE: Color      0x  AA RR GG BB
      //       Steel blue 0x  00 46 82 B4
      U32 blue = (U32)(x + blue_offset) % 255;
      U32 green = (U32)(y + green_offset) % 255;

      *pixel++ = (green << 8 | blue << 8) | (blue | green);
    }
    // NOTE:because row is U32 we move 4 bytes * width
    row += buffer->width;
  }
}
internal void Draw_Inputs(Loaded_Bitmap* buffer, Game_Input* input)
{

  for (S32 i = 0; i < (S32)Array_Count(input->mouse_buttons); i++)
  {
    if (input->mouse_buttons[i].ended_down)
    {
      F32 offset = (F32)i * 20.f;
      Draw_Rectf(buffer, 10.f + offset, 10.f, 20.f + offset, 20.f, 1.f, 0.f, 0.f);
    }
  }
  if (input->mouse_z > 0)
  {
    Draw_Rectf(buffer, 10.f, 30.f, 20.f, 40.f, 1.f, 0.f, 0.f);
  }
  else if (input->mouse_z < 0)
  {
    Draw_Rectf(buffer, 30.f, 30.f, 40.f, 40.f, 1.f, 0.f, 0.f);
  }

  Vec2 mouse = vec2((F32)input->mouse_x, (F32)input->mouse_y);

  Rect2 r = Rect_Min_Dim(mouse, vec2(4.f, 4.f));
  Draw_Rect_Outline(buffer, r, vec3(1, 1, 0));
}

internal Loaded_Bitmap DEBUG_Load_BMP(Thread_Context* thread, Debug_Platform_Read_Entire_File_Func* Read_Entire_File,
                                      Debug_Platform_Free_File_Memory_Func* Free_File_Memory, Arena* arena,
                                      char* file_name)
{

  Loaded_Bitmap result = {};

  // NOTE: byte order in memory is AA RR GG BB
  Debug_Read_File_Result read_result = Read_Entire_File(thread, file_name);
  if (read_result.contents_size != 0)
  {
    void* contents = read_result.contents;

    Bitmap_Header* header = (Bitmap_Header*)contents;
    U32* pixels = (U32*)(void*)((U8*)contents + header->OffBits);
    result.width = header->Width;
    result.height = header->Height;

    ASSERT(header->Compression == 3);

    // NOTE: memory copy to dest first because pixels may not be 4 byte aligned
    // and may cause a crash if read as 4 bytes.
    S32 pixels_count = result.width * result.height;
    result.memory = Push_Array(arena, (U64)pixels_count, U32);

    Memory_Copy(result.memory, pixels, (pixels_count * (S32)sizeof(U32)));

    // NOTE: Shift down to bottom bits and shift up to match
    //   BB GG RR AA
    //   low bits to high bits

    Bit_Scan_Result blue_scan = Find_Least_Significant_Set_Bit(header->BlueMask);
    Bit_Scan_Result red_scan = Find_Least_Significant_Set_Bit(header->RedMask);
    Bit_Scan_Result green_scan = Find_Least_Significant_Set_Bit(header->GreenMask);
    Bit_Scan_Result alpha_scan = Find_Least_Significant_Set_Bit(header->AlphaMask);

    ASSERT(blue_scan.found);
    ASSERT(red_scan.found);
    ASSERT(green_scan.found);
    ASSERT(alpha_scan.found);

    S32 blue_shift_down = (S32)blue_scan.index;
    S32 green_shift_down = (S32)green_scan.index;
    S32 red_shift_down = (S32)red_scan.index;
    S32 alpha_shift_down = (S32)alpha_scan.index;

    U32* p = (U32*)result.memory;
    for (S32 i = 0; i < pixels_count; ++i)
    {
      F32 b = (F32)((p[i] & header->BlueMask) >> blue_shift_down);
      F32 g = (F32)((p[i] & header->GreenMask) >> green_shift_down);
      F32 r = (F32)((p[i] & header->RedMask) >> red_shift_down);
      F32 a = (F32)((p[i] & header->AlphaMask) >> alpha_shift_down);
      F32 alpha = (a / 255.0f);

      b = b * alpha;
      g = g * alpha;
      r = r * alpha;

      p[i] = (((U32)(a + 0.5f) << 24) | ((U32)(r + 0.5f) << 16) | ((U32)(g + 0.5f) << 8) | ((U32)(b + 0.5f) << 0));
    }
    result.pitch_in_bytes = -result.width * BITMAP_BYTES_PER_PIXEL;
    result.memory = (U8*)result.memory - result.pitch_in_bytes * (result.height - 1);
    Free_File_Memory(thread, read_result.contents);
  }
  return result;
}
internal Sprite_Sheet DEBUG_Load_SpriteSheet_BMP(Thread_Context* thread,
                                                 Debug_Platform_Read_Entire_File_Func* Read_Entire_File,
                                                 Debug_Platform_Free_File_Memory_Func* Free_File_Memory, Arena* arena,
                                                 char* file_name, S32 sprite_width, S32 sprite_height)
{
  ASSERT(sprite_width > 0);
  ASSERT(sprite_height > 0);

  Sprite_Sheet result = {};
  result.bitmap = DEBUG_Load_BMP(thread, Read_Entire_File, Free_File_Memory, arena, file_name);

  result.sprite_height = sprite_height;
  result.sprite_width = sprite_width;

  ASSERT(result.bitmap.width % sprite_width == 0);
  ASSERT(result.bitmap.height % sprite_height == 0);

  result.sprite_count = (result.bitmap.width / sprite_width) * (result.bitmap.height / sprite_height);
  result.sprites = Push_Array(arena, (U64)result.sprite_count, Sprite);

  S32 sprite_index = 0;
  for (S32 y = 0; y < result.bitmap.height; y += sprite_height)
  {
    for (S32 x = 0; x < result.bitmap.width; x += sprite_width)
    {
      ASSERT(sprite_index < result.sprite_count);
      Sprite* sprite = &result.sprites[sprite_index++];
      sprite->x = x;
      sprite->y = y;
      sprite->width = sprite_width;
      sprite->height = sprite_height;
    }
  }
  ASSERT(sprite_index == result.sprite_count); // sanity check
  return result;
}

struct Add_Low_Entity_Result
{
  Low_Entity* low;
  U32 low_index;
};

internal Add_Low_Entity_Result Add_Low_Entity(Game_State* game_state, Entity_Type type, World_Position pos)
{
  ASSERT(game_state->low_entity_count < Array_Count(game_state->low_entities));
  U32 entity_index = game_state->low_entity_count++;
  Low_Entity* entity_low = game_state->low_entities + entity_index;

  *entity_low = {};
  entity_low->sim.type = type;
  entity_low->pos = Null_Position();
  entity_low->sim.collision = game_state->null_collision;

  Change_Entity_Location(&game_state->world_arena, game_state->world, entity_index, entity_low, pos);

  Add_Low_Entity_Result result;
  result.low = entity_low;
  result.low_index = entity_index;

  return result;
}
internal World_Position Chunk_Position_From_Tile_Position(World* world, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z,
                                                          Vec3 additional_offset = vec3(0))

{
  World_Position base_pos = {};

  // NOTE: This is for world gen only really
  F32 tile_size_in_meters = 1.4f;
  F32 tile_depth_in_meters = 3.f;

  Vec3 tile_dim = vec3(tile_size_in_meters, tile_size_in_meters, tile_depth_in_meters);
  Vec3 offset = Vec_Hadamard(tile_dim, vec3((F32)abs_tile_x, (F32)abs_tile_y, (F32)abs_tile_z));

  World_Position result = Map_Into_Chunk_Space(world, base_pos, offset + additional_offset);

  ASSERT(Is_Canonical(world, result.offset_));

  return result;
}

internal Add_Low_Entity_Result Add_Grounded_Entity(Game_State* game_state, Entity_Type type, World_Position pos,
                                                   Sim_Entity_Collision_Volume_Group* collision)

{
  Add_Low_Entity_Result result = Add_Low_Entity(game_state, type, pos);
  result.low->sim.collision = collision;
  return result;
}
internal Add_Low_Entity_Result Add_Sword(Game_State* game_state)

{
  Add_Low_Entity_Result entity =
      Add_Grounded_Entity(game_state, ENTITY_TYPE_SWORD, Null_Position(), game_state->sword_collision);

  Set_Flag(&entity.low->sim, ENTITY_FLAG_MOVEABLE);
  return entity;
}

internal Add_Low_Entity_Result Add_Player(Game_State* game_state)
{
  World_Position pos = game_state->camera_pos;
  Add_Low_Entity_Result entity = Add_Grounded_Entity(game_state, ENTITY_TYPE_PLAYER, pos, game_state->player_collision);

  entity.low->sim.max_health = 25;
  entity.low->sim.health = entity.low->sim.max_health;

  Set_Flag(&entity.low->sim, ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVEABLE);

  Add_Low_Entity_Result sword = Add_Sword(game_state);
  entity.low->sim.sword.index = sword.low_index;

  if (game_state->camera_follow_entity_index == 0)
  {
    game_state->camera_follow_entity_index = entity.low_index;
  }
  return entity;
}
internal Add_Low_Entity_Result Add_Space(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)
{

  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity =
      Add_Grounded_Entity(game_state, ENTITY_TYPE_SPACE, pos, game_state->standard_room_collision);

  Set_Flag(&entity.low->sim, ENTITY_FLAG_TRAVERSABLE);

  return entity;
}

internal Add_Low_Entity_Result Add_Wall(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)
{

  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity = Add_Grounded_Entity(game_state, ENTITY_TYPE_WALL, pos, game_state->wall_collision);

  Set_Flag(&entity.low->sim, ENTITY_FLAG_COLLIDES);

  return entity;
}
internal Add_Low_Entity_Result Add_Stair(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)
{
  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity = Add_Grounded_Entity(game_state, ENTITY_TYPE_STAIR, pos, game_state->stair_collision);

  entity.low->sim.walkable_height = game_state->typical_floor_height;
  entity.low->sim.walkable_dim = entity.low->sim.collision->total_volume.dim.xy;
  Set_Flag(&entity.low->sim, ENTITY_FLAG_COLLIDES);

  return entity;
}

internal U32 Add_Monster(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)

{
  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity =
      Add_Grounded_Entity(game_state, ENTITY_TYPE_MONSTER, pos, game_state->monster_collision);

  entity.low->sim.max_health = 30;
  entity.low->sim.health = entity.low->sim.max_health;

  Set_Flag(&entity.low->sim, ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVEABLE);

  return entity.low_index;
}
internal U32 Add_Familiar(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)

{
  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity =
      Add_Grounded_Entity(game_state, ENTITY_TYPE_FAMILIAR, pos, game_state->familiar_collision);

  entity.low->sim.max_health = 12;
  entity.low->sim.health = entity.low->sim.max_health / 2;

  // Set_Flag(&entity.low->sim, ENTITY_FLAG_MOVEABLE);

  return entity.low_index;
}
internal void* Push_Render_Piece(Render_Group* group, U32 size)
{
  void* result = 0;
  if ((group->push_buffer_size + size) < group->max_push_buffer_size)
  {
    result = group->push_buffer_base + group->push_buffer_size;
    group->push_buffer_size += size;
  }
  else
  {
    Invalid_Code_Path;
  }

  return result;
}

internal inline void Add_Render_Piece(Render_Group* group, Loaded_Bitmap* bitmap, Vec2 bmp_inner_offset, Vec2 dim,
                                      Vec2 offset, F32 offset_z, Vec2 align, F32 scale, Vec4 color, F32 entity_cz,
                                      B32 wire_frame = false)

{
  Entity_Render_Piece* piece = (Entity_Render_Piece*)Push_Render_Piece(group, sizeof(Entity_Render_Piece));
  piece->basis = group->default_basis;
  piece->bitmap = bitmap;
  piece->bmp_offset_x = (S32)bmp_inner_offset.x;
  piece->bmp_offset_y = (S32)bmp_inner_offset.y;
  piece->dim = dim;
  piece->offset = ((vec2(offset.x, -offset.y) * group->meters_to_pixels) - align);
  piece->offset_z = offset_z;
  piece->scale = scale;
  piece->color = color;
  piece->entity_cz = entity_cz;
  piece->wire_frame = wire_frame;
}

internal inline void Add_Bitmap_Render(Render_Group* group, Loaded_Bitmap* bitmap, Vec2 offset, F32 offset_z,
                                       Vec2 align, F32 scale, F32 alpha = 1.f, F32 entity_cz = 1.f)
{
  Add_Render_Piece(group, bitmap, vec2(0.f, 0.f), {{(F32)bitmap->width, (F32)bitmap->height}}, offset, offset_z, align,
                   scale, {{0, 0, 0, alpha}}, entity_cz);
}
internal inline void Add_Sprite_Bitmap_Render(Render_Group* group, Loaded_Bitmap* bitmap, Vec2 offset, F32 offset_z,
                                              Vec2 align, F32 scale, F32 alpha = 1.f, F32 entity_cz = 1.f)
{
  scale = scale * group->sprite_scale;
  Add_Bitmap_Render(group, bitmap, offset, offset_z, align, scale, alpha, entity_cz);
}

internal inline void Add_Sprite_Render_Piece(Render_Group* group, Sprite_Sheet* sprite_sheet, U32 sprite_index,
                                             Vec2 offset, F32 offset_z, Vec2 align, F32 scale, F32 alpha = 1.f,
                                             F32 entity_cz = 1.f)
{
  Sprite sprite = sprite_sheet->sprites[sprite_index];
  scale = scale * group->sprite_scale;
  Add_Render_Piece(group, &sprite_sheet->bitmap, vec2((F32)sprite.x, (F32)sprite.y),
                   vec2((F32)sprite.width, (F32)sprite.height), offset, offset_z, align, scale, vec4(0, 0, 0, alpha),
                   entity_cz);
}
internal inline void Add_Bitmap_Center_Render(Render_Group* group, Loaded_Bitmap* bitmap, Vec2 offset, F32 offset_z,
                                              Vec2 align, F32 scale, F32 alpha = 1.f, F32 entity_cz = 1.f)
{
  align = vec2(align.x + 0.5f * (F32)bitmap->width, align.y + 0.5f * (F32)bitmap->height);
  Add_Bitmap_Render(group, bitmap, offset, offset_z, align, scale, alpha, entity_cz);
}

internal inline void Add_Rect_Render(Render_Group* group, Vec2 offset, F32 offset_z, Vec2 align, Vec2 dim, F32 scale,
                                     Color4F color, F32 entity_cz = 1.f, B32 wire_frame = false)

{
  Add_Render_Piece(group, NULL, vec2(0.f, 0.f), dim * group->meters_to_pixels, offset, offset_z, align, scale, color,
                   entity_cz, wire_frame);
}

internal inline void Add_Pixel_Rect_Render(Render_Group* group, Vec2 offset, F32 offset_z, Vec2 align, Vec2 dim,
                                           F32 scale, Color4F color, F32 entity_cz = 1.f, B32 wire_frame = false)

{
  scale = scale * group->sprite_scale;
  Add_Render_Piece(group, NULL, vec2(0.f, 0.f), dim, offset, offset_z, align, scale, color, entity_cz, wire_frame);
}
internal inline void Add_Rect_Outline_Render(Render_Group* group, Vec2 offset, F32 offset_z, Vec2 align, Vec2 dim,
                                             F32 scale, Color4F color, F32 entity_cz = 1.f, F32 thickness = 1.0f)
{

  // TOP/BOTTOM
  Vec2 half_dim_y = vec2(0.f, .5f * dim.y);
  Vec2 half_dim_x = vec2(0.5f * dim.x, 0.f);

  Vec2 dim_horizontal = vec2(dim.x, thickness);
  Vec2 dim_vertical = vec2(thickness, dim.y);

  Add_Rect_Render(group, offset + half_dim_y, offset_z, align, dim_horizontal, scale, color, entity_cz, false);
  Add_Rect_Render(group, offset - half_dim_y, offset_z, align, dim_horizontal, scale, color, entity_cz, false);

  // LEFT/RIGHT
  Add_Rect_Render(group, offset - half_dim_x, offset_z, align, dim_vertical, scale, color, entity_cz, false);
  Add_Rect_Render(group, offset + half_dim_x, offset_z, align, dim_vertical, scale, color, entity_cz, false);
}
internal inline void Draw_Health(Render_Group* group, Sim_Entity* entity, F32 x_scale)

{
  Vec2 scale = vec2(x_scale, 1.f);
  F32 health_ratio = (F32)entity->health / (F32)entity->max_health;

  Vec2 max_hp_pos = vec2(0, 16) * scale;
  Vec2 max_hp_bar = vec2(16, 2) * scale;

  Vec2 hp_pos = vec2(max_hp_pos.x, max_hp_pos.y);
  hp_pos.x += (1.f - health_ratio) * 0.5f * max_hp_bar.x;

  Vec2 hp_bar = vec2(max_hp_bar.x * health_ratio, max_hp_bar.y);

  // red hp background
  Add_Pixel_Rect_Render(group, vec2(0), 0, max_hp_pos, max_hp_bar, 1.f, vec4(0.8f, 0.2f, 0.2f, 1.f));
  // green hp
  Add_Pixel_Rect_Render(group, vec2(0), 0, hp_pos, hp_bar, 1.f, vec4(0.2f, 0.8f, 0.2f, 1.f));
}
internal void Fill_Ground_Chunk(Transient_State* transient_state, Game_State* game_state, Ground_Buffer* ground_buffer,
                                World_Position* chunk_pos)
{
  Loaded_Bitmap* buffer = &ground_buffer->bitmap;
  ground_buffer->pos = *chunk_pos;

  F32 width = (F32)buffer->width;
  F32 height = (F32)buffer->height;

  for (S32 chunk_offset_y = -1; chunk_offset_y <= 1; ++chunk_offset_y)
  {
    for (S32 chunk_offset_x = -1; chunk_offset_x <= 1; ++chunk_offset_x)
    {

      S32 chunk_x = chunk_pos->chunk_x + chunk_offset_x;
      S32 chunk_y = chunk_pos->chunk_y + chunk_offset_y;
      S32 chunk_z = chunk_pos->chunk_z;

      // TODO: look into wang hashing
      Random_Series series = Seed(397 * (U32)chunk_x + 503 * (U32)chunk_y + 37 * (U32)chunk_z);

      Vec2 center = vec2(width * (F32)chunk_offset_x, -height * (F32)chunk_offset_y);

      for (U32 grass_index = 0; grass_index < 90; ++grass_index)
      {
        Loaded_Bitmap* stamp;
        if (Random_0_To_1(&series) > 0.05f && chunk_pos->chunk_z == 1)
        {

          stamp = game_state->grass + Random_Choice(&series, Array_Count(game_state->grass));
        }
        else
        {
          stamp = game_state->ground + Random_Choice(&series, Array_Count(game_state->ground));
        }

        Vec2 bitmap_center = 0.5f * vec2i(stamp->width, stamp->height);
        Vec2 offset = vec2(width * Random_0_To_1(&series), height * Random_0_To_1(&series));
        Vec2 pos = offset - bitmap_center + center;

        Draw_BMP(buffer, stamp, pos.x, pos.y, 1.f);
      }
    }
  }
  for (S32 chunk_offset_y = -1; chunk_offset_y <= 1; ++chunk_offset_y)
  {
    for (S32 chunk_offset_x = -1; chunk_offset_x <= 1; ++chunk_offset_x)
    {

      S32 chunk_x = chunk_pos->chunk_x + chunk_offset_x;
      S32 chunk_y = chunk_pos->chunk_y + chunk_offset_y;
      S32 chunk_z = chunk_pos->chunk_z;

      // TODO: look into wang hashing
      Random_Series series = Seed(997 * (U32)chunk_x + 503 * (U32)chunk_y + 11 * (U32)chunk_z);

      Vec2 center = vec2(width * (F32)chunk_offset_x, -height * (F32)chunk_offset_y);

      for (U32 grass_index = 0; grass_index < 40; ++grass_index)
      {
        Loaded_Bitmap* stamp;

        stamp = game_state->tuft + Random_Choice(&series, Array_Count(game_state->tuft));

        Vec2 bitmap_center = 0.5f * vec2i(stamp->width, stamp->height);
        Vec2 offset = vec2(width * Random_0_To_1(&series), height * Random_0_To_1(&series));
        Vec2 pos = offset - bitmap_center + center;

        Draw_BMP(buffer, stamp, pos.x, pos.y);
      }
    }
  }
}

internal void Clear_Collision_Rules_For(Game_State* game_state, U32 storage_index)
{
  for (U32 rule_pp_index = 0; rule_pp_index < Array_Count(game_state->collision_rule_hash); ++rule_pp_index)
  {
    for (Pairwise_Collision_Rule** rule_pp = &game_state->collision_rule_hash[rule_pp_index]; *rule_pp;)
    {
      if (((*rule_pp)->storage_index_a == storage_index) || ((*rule_pp)->storage_index_b == storage_index))
      {
        Pairwise_Collision_Rule* rule_to_delete = *rule_pp;
        // Delete
        *rule_pp = (*rule_pp)->next_in_hash;
        game_state->collision_rule_count--;

        // push to free list
        rule_to_delete->next_in_hash = game_state->first_free_collision_rule;
        game_state->first_free_collision_rule = rule_to_delete;
      }
      else
      {
        rule_pp = &(*rule_pp)->next_in_hash;
      }
    }
  }
}
internal B32 Remove_Collision_Rule(Game_State* game_state, U32 storage_index_a, U32 storage_index_b)
{
  return false;
}

internal B32 Add_Collision_Rule(Game_State* game_state, U32 storage_index_a, U32 storage_index_b, B32 should_collide)
{
  B32 added = false;
  if (storage_index_a > storage_index_b)

  {
    U32 temp = storage_index_a;
    storage_index_a = storage_index_b;
    storage_index_b = temp;
  }

  Pairwise_Collision_Rule* found = 0;
  U32 hash_bucket = storage_index_a & (Array_Count(game_state->collision_rule_hash) - 1);
  for (Pairwise_Collision_Rule* rule = game_state->collision_rule_hash[hash_bucket]; rule; rule = rule->next_in_hash)
  {
    if ((rule->storage_index_a == storage_index_a) && (rule->storage_index_b == storage_index_b))
    {
      found = rule;
      break;
    }
  }
  if (!found)
  {
    found = game_state->first_free_collision_rule;
    if (found)
    {
      game_state->first_free_collision_rule = found->next_in_hash;
    }
    else
    {
      found = Push_Struct(&game_state->world_arena, Pairwise_Collision_Rule);
    }

    found->next_in_hash = game_state->collision_rule_hash[hash_bucket];
    game_state->collision_rule_hash[hash_bucket] = found;
    game_state->collision_rule_count++;

    added = true;
  }

  if (found)
  {
    found->storage_index_a = storage_index_a;
    found->storage_index_b = storage_index_b;
    found->should_collide = should_collide;
  }
  return added;
}
inline Sim_Entity_Collision_Volume_Group* Make_Null_Collision_Volume(Game_State* game_state)
{
  Sim_Entity_Collision_Volume_Group* result = Push_Struct(&game_state->world_arena, Sim_Entity_Collision_Volume_Group);
  result->volumes = 0;
  result->volume_count = 0;
  result->total_volume.dim = vec3(0);
  result->total_volume.offset_pos = vec3(0);

  return result;
}
inline Sim_Entity_Collision_Volume_Group* Make_Simple_Grounded_Collision_Volume(Game_State* game_state, Vec3 dim)
{
  Sim_Entity_Collision_Volume_Group* result = Push_Struct(&game_state->world_arena, Sim_Entity_Collision_Volume_Group);
  result->volume_count = 1;
  result->volumes = Push_Array(&game_state->world_arena, result->volume_count, Sim_Entity_Collision_Volume);
  result->volumes[0].dim = dim;
  result->volumes[0].offset_pos = vec3(0, 0, 0.5f * dim.z);
  result->total_volume = result->volumes[0];

  return result;
}
internal void Clear_Bitmap(Loaded_Bitmap* bitmap)
{
  if (bitmap->memory)
  {
    S32 total_bitmap_size = bitmap->width * bitmap->height * BITMAP_BYTES_PER_PIXEL;
    Zero_Size((size_t)total_bitmap_size, bitmap->memory);
  }
}

internal Loaded_Bitmap Make_Empty_Bitmap(Arena* arena, S32 width, S32 height, B32 clear_to_zero = false)
{
  Loaded_Bitmap result = {};
  result.width = width;
  result.height = height;
  result.pitch_in_bytes = width * BITMAP_BYTES_PER_PIXEL;
  S32 total_bitmap_size = width * height * BITMAP_BYTES_PER_PIXEL;
  result.memory = Push_Size(arena, (size_t)total_bitmap_size);
  if (clear_to_zero)
  {
    Clear_Bitmap(&result);
  }

  return result;
}

internal void Request_Ground_Buffers(World_Position center_pos, Rect3 bounds)
{
  bounds = Rect_Add_Offset(bounds, center_pos.offset_);
  center_pos.offset_ = vec3(0);

  // TODO: test fill
  // Fill_Ground_Chunk(transient_state, game_state, transient_state->ground_buffers, &game_state->camera_pos);
}

extern "C" GAME_UPDATE_AND_RENDER(Game_Update_And_Render)
{
  ASSERT((&input->controllers[0].button_count - &input->controllers[0].buttons[0]) ==
         (Array_Count(input->controllers[0].buttons)));
  ASSERT(sizeof(Game_State) < memory->permanent_storage_size);

  S32 ground_buffer_width = 256;
  S32 ground_buffer_height = 256;

  Game_State* game_state = (Game_State*)memory->permanent_storage;
  if (!memory->is_initialized)
  {

    Initialize_Arena(&game_state->world_arena, memory->permanent_storage_size - sizeof(Game_State),
                     (U8*)memory->permanent_storage + sizeof(Game_State));

    Arena* world_arena = &game_state->world_arena;

    game_state->typical_floor_height = 3.f;
    game_state->meters_to_pixels = 32.f;
    game_state->pixels_to_meters = 1.f / game_state->meters_to_pixels;
    Vec3 world_chunk_dim_in_meters =
        vec3((F32)ground_buffer_width * game_state->pixels_to_meters,
             (F32)ground_buffer_height * game_state->pixels_to_meters, game_state->typical_floor_height);

    game_state->world = Push_Struct(world_arena, World);
    World* world = game_state->world;
    Initialize_World(world, world_chunk_dim_in_meters);

    // F32 tile_size_in_pixels = TILE_SIZE_IN_PIXELS;
    F32 tile_size_in_meters = 1.4f;
    F32 tile_depth_in_meters = game_state->typical_floor_height;
    game_state->draw_scale = tile_size_in_meters;
    F32 sprite_size = 16.f;
    game_state->sprite_scale = game_state->draw_scale * (game_state->meters_to_pixels / sprite_size);

    game_state->null_collision = Make_Null_Collision_Volume(game_state);
    game_state->sword_collision = Make_Simple_Grounded_Collision_Volume(game_state, vec3(0.5f, 0.5f, 0.1f));
    game_state->player_collision = Make_Simple_Grounded_Collision_Volume(game_state, vec3(1.f, 0.6f, 0.7f));
    game_state->stair_collision = Make_Simple_Grounded_Collision_Volume(
        game_state, vec3(tile_size_in_meters, 2.f * tile_size_in_meters, 1.001f * tile_depth_in_meters));
    game_state->familiar_collision = Make_Simple_Grounded_Collision_Volume(game_state, vec3(0.3f, 0.3f, 0.3f));
    game_state->wall_collision = Make_Simple_Grounded_Collision_Volume(
        game_state, vec3(tile_size_in_meters, tile_size_in_meters, tile_depth_in_meters * 0.99f));
    game_state->monster_collision = Make_Simple_Grounded_Collision_Volume(game_state, vec3(0.95f, 0.5f, 1.f));
    game_state->standard_room_collision = Make_Simple_Grounded_Collision_Volume(
        game_state, vec3(tile_size_in_meters * TILES_PER_WIDTH, tile_size_in_meters * TILES_PER_HEIGHT,
                         0.9f * tile_depth_in_meters * TILES_PER_DEPTH));

    // NOTE: first entity is NULL entity
    Add_Low_Entity(game_state, ENTITY_TYPE_NULL, Null_Position());

    game_state->knight_sprite_sheet =
        DEBUG_Load_SpriteSheet_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                   memory->DEBUG_Platform_Free_File_Memory, world_arena, "assets/knight.bmp", 16, 16);
    game_state->grass_sprite_sheet = DEBUG_Load_SpriteSheet_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                                                memory->DEBUG_Platform_Free_File_Memory, world_arena,
                                                                "assets/grass_tileset.bmp", 16, 16);
    game_state->slime_sprite_sheet =
        DEBUG_Load_SpriteSheet_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                   memory->DEBUG_Platform_Free_File_Memory, world_arena, "assets/slime1.bmp", 16, 16);
    game_state->eye_sprite_sheet =
        DEBUG_Load_SpriteSheet_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                   memory->DEBUG_Platform_Free_File_Memory, world_arena, "assets/eye.bmp", 16, 16);

    game_state->shadow_bmp = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                            memory->DEBUG_Platform_Free_File_Memory, world_arena, "assets/shadow.bmp");
    game_state->stone_floor_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/stone_floor.bmp");
    game_state->stair_up_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/stair_up.bmp");
    game_state->stair_down_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/stair_down.bmp");
    game_state->test_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/structured_art.bmp");
    game_state->wall1_bmp = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                           memory->DEBUG_Platform_Free_File_Memory, world_arena, "assets/rock.bmp");
    game_state->wall2_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/stone_wall.bmp");
    game_state->pillar_bmp = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                            memory->DEBUG_Platform_Free_File_Memory, world_arena, "assets/pillar.bmp");
    game_state->shuriken_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/shuriken.bmp");
    game_state->grass[0] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                          memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/grass00.bmp");
    game_state->grass[1] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                          memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/grass01.bmp");
    game_state->ground[0] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                           memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/ground00.bmp");

    game_state->ground[1] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                           memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/ground01.bmp");

    game_state->ground[2] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                           memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/ground02.bmp");

    game_state->ground[3] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                           memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/ground03.bmp");
    game_state->tuft[0] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                         memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/tuft00.bmp");

    game_state->tuft[1] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                         memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/tuft01.bmp");

    game_state->tuft[2] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                         memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/tuft02.bmp");

    Random_Series series = {1234};

    S32 screen_base_x = 0;
    S32 screen_base_y = 0;
    S32 screen_base_z = 0;

    S32 screen_x = screen_base_x;
    S32 screen_y = screen_base_y;
    S32 abs_tile_z = screen_base_z;

    B32 door_left = false;
    B32 door_right = false;
    B32 door_top = false;
    B32 door_bottom = false;
    B32 door_up = false;
    B32 door_down = false;

    for (U32 screen_index = 0; screen_index < 2000; ++screen_index)
    {

      U32 door_direction = Random_Choice(&series, (door_up || door_down) ? 2 : 3);

      B32 created_z_door = false;
      if (door_direction == 2)
      {
        created_z_door = true;
        if (abs_tile_z == screen_base_z)
        {
          door_up = true;
        }
        else
        {
          door_down = true;
        }
      }
      else if (door_direction == 1)
      {
        door_right = true;
      }
      else
      {
        door_top = true;
      }

      Add_Space(game_state, (screen_x * TILES_PER_WIDTH + TILES_PER_WIDTH / 2),
                (screen_y * TILES_PER_HEIGHT + TILES_PER_HEIGHT / 2), abs_tile_z);

      for (S32 tile_y = 0; tile_y < TILES_PER_HEIGHT; ++tile_y)
      {
        for (S32 tile_x = 0; tile_x < TILES_PER_WIDTH; ++tile_x)
        {
          S32 abs_tile_x = screen_x * TILES_PER_WIDTH + tile_x;
          S32 abs_tile_y = screen_y * TILES_PER_HEIGHT + tile_y;

          B32 wall = false;
          if ((tile_x == 0) && (!door_left || (tile_y != TILES_PER_HEIGHT / 2)))
          {
            wall = true;
          }
          if ((tile_x == (TILES_PER_WIDTH - 1)) && (!door_right || (tile_y != TILES_PER_HEIGHT / 2)))
          {
            wall = true;
          }

          if ((tile_y == 0) && (!door_bottom || (tile_x != TILES_PER_WIDTH / 2)))
          {
            wall = true;
          }
          if ((tile_y == (TILES_PER_HEIGHT - 1)) && (!door_top || (tile_x != TILES_PER_WIDTH / 2)))
          {
            wall = true;
          }
          if (wall)
          {
            Add_Wall(game_state, abs_tile_x, abs_tile_y, abs_tile_z);
          }
          else if (created_z_door)
          {
            if ((tile_x == 10) && (tile_y == 5))
            {
              if (door_down)
              {
                int a = 4;
                a += a;
              }
              Add_Stair(game_state, abs_tile_x, abs_tile_y, door_down ? abs_tile_z - 1 : abs_tile_z);
            }
          }
        }
      }
      door_left = door_right;
      door_bottom = door_top;
      door_right = false;
      door_top = false;
      if (door_direction == 2)
      {
        door_down = !door_down;
        door_up = !door_up;
      }
      else
      {
        door_up = door_down = false;
      }

      if (door_direction == 2)
      {
        if (abs_tile_z == screen_base_z)
        {
          abs_tile_z = screen_base_z + 1;
        }
        else
        {
          abs_tile_z = screen_base_z;
        }
      }
      else if (door_direction == 1)

      {
        screen_x += 1;
      }
      else
      {
        screen_y += 1;
      }
    }

    World_Position new_camera_pos = {};
    S32 camera_tile_x = screen_base_x * TILES_PER_WIDTH + 17 / 2;
    S32 camera_tile_y = screen_base_y * TILES_PER_HEIGHT + 9 / 2;
    S32 camera_tile_z = screen_base_z;

    new_camera_pos = Chunk_Position_From_Tile_Position(world, camera_tile_x, camera_tile_y, camera_tile_z);

    Add_Monster(game_state, camera_tile_x + 2, camera_tile_y + 2, camera_tile_z);
    for (U32 familiar_index = 0; familiar_index < 25; ++familiar_index)
    {
      S32 x = camera_tile_x + Random_Between(&series, -7, 7);
      S32 y = camera_tile_y + Random_Between(&series, -4, 1);

      Add_Familiar(game_state, x, y, camera_tile_z);
    }

    game_state->camera_pos = new_camera_pos;

    memory->is_initialized = true;
  }

  ASSERT(sizeof(Transient_State) < memory->transient_storage_size);
  Transient_State* transient_state = (Transient_State*)memory->transient_storage;
  if (!transient_state->is_initialized)
  {
    Initialize_Arena(&transient_state->transient_arena, memory->transient_storage_size - sizeof(Transient_State),
                     (U8*)memory->transient_storage + sizeof(Transient_State));

    transient_state->ground_buffer_count = 64; // 128
    transient_state->ground_buffers =
        Push_Array(&transient_state->transient_arena, transient_state->ground_buffer_count, Ground_Buffer);
    transient_state->ground_bitmap_template =
        Make_Empty_Bitmap(&transient_state->transient_arena, ground_buffer_width, ground_buffer_height, true);
    for (U32 ground_buffer_index = 0; ground_buffer_index < transient_state->ground_buffer_count; ++ground_buffer_index)
    {
      Ground_Buffer* ground_buffer = transient_state->ground_buffers + ground_buffer_index;

      ground_buffer->bitmap =
          Make_Empty_Bitmap(&transient_state->transient_arena, ground_buffer_width, ground_buffer_height, true);
      ground_buffer->pos = Null_Position();
    }

    transient_state->is_initialized = true;
  }

  if (input->executable_reloaded)
  {
    for (U32 ground_buffer_index = 0; ground_buffer_index < transient_state->ground_buffer_count; ++ground_buffer_index)
    {
      Ground_Buffer* ground_buffer = transient_state->ground_buffers + ground_buffer_index;
      ground_buffer->bitmap =
          Make_Empty_Bitmap(&transient_state->transient_arena, ground_buffer_width, ground_buffer_height, true);
      ground_buffer->pos = Null_Position();
    }
  }

  F32 delta_time = input->delta_time_s;

  for (S32 controller_index = 0; controller_index < (S32)Array_Count(input->controllers); controller_index++)
  {
    Game_Controller_Input* controller = Get_Controller(input, controller_index);
    if (controller->dpad_up.ended_down)
    {
      game_state->volume += 0.001f;
    }
    else if (controller->dpad_down.ended_down)
    {
      game_state->volume -= 0.001f;
    }
    game_state->volume = CLAMP(game_state->volume, 0.0f, 0.5f);

    Controlled_Player* con_player = game_state->controlled_players + controller_index;
    {
      U32 entity_index = con_player->entity_index;
      *con_player = {};
      con_player->entity_index = entity_index;
    }

    if (con_player->entity_index == 0)
    {
      if (controller->start.ended_down)
      {
        *con_player = {};
        con_player->entity_index = Add_Player(game_state).low_index;
      }
    }
    else
    {
      // NOTE: player movement

      con_player->move_dir = controller->stick_left;

      if (controller->action_up.ended_down)
      {
        con_player->sprint = true;
      }
      // NOTE: player jump
      if (controller->start.ended_down)
      {
        con_player->jump_vel = 3.f;
      }

      if (controller->action_up.ended_down)
      {
        con_player->attack_dir = vec2(0.f, 1.f);
      }
      if (controller->action_down.ended_down)
      {
        con_player->attack_dir = vec2(0.f, -1.f);
      }
      if (controller->action_left.ended_down)
      {
        con_player->attack_dir = vec2(-1.f, 0.f);
      }
      if (controller->action_right.ended_down)
      {
        con_player->attack_dir = vec2(1.f, 0.f);
      }
    }
  }

  World* world = game_state->world;

  Temporary_Memory render_memory = Begin_Temp_Memory(&transient_state->transient_arena);
  Render_Group* render_group = Allocate_Render_Group(&transient_state->transient_arena, Megabytes(4),
                                                     game_state->meters_to_pixels, game_state->sprite_scale);
  Loaded_Bitmap draw_buffer_ = {};
  Loaded_Bitmap* draw_buffer = &draw_buffer_;
  draw_buffer->width = buffer->width;
  draw_buffer->height = buffer->height;
  draw_buffer->pitch_in_bytes = buffer->pitch_in_bytes;
  draw_buffer->memory = buffer->memory;

  // NOTE: Clear Buffer --------------------------------------------------------
  Draw_Rectf(draw_buffer, 0, 0, (F32)draw_buffer->width, (F32)draw_buffer->height, 0.35f, 0.58f, 0.93f);
  //------------------------------------

  Vec2 screen_in_meters = vec2i(buffer->width, buffer->height) * game_state->pixels_to_meters;
  Rect3 camera_bounds_in_meters = Rect_Center_Dim(vec3(0), vec3(screen_in_meters.x, screen_in_meters.y, 0.f));

  // camera_bounds_in_meters =
  //     Rect_Add_Radius(camera_bounds_in_meters, game_state->world->chunk_dim_in_meters.x * vec3(0.5f, 0.5f, 0));
  // NOTE: Draw chunk bounds
  {

    World_Position min_chunk_pos =
        Map_Into_Chunk_Space(world, game_state->camera_pos, Rect_Get_Min_Corner(camera_bounds_in_meters));
    World_Position max_chunk_pos =
        Map_Into_Chunk_Space(world, game_state->camera_pos, Rect_Get_Max_Corner(camera_bounds_in_meters));

    for (S32 chunk_z = min_chunk_pos.chunk_z; chunk_z <= max_chunk_pos.chunk_z; ++chunk_z)
    {
      for (S32 chunk_y = min_chunk_pos.chunk_y; chunk_y <= max_chunk_pos.chunk_y; ++chunk_y)
      {
        for (S32 chunk_x = min_chunk_pos.chunk_x; chunk_x <= max_chunk_pos.chunk_x; ++chunk_x)
        {
          World_Position chunk_center_pos = Centered_Chunk_Point(chunk_x, chunk_y, chunk_z);
          Ground_Buffer* furthest_buffer = 0;
          F32 furthest_distance_from_camera = 0.f;
          for (U32 ground_buffer_index = 0; ground_buffer_index < transient_state->ground_buffer_count;
               ++ground_buffer_index)
          {
            Ground_Buffer* ground_buffer = transient_state->ground_buffers + ground_buffer_index;
            if (Are_In_Same_Chunk(world, &ground_buffer->pos, &chunk_center_pos))
            {
              furthest_buffer = 0;
              break;
            }
            else if (Is_Valid(ground_buffer->pos))
            {
              Vec3 relative_pos = World_Pos_Subtract(world, &ground_buffer->pos, &game_state->camera_pos);
              F32 distance_from_camera = Vec_Length_Sq(relative_pos.xy);
              if (distance_from_camera > furthest_distance_from_camera)
              {
                furthest_distance_from_camera = distance_from_camera;
                furthest_buffer = ground_buffer;
              }
            }
            else
            {
              furthest_buffer = ground_buffer;
              furthest_distance_from_camera = F32_MAX;
            }
          }
          if (furthest_buffer)
          {

            Fill_Ground_Chunk(transient_state, game_state, furthest_buffer, &chunk_center_pos);
          }

          // Rect2 rect = Rect_Center_Dim(screen_center + offset.xy,
          //                              game_state->meters_to_pixels * game_state->world->chunk_dim_in_meters.xy);
          // Draw_Rect_Outline(draw_buffer, rect, vec3(1, 1, 0), 2.f);
        }
      }
    }
  }

  // NOTE: Draw floor
  Vec2 screen_center = Vec2{{0.5f * (F32)draw_buffer->width, 0.5f * (F32)draw_buffer->height}};
  for (U32 ground_buffer_index = 0; ground_buffer_index < transient_state->ground_buffer_count; ++ground_buffer_index)
  {
    Ground_Buffer* ground_buffer = transient_state->ground_buffers + ground_buffer_index;
    if (Is_Valid(ground_buffer->pos))
    {
      Loaded_Bitmap* bitmap = &ground_buffer->bitmap;

      Vec3 offset = World_Pos_Subtract(game_state->world, &ground_buffer->pos, &game_state->camera_pos);

      if (ground_buffer->pos.chunk_y == 2)
      {
        Add_Rect_Outline_Render(render_group, offset.xy, 0, vec2(0), game_state->world->chunk_dim_in_meters.xy, 1.f,
                                vec4(1, 1, 0, 1), 1.f, 0.08f);
      }
      // Draw_Rect_Outline(draw_buffer, rect, vec3(1, 1, 0), 2.f);
      if (ground_buffer->pos.chunk_z == game_state->camera_pos.chunk_z)
      {
        // Draw_BMP(draw_buffer, &bitmap, ground_center.x, ground_center.y, scale, true);
        Add_Bitmap_Center_Render(render_group, bitmap, offset.xy, offset.z, vec2(0), 1.f);
      }
    }
  }

  // TODO: how big do we want to expand here
  Vec3 sim_bounds_expansion = world->chunk_dim_in_meters;
  Rect3 sim_bounds = Rect_Add_Radius(camera_bounds_in_meters, sim_bounds_expansion);

  Temporary_Memory sim_memory = Begin_Temp_Memory(&transient_state->transient_arena);
  Sim_Region* sim_region = Begin_Sim(&transient_state->transient_arena, game_state, game_state->world,
                                     game_state->camera_pos, sim_bounds, input->delta_time_s);

  // NOTE:Draw entities

  Vec2 shadow_align = vec2(6, 4);
  Vec2 sprite_align = vec2(8, 8);
  Vec2 unit_align = vec2(8, 14);
  Vec2 sprite1x2_align = vec2(8, 24);

  for (U32 entity_index = 0; entity_index < sim_region->entity_count; ++entity_index)
  {
    Sim_Entity* entity = sim_region->entities + entity_index;

    if (!entity->updatable)
    {
      continue;
    }

    // TODO: bad design, this should be computed after update
    F32 base_z = entity->pos.z;
    F32 shadow_alpha = CLAMP((1.0f - 0.8f * base_z), 0.5f, 0.8f);
    F32 shadow_scale = shadow_alpha * 0.75f;

    Move_Spec move_spec = Default_Move_Spec();
    Vec3 accel = {};

    Render_Basis* basis = Push_Struct(&transient_state->transient_arena, Render_Basis);
    render_group->default_basis = basis;

    switch (entity->type)
    {
      case ENTITY_TYPE_PLAYER:
      {
        for (U32 control_index = 0; control_index < Array_Count(game_state->controlled_players); ++control_index)
        {
          Controlled_Player* con_player = game_state->controlled_players + control_index;
          if (entity->storage_index == con_player->entity_index)
          {
            if (entity->vel.z == 0.f && con_player->jump_vel > 0.f)

            {
              entity->vel.z = con_player->jump_vel;
            }

            move_spec.normalize_accel = true;
            move_spec.drag = 8.f;
            move_spec.speed = 50.f;
            move_spec.speed *= con_player->sprint ? 3.f : 1.f;
            accel = vec3(con_player->move_dir, 0.f);

            if (Vec_Length_Sq(con_player->attack_dir) > 0.f)
            {
              Sim_Entity* sword = entity->sword.ptr;
              if (sword && Has_Flag(sword, ENTITY_FLAG_NONSPATIAL))
              {
                sword->pos = entity->pos;
                sword->distance_limit = 5.f;
                Vec3 vel = vec3(10.f * con_player->attack_dir, 0.f);
                Make_Entity_Spatial(sword, entity->pos, vel);
                Add_Collision_Rule(game_state, entity->storage_index, sword->storage_index, false);
              }
            }
          }
        }

        Loaded_Bitmap* bmp = &game_state->shadow_bmp;
        Add_Sprite_Bitmap_Render(render_group, bmp, vec2(0.f, 0.f), 0, shadow_align, shadow_scale, shadow_alpha, 0.f);
        Add_Sprite_Render_Piece(render_group, &game_state->knight_sprite_sheet, entity->sprite_index, vec2(0.f, 0.f), 0,
                                unit_align, 1.f, 1.f);

        Draw_Health(render_group, entity, 1.f);
      }
      break;
      case ENTITY_TYPE_MONSTER:
      {

        Loaded_Bitmap* bmp = &game_state->shadow_bmp;
        entity->sprite_index++;
        entity->sprite_index = entity->sprite_index % ((S32)E_SPRITE_WALK_COUNT);

        Add_Sprite_Bitmap_Render(render_group, bmp, vec2(0.f, 0.f), 0, shadow_align, shadow_scale * 1.5f, shadow_alpha,
                                 1.f);

        Add_Sprite_Render_Piece(render_group, &game_state->slime_sprite_sheet, entity->sprite_index, vec2(0.f, 0.f), 0,
                                unit_align, 1.f, 1.f);
        Draw_Health(render_group, entity, 1.f);
      }
      break;
      case ENTITY_TYPE_FAMILIAR:
      {
        Sim_Entity* closest_player = {};
        F32 closest_player_distsq = Square(10.f); // NOTE: 10m max search

        // TODO: make spatial queries easy for things!
        Sim_Entity* test_entity = sim_region->entities;
        for (U32 test_entity_index = 0; test_entity_index < sim_region->entity_count;
             ++test_entity_index, ++test_entity)

        {
          if (test_entity->type == ENTITY_TYPE_PLAYER)
          {
            F32 test_distsq = Vec_Length_Sq(test_entity->pos - entity->pos);
            if (test_distsq < closest_player_distsq)
            {
              closest_player_distsq = test_distsq;
              closest_player = test_entity;
            }
          }
        }

        // NOTE: follow player
        if (closest_player && (closest_player_distsq > Square(3.5f)))
        {
          accel = (closest_player->pos - entity->pos);
        }

        move_spec.normalize_accel = true;
        move_spec.drag = 2.f;
        move_spec.speed = 6.f;

        entity->bob_time += delta_time;
        if (entity->bob_time > M_2PI)
        {
          entity->bob_time -= (F32)M_2PI;
        }

        F32 entity_scale_mod = 0.5f;
        F32 bob_sin = (0.5f * sinf(entity->bob_time) + 1.f);
        F32 bob_offset = (0.20f * -bob_sin + 0.50f) * game_state->draw_scale * game_state->meters_to_pixels;

        Vec2 familiar_align = sprite_align;
        familiar_align.y += bob_offset;

        shadow_alpha = 0.3f * shadow_alpha + 0.25f * bob_sin;
        shadow_scale = (entity_scale_mod * 0.8f) * shadow_scale + 0.1f * bob_sin;
        Loaded_Bitmap* bmp = &game_state->shadow_bmp;
        Add_Sprite_Bitmap_Render(render_group, bmp, vec2(0.f, 0.f), 0, shadow_align, shadow_scale, shadow_alpha, 1.f);

        Add_Sprite_Render_Piece(render_group, &game_state->eye_sprite_sheet, entity->sprite_index, vec2(0), 0.f,
                                familiar_align, entity_scale_mod, 1.f);

        Draw_Health(render_group, entity, entity_scale_mod);
      }
      break;
      case ENTITY_TYPE_SWORD:
      {

        if (entity->distance_limit <= 0.f)
        {
          Make_Entity_Nonspatial(entity);
          Clear_Collision_Rules_For(game_state, entity->storage_index);
          continue;
        }

        F32 entity_scale_mod = 0.5f;
        shadow_scale *= entity_scale_mod;
        shadow_alpha *= entity_scale_mod;

        Add_Sprite_Bitmap_Render(render_group, &game_state->shadow_bmp, vec2(0.f, 1.f), 0, shadow_align, shadow_scale,
                                 shadow_alpha, 0.f);
        Add_Sprite_Bitmap_Render(render_group, &game_state->shuriken_bmp, vec2(0.f, 0.f), 0, sprite_align,
                                 entity_scale_mod, 1.f);
      }
      break;
      case ENTITY_TYPE_WALL:
      {
        if (entity->chunk_z <= 1)
        {
          // Add_Bitmap_Render_Piece(&game_state->wall1_bmp, {{0.f, 0.f}}, 0, 1.f, 1.f);
          Add_Sprite_Bitmap_Render(render_group, &game_state->pillar_bmp, vec2(0.f, 0.f), 0, sprite1x2_align, 1.f, 1.f);
        }
        else if (entity->chunk_z > 1)
        {
          Add_Sprite_Bitmap_Render(render_group, &game_state->wall2_bmp, vec2(0.f, 0.f), 0, sprite_align, 1.f, 1.f);
          // Add_Bitmap_Render_Piece(&render_group, &game_state->pillar_bmp, vec2(0.f, 0.f), 0,
          // sprite1x2_align, 1.f, 1.f);
        }
      }
      break;
      case ENTITY_TYPE_STAIR:
      {
        if (entity->pos.z < 0)
        {

          Add_Rect_Render(render_group, vec2(0), 0, vec2(0, 0), entity->walkable_dim, 1.f, Color_Pastel_Red, 1.f, true);
          Add_Rect_Render(render_group, vec2(0), 0, vec2(0, 0), entity->walkable_dim, 1.f, Color_Pastel_Yellow, 0.f,
                          false);
        }
        else
        {
          Add_Rect_Render(render_group, vec2(0), entity->walkable_height, vec2(0, 0), entity->walkable_dim, 1.f,
                          Color_Pastel_Yellow, 1.f, true);
          Add_Rect_Render(render_group, vec2(0), 0, vec2(0, 0), entity->walkable_dim, 1.f, Color_Pastel_Red, 0.f,
                          false);
        }
      }
      break;
      case ENTITY_TYPE_SPACE:
      {
        // for (U32 volume_index = 0; volume_index < entity->collision->volume_count; ++volume_index)
        // {
        //   Sim_Entity_Collision_Volume* volume = entity->collision->volumes + volume_index;
        //   if (entity->chunk_z == game_state->camera_pos.chunk_z)
        //   {
        //     Add_Rect_Outline_Render(&render_group, volume->offset_pos.xy, 0, vec2(0), volume->dim.xy, 1.f,
        //                             Color_Pastel_Pink, 1.f, .14f);
        //   }
        //   else
        //   {
        //     Add_Rect_Render(&render_group, volume->offset_pos.xy, 0, vec2(0), volume->dim.xy,
        //
        //                     1.f, Color_Pastel_Pink, 1.f, true);
        //   }
        // }
      }
      break;
      case ENTITY_TYPE_NULL:
      default:
      {
        Invalid_Code_Path;
      };
    }
    // NOTE: Draw collision boxes
    {
      // NOTE: this floor is at z == 0 so check if entity collides with it in z
      F32 entity_collision_z = entity->collision->total_volume.dim.z + entity->pos.z;
      B32 is_on_this_floor = ((entity->pos.z >= 0.f) && (entity->pos.z < game_state->typical_floor_height)) ||
                             ((entity_collision_z >= 0.f) && (entity_collision_z < game_state->typical_floor_height));

      if (Has_Flag(entity, ENTITY_FLAG_COLLIDES) && is_on_this_floor)
      {
        Add_Rect_Render(render_group, vec2(0), 0, vec2(0), entity->collision->total_volume.dim.xy, 1.f,
                        Color_Pastel_Cyan, 1.f, true);
      }
    }
    // NOTE: Update Entity
    if (!Has_Flag(entity, ENTITY_FLAG_NONSPATIAL) && Has_Flag(entity, ENTITY_FLAG_MOVEABLE))
    {
      Move_Entity(game_state, sim_region, entity, delta_time, &move_spec, accel);
    }
    basis->pos = Get_Entity_Ground_Point(entity);
  }

  // NOTE: Draw all entities
  F32 meters_to_pixels = game_state->meters_to_pixels;
  for (U32 base_address = 0; base_address < render_group->push_buffer_size;)
  {
    Entity_Render_Piece piece = *(Entity_Render_Piece*)(void*)(render_group->push_buffer_base + base_address);
    base_address += sizeof(Entity_Render_Piece);

    Vec3 entity_base_pos = piece.basis->pos;
    F32 z_fudge = (1.f + 0.04f * piece.entity_cz * (entity_base_pos.z + piece.offset_z));
    z_fudge = MAX(0.5f, z_fudge);

    Vec2 entity_pos = vec2(screen_center.x + meters_to_pixels * entity_base_pos.x * z_fudge,
                           screen_center.y - meters_to_pixels * entity_base_pos.y * z_fudge);

    F32 z = -meters_to_pixels * entity_base_pos.z;
    F32 x = entity_pos.x + piece.offset.x * piece.scale;
    F32 y = entity_pos.y + (piece.offset.y) * piece.scale + (piece.entity_cz * z);

    F32 alpha_fudge = CLAMP(1.f + (1.f - entity_base_pos.z), 0.55f, 1.f);

    if (piece.bitmap)
    {
      Draw_BMP_Subset(draw_buffer, piece.bitmap, x, y, piece.bmp_offset_x, piece.bmp_offset_y, (S32)piece.dim.x,
                      (S32)piece.dim.y, piece.scale * z_fudge, true, piece.color.a * alpha_fudge);
    }
    else
    {
      Rect2 r = Rect_Center_Dim(vec2(x, y), piece.dim * piece.scale * z_fudge);
      if (piece.wire_frame)
      {
        Draw_Rect_Outline(draw_buffer, r, piece.color.xyz, 1.f);
      }
      else
      {
        Draw_Rect(draw_buffer, r, piece.color.rgb);
      }
    }
  }

  // Draw_BMP(draw_buffer, &game_state->test_bmp, 10, 10, 2);
  // Draw_BMP(draw_buffer, &game_state->test_bmp, 20, 20, 2);
  Draw_Inputs(draw_buffer, input);
  End_Sim(sim_region, game_state);
  End_Temp_Memory(&transient_state->transient_arena, sim_memory);
  End_Temp_Memory(&transient_state->transient_arena, render_memory);

  Check_Arena(&game_state->world_arena);
  Check_Arena(&transient_state->transient_arena);
  return;
}

extern "C" GAME_GET_SOUND_SAMPLES(Game_Get_Sound_Samples)
{
  Game_State* game_state = (Game_State*)memory->permanent_storage;
  // TODO: Allow sample offests for more robust platform options
  Game_Output_Sound(game_state, sound_buffer);
}
