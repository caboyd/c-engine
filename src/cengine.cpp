
#undef CLANGD
#include "cengine.h"
#include "hot.h"
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

internal void Draw_BMP_Subset(Game_Offscreen_Buffer* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, S32 bmp_x_offset,
                              S32 bmp_y_offset, S32 bmp_x_dim, S32 bmp_y_dim, F32 scale, B32 alpha_blend, F32 c_alpha)
{
#if 1
  Draw_BMP_Subset_Hot(buffer, bitmap, x, y, bmp_x_offset, bmp_y_offset, bmp_x_dim, bmp_y_dim, scale, alpha_blend,
                      c_alpha);
#else
  c_alpha = CLAMP(c_alpha, 0.f, 1.f);
  if (!bitmap || !bitmap->pixels)
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

  U8* dest_row_in_bytes = (U8*)buffer->memory + (min_y * buffer->pitch_in_bytes) + (min_x * buffer->bytes_per_pixel);
  F32 epsilon = 0.0001f;
  for (S32 y_index = min_y; y_index < max_y; y_index++)
  {
    U8* pixel = dest_row_in_bytes;
    S32 y_src_offset = Trunc_F32_S32((F32)(y_index - min_y + y_draw_offset) / scale - epsilon);
    // NOTE: flip the bmp to render into buffer top to bottom
    S32 y_src = (bitmap->height - 1) - y_src_offset;

    for (S32 x_index = min_x; x_index < max_x; x_index++)
    {
      S32 x_src = Trunc_F32_S32((F32)(x_index - min_x + x_draw_offset) / scale - epsilon);
      ASSERT(x_src < bitmap->width);
      ASSERT(y_src < bitmap->height);

      U8* src = (U8*)(void*)(bitmap->pixels + y_src * bitmap->width + x_src);
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
internal void Draw_Sprite_Sheet_Sprite(Game_Offscreen_Buffer* buffer, Sprite_Sheet* sprite_sheet, U32 sprite_index,
                                       F32 x, F32 y, F32 scale, B32 alpha_blend = false, F32 c_alpha = 1.0f)
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

internal void Draw_BMP(Game_Offscreen_Buffer* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, F32 scale,
                       B32 alpha_blend = false, F32 c_alpha = 1.0f)
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

internal void Draw_Rectf(Game_Offscreen_Buffer* buffer, F32 fmin_x, F32 fmin_y, F32 fmax_x, F32 fmax_y, F32 r, F32 g,
                         F32 b, B32 wire_frame = false)
{

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

  U8* row_in_bytes = (U8*)buffer->memory + (min_y * buffer->pitch_in_bytes) + (min_x * buffer->bytes_per_pixel);

  for (S32 y = min_y; y < max_y; y++)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;
    for (S32 x = min_x; x < max_x; x++)
    {
      if (wire_frame)
      {
        if ((y == min_y || y == max_y - 1) || (x == min_x || x == max_x - 1))
        {
          *pixel++ = color;
        }
        else
        {
          pixel++;
        }
      }
      else
      {
        *pixel++ = color;
      }
    }
    row_in_bytes += buffer->pitch_in_bytes;
  }
}
internal void Draw_Rect(Game_Offscreen_Buffer* buffer, Rect2 rect, F32 r, F32 g, F32 b, B32 wire_frame = false)
{
  Draw_Rectf(buffer, rect.min.x, rect.min.y, rect.max.x, rect.max.y, r, g, b, wire_frame);
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
internal void Draw_Inputs(Game_Offscreen_Buffer* buffer, Game_Input* input)
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
  F32 mouse_x = (F32)input->mouse_x;
  F32 mouse_y = (F32)input->mouse_y;
  Draw_Rectf(buffer, mouse_x, mouse_y, mouse_x + 4.0f, mouse_y + 4.0f, 0.f, 1.f, 0.f, true);
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
    result.pixels = Push_Array(arena, (U64)pixels_count, U32);

    Memory_Copy(result.pixels, pixels, (pixels_count * (S32)sizeof(U32)));

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

    S32 blue_shift = 0 - (S32)blue_scan.index;
    S32 green_shift = 8 - (S32)green_scan.index;
    S32 red_shift = 16 - (S32)red_scan.index;
    S32 alpha_shift = 24 - (S32)alpha_scan.index;

    bool already_argb =
        (blue_scan.index == 0 && green_scan.index == 8 && red_scan.index == 16 && alpha_scan.index == 24);

    if (!already_argb)
    {
      U32* p = result.pixels;
      for (S32 i = 0; i < pixels_count; ++i)
      {
        p[i] = Rotate_Left(p[i] & header->BlueMask, blue_shift) | Rotate_Left(p[i] & header->GreenMask, green_shift) |
               Rotate_Left(p[i] & header->RedMask, red_shift) | Rotate_Left(p[i] & header->AlphaMask, alpha_shift);
      }
    }
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

inline Vec2 Get_Camera_Space_Pos(Game_State* game_state, Low_Entity* entity_low)
{
  World_Diff diff = World_Pos_Subtract(game_state->world, &entity_low->pos, &game_state->camera_pos);
  Vec2 result = diff.dxy;
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

  Change_Entity_Location(&game_state->world_arena, game_state->world, entity_index, entity_low, pos);

  Add_Low_Entity_Result result;
  result.low = entity_low;
  result.low_index = entity_index;

  return result;
}
internal Add_Low_Entity_Result Add_Sword(Game_State* game_state)

{
  Add_Low_Entity_Result entity = Add_Low_Entity(game_state, ENTITY_TYPE_SWORD, Null_Position());

  entity.low->sim.height = 0.5f;
  entity.low->sim.width = 0.5f;

  return entity;
}

internal Add_Low_Entity_Result Add_Player(Game_State* game_state)
{
  World_Position pos = game_state->camera_pos;
  Add_Low_Entity_Result entity = Add_Low_Entity(game_state, ENTITY_TYPE_PLAYER, pos);

  entity.low->sim.max_health = 25;
  entity.low->sim.health = entity.low->sim.max_health;

  entity.low->sim.height = 0.4f;
  entity.low->sim.width = 0.8f;
  Set_Flag(&entity.low->sim, ENTITY_FLAG_COLLIDES);

  Add_Low_Entity_Result sword = Add_Sword(game_state);
  entity.low->sim.sword.index = sword.low_index;

  if (game_state->camera_follow_entity_index == 0)
  {
    game_state->camera_follow_entity_index = entity.low_index;
  }
  return entity;
}

internal Add_Low_Entity_Result Add_Wall(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)
{
  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity = Add_Low_Entity(game_state, ENTITY_TYPE_WALL, pos);

  entity.low->sim.height = game_state->world->tile_size_in_meters;
  entity.low->sim.width = entity.low->sim.height;
  Set_Flag(&entity.low->sim, ENTITY_FLAG_COLLIDES);

  return entity;
}
internal Add_Low_Entity_Result Add_Stair(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z,
                                         S32 delta_z)
{
  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity = Add_Low_Entity(game_state, ENTITY_TYPE_STAIR, pos);

  entity.low->sim.height = game_state->world->tile_size_in_meters;
  entity.low->sim.width = entity.low->sim.height;
  // Set_Flag(&entity.low->sim, ENTITY_FLAG_COLLIDES);
  entity.low->sim.delta_abs_tile_z = delta_z;

  return entity;
}

internal U32 Add_Monster(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)

{
  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity = Add_Low_Entity(game_state, ENTITY_TYPE_MONSTER, pos);

  entity.low->sim.max_health = 30;
  entity.low->sim.health = entity.low->sim.max_health;

  entity.low->sim.height = 0.5f;
  entity.low->sim.width = 0.95f;
  Set_Flag(&entity.low->sim, ENTITY_FLAG_COLLIDES);

  return entity.low_index;
}
internal U32 Add_Familiar(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)

{
  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity = Add_Low_Entity(game_state, ENTITY_TYPE_FAMILIAR, pos);

  entity.low->sim.max_health = 12;
  entity.low->sim.health = entity.low->sim.max_health;

  entity.low->sim.height = 0.3f;
  entity.low->sim.width = 0.3f;

  return entity.low_index;
}

internal inline void Add_Render_Piece(Entity_Render_Group* group, Loaded_Bitmap* bitmap, Vec2 bmp_inner_offset,
                                      Vec2 dim, Vec2 offset, F32 offset_z, Vec2 align, F32 scale, Vec4 color,
                                      F32 entity_cz)
{
  ASSERT(group->count < Array_Count(group->pieces));
  Entity_Render_Piece* piece = &group->pieces[group->count++];
  piece->bitmap = bitmap;
  piece->bmp_offset_x = (S32)bmp_inner_offset.x;
  piece->bmp_offset_y = (S32)bmp_inner_offset.y;
  piece->dim = dim;
  piece->offset = ((offset * group->game_state->meters_to_pixels) - align);
  piece->offset_z = offset_z * group->game_state->meters_to_pixels;
  piece->scale = scale;
  piece->color = color;
  piece->entity_cz = entity_cz;
}

internal inline void Add_Sprite_Render_Piece(Entity_Render_Group* group, Sprite_Sheet* sprite_sheet, U32 sprite_index,
                                             Vec2 offset, F32 offset_z, Vec2 align, F32 scale, F32 alpha = 1.f,
                                             F32 entity_cz = 1.f)
{
  Sprite sprite = sprite_sheet->sprites[sprite_index];
  Add_Render_Piece(group, &sprite_sheet->bitmap, {{(F32)sprite.x, (F32)sprite.y}},
                   {{(F32)sprite.width, (F32)sprite.height}}, offset, offset_z, align, scale, {{0, 0, 0, alpha}},
                   entity_cz);
}
internal inline void Add_Bitmap_Render_Piece(Entity_Render_Group* group, Loaded_Bitmap* bitmap, Vec2 offset,
                                             F32 offset_z, Vec2 align, F32 scale, F32 alpha = 1.f, F32 entity_cz = 1.f)
{
  Add_Render_Piece(group, bitmap, vec2(0.f, 0.f), {{(F32)bitmap->width, (F32)bitmap->height}}, offset, offset_z, align,
                   scale, {{0, 0, 0, alpha}}, entity_cz);
}
internal inline void Add_Rect_Render_Piece(Entity_Render_Group* group, Vec2 align, Vec2 dim, F32 scale, F32 r, F32 g,
                                           F32 b, F32 entity_cz = 1.f)
{
  Add_Render_Piece(group, NULL, vec2(0.f, 0.f), dim, vec2(0.f, 0.f), 0, align, scale, {{r, g, b, 1}}, entity_cz);
}
internal inline void Draw_Health(Entity_Render_Group* group, Sim_Entity* entity, F32 x_scale)

{
  Vec2 scale = vec2(x_scale, 1.f);
  F32 health_ratio = (F32)entity->health / (F32)entity->max_health;

  Vec2 max_hp_pos = vec2(8, 16) * scale;
  Vec2 max_hp_bar = vec2(16, 2) * scale;

  Vec2 hp_pos = vec2(max_hp_pos.x, max_hp_pos.y);
  Vec2 hp_bar = vec2(max_hp_bar.x * health_ratio, max_hp_bar.y) * scale;

  // red hp background
  Add_Rect_Render_Piece(group, max_hp_pos, max_hp_bar, 1.f, 0.8f, 0.2f, 0.2f);
  // green hp
  Add_Rect_Render_Piece(group, hp_pos, hp_bar, 1.f, 0.2f, 0.8f, 0.2f);
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

internal void Add_Collision_Rule(Game_State* game_state, U32 storage_index_a, U32 storage_index_b, B32 should_collide)
{
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
  }

  if (found)
  {
    found->storage_index_a = storage_index_a;
    found->storage_index_b = storage_index_b;
    found->should_collide = should_collide;
  }
}
extern "C" GAME_UPDATE_AND_RENDER(Game_Update_And_Render)
{
  ASSERT((&input->controllers[0].button_count - &input->controllers[0].buttons[0]) ==
         (Array_Count(input->controllers[0].buttons)));
  ASSERT(sizeof(Game_State) < memory->permanent_storage_size);

  Game_State* game_state = (Game_State*)memory->permanent_storage;
  if (!memory->is_initialized)
  {
    // NOTE: first entity is NULL entity
    Add_Low_Entity(game_state, ENTITY_TYPE_NULL, Null_Position());

    Initialize_Arena(&game_state->world_arena, memory->permanent_storage_size - sizeof(Game_State),
                     (U8*)memory->permanent_storage + sizeof(Game_State));
    Arena* world_arena = &game_state->world_arena;

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

    game_state->world = Push_Struct_Align(world_arena, World, 8);

    World* world = game_state->world;

    Initialize_World(world, TILE_SIZE_IN_METERS);
    F32 tile_size_in_pixels = TILE_SIZE_IN_PIXELS;
    game_state->sprite_scale = 4.f * ((F32)tile_size_in_pixels / 64.f);
    game_state->meters_to_pixels = (F32)tile_size_in_pixels / world->tile_size_in_meters;

    S32 screen_base_x = 0;
    S32 screen_base_y = 0;
    S32 screen_base_z = 0;

    S32 screen_x = screen_base_x;
    S32 screen_y = screen_base_y;
    S32 abs_tile_z = screen_base_z;

    U32 random_index = 0;
    B32 door_left = false;
    B32 door_right = false;
    B32 door_top = false;
    B32 door_bottom = false;
    B32 door_up = false;
    B32 door_down = false;

    for (U32 screen_index = 0; screen_index < 2000; ++screen_index)
    {

      ASSERT(random_index < Array_Count(random_number_table));
      U32 random_choice;
      if (door_up || door_down)
      {
        random_choice = random_number_table[random_index++] % 2;
      }
      else
      {
        random_choice = random_number_table[random_index++] % 3;
      }

      if (random_choice == 2)
      {
        if (abs_tile_z == 0)
        {
          door_up = true;
        }
        else
        {
          door_down = true;
        }
      }
      else if (random_choice == 1)
      {
        door_right = true;
      }
      else
      {
        door_top = true;
      }

      for (S32 tile_y = 0; tile_y < TILES_PER_HEIGHT; ++tile_y)
      {
        for (S32 tile_x = 0; tile_x < TILES_PER_WIDTH; ++tile_x)
        {
          S32 abs_tile_x = screen_x * TILES_PER_WIDTH + tile_x;
          S32 abs_tile_y = screen_y * TILES_PER_HEIGHT + tile_y;

          U32 tile_value = 1;
          if ((tile_x == 0) && (!door_left || (tile_y != TILES_PER_HEIGHT / 2)))
          {
            tile_value = 2;
          }
          if ((tile_x == (TILES_PER_WIDTH - 1)) && (!door_right || (tile_y != TILES_PER_HEIGHT / 2)))
          {
            tile_value = 2;
          }

          if ((tile_y == 0) && (!door_bottom || (tile_x != TILES_PER_WIDTH / 2)))
          {
            tile_value = 2;
          }
          if ((tile_y == (TILES_PER_HEIGHT - 1)) && (!door_top || (tile_x != TILES_PER_WIDTH / 2)))
          {
            tile_value = 2;
          }
          if ((tile_x == 10) && (tile_y == 6))
          {
            if (door_up)
            {
              tile_value = 3;
            }
            if (door_down)
            {
              tile_value = 4;
            }
          }
          // Set_Tile_Value(&game_state->permananent_arena, world->tile_map, abs_tile_x, abs_tile_y, abs_tile_z,
          //                tile_value);

          if (tile_value == 2)
          {
            Add_Wall(game_state, abs_tile_x, abs_tile_y, abs_tile_z);
          }
          else if (tile_value == 3)
          {
            Add_Stair(game_state, abs_tile_x, abs_tile_y, abs_tile_z, 1);
          }
          else if (tile_value == 4)
          {
            Add_Stair(game_state, abs_tile_x, abs_tile_y, abs_tile_z, -1);
          }
        }
      }
      door_left = door_right;
      door_bottom = door_top;
      door_right = false;
      door_top = false;
      if (random_choice == 2)
      {
        door_down = !door_down;
        door_up = !door_up;
      }
      else
      {
        door_up = door_down = false;
      }

      if (random_choice == 2)
      {
        if (abs_tile_z == 0)
        {
          abs_tile_z = 1;
        }
        else
        {
          abs_tile_z = 0;
        }
      }
      else if (random_choice == 1)

      {
        screen_x += 1;
      }
      else
      {
        screen_y += 1;
      }
    }
    // while (game_state->low_entity_count < (Array_Count(game_state->low_entities) - 16))
    // {
    //   S32 coord = 1024 + (S32)game_state->low_entity_count;
    //   Add_Wall(game_state, coord, coord, coord);
    // }

    World_Position new_camera_pos = {};
    S32 camera_tile_x = screen_base_x * TILES_PER_WIDTH + 17 / 2;
    S32 camera_tile_y = screen_base_y * TILES_PER_HEIGHT + 9 / 2;
    S32 camera_tile_z = screen_base_z;

    new_camera_pos = Chunk_Position_From_Tile_Position(world, camera_tile_x, camera_tile_y, camera_tile_z);

    Add_Monster(game_state, camera_tile_x + 2, camera_tile_y + 2, camera_tile_z);
    for (S32 y = camera_tile_y - 3; y < camera_tile_y + 3; ++y)
    {
      for (S32 x = camera_tile_x - 4; x < camera_tile_x + 4; ++x)
      {
        Add_Familiar(game_state, x, y, camera_tile_z);
      }
    }

    game_state->camera_pos = new_camera_pos;

    memory->is_initialized = true;
  }
  F32 delta_time = input->delta_time_s;

  S32 tile_size_in_pixels = TILE_SIZE_IN_PIXELS;
  F32 sprite_scale = game_state->sprite_scale;
  F32 meters_to_pixels = game_state->meters_to_pixels;

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
        con_player->jump_vel = 4.f;
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

  S32 tile_span_x = TILES_PER_WIDTH * 3;
  S32 tile_span_y = TILES_PER_HEIGHT * 3;
  Rect2 camera_bounds =
      Rect_Center_Dim(Vec2{{0, 0}}, Vec2{{(F32)tile_span_x, (F32)tile_span_y}} * world->tile_size_in_meters);

  Arena sim_arena;
  Initialize_Arena(&sim_arena, memory->transient_storage_size, memory->transient_storage);
  Sim_Region* sim_region = Begin_Sim(&sim_arena, game_state, game_state->world, game_state->camera_pos, camera_bounds);

  // NOTE: Clear Buffer --------------------------------------------------------
  Draw_Rectf(buffer, 0, 0, (F32)buffer->width, (F32)buffer->height, 0.35f, 0.58f, 0.93f);
  //------------------------------------
  //

  // NOTE: Draw Tile map floor

  Vec2 screen_center = Vec2{{0.5f * (F32)buffer->width, 0.5f * (F32)buffer->height}};
  for (S32 rel_row = -10; rel_row < 10; rel_row++)
  {
    for (S32 rel_col = -20; rel_col < 20; rel_col++)
    {
      // NOTE: for stable tile floors that keep same tile visually when camera jumps screens
      S32 col = game_state->camera_pos.chunk_x * TILES_PER_CHUNK +
                Round_F32_S32(game_state->camera_pos.offset_.x / TILE_SIZE_IN_METERS) + rel_col;
      S32 row = game_state->camera_pos.chunk_y * TILES_PER_CHUNK +
                Round_F32_S32(game_state->camera_pos.offset_.y / TILE_SIZE_IN_METERS) + rel_row;
      U32 tile = 1;
      if (tile != 0)
      {

        Vec2 center = {{screen_center.x + ((F32)(rel_col) * (F32)tile_size_in_pixels),
                        screen_center.y - ((F32)(rel_row) * (F32)tile_size_in_pixels)}};
        Vec2 tile_size_pixels = {{(F32)tile_size_in_pixels, (F32)tile_size_in_pixels}};

        Vec2 min = center - 0.5f * tile_size_pixels;
        // Vec2 max = center + 0.5f * tile_size_pixels;

        if (game_state->camera_pos.chunk_z == 0)
        {
          U32 grass_sprite_index = ((U32)col % 6 + (U32)row % 5) % 4;
          Draw_Sprite_Sheet_Sprite(buffer, &game_state->grass_sprite_sheet, grass_sprite_index, min.x, min.y,
                                   game_state->sprite_scale, false);
        }
        else
        {
          Draw_BMP(buffer, &game_state->stone_floor_bmp, min.x, min.y, game_state->sprite_scale, false);
        }
      }
    }
  }

  // NOTE:Draw entities

  Vec2 shadow_align = vec2(6, 4);
  Vec2 sprite_align = vec2(8, 8);
  Vec2 unit_align = vec2(8, 14);
  Vec2 sprite1x2_align = vec2(8, 24);
  Entity_Render_Group render_group = {};
  render_group.game_state = game_state;
  S32 camera_z = game_state->camera_pos.chunk_z;

  Sim_Entity* entity = sim_region->entities;
  for (U32 entity_index = 0; entity_index < sim_region->entity_count; ++entity_index, ++entity)
  {
    render_group.count = 0;
    if (!entity->updatable)
    {
      continue;
    }

    // TODO: bad design, this should be computed after update
    F32 shadow_alpha = CLAMP((1.0f - entity->z), 0.5f, 0.8f);
    F32 shadow_scale = shadow_alpha * 0.75f;

    Move_Spec move_spec = Default_Move_Spec();
    Vec2 accel = {};

    switch (entity->type)
    {
      case ENTITY_TYPE_PLAYER:
      {
        for (U32 control_index = 0; control_index < Array_Count(game_state->controlled_players); ++control_index)
        {
          Controlled_Player* con_player = game_state->controlled_players + control_index;
          if (entity->storage_index == con_player->entity_index)
          {
            if (entity->z <= 0.f)
            {
              entity->vel_z = con_player->jump_vel;
            }

            move_spec.normalize_accel = true;
            move_spec.drag = 8.f;
            move_spec.speed = 50.f;
            move_spec.speed *= con_player->sprint ? 3.f : 1.f;
            accel = con_player->move_dir;

            if (Vec2_Length_Sq(con_player->attack_dir) > 0.f)
            {
              Sim_Entity* sword = entity->sword.ptr;
              if (sword && Has_Flag(sword, ENTITY_FLAG_NONSPATIAL))
              {
                sword->pos = entity->pos;
                sword->distance_limit = 5.f;
                Vec2 vel = 10.f * con_player->attack_dir;
                Make_Entity_Spatial(sword, entity->pos, vel);
                Add_Collision_Rule(game_state, entity->storage_index, sword->storage_index, false);
              }
            }
          }
        }

        Loaded_Bitmap* bmp = &game_state->shadow_bmp;
        Add_Bitmap_Render_Piece(&render_group, bmp, vec2(0.f, 0.f), 0, shadow_align, shadow_scale, shadow_alpha, 0.f);
        Add_Sprite_Render_Piece(&render_group, &game_state->knight_sprite_sheet, entity->sprite_index, vec2(0.f, 0.f),
                                0, unit_align, 1.f, 1.f);

        Draw_Health(&render_group, entity, 1.f);
      }
      break;
      case ENTITY_TYPE_MONSTER:
      {

        Loaded_Bitmap* bmp = &game_state->shadow_bmp;
        Add_Bitmap_Render_Piece(&render_group, bmp, vec2(0.f, 0.f), 0, shadow_align, shadow_scale * 1.5f, shadow_alpha,
                                0.f);

        Add_Sprite_Render_Piece(&render_group, &game_state->slime_sprite_sheet, entity->sprite_index, vec2(0.f, 0.f), 0,
                                unit_align, 1.f, 1.f);
        Draw_Health(&render_group, entity, 1.f);
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
            F32 test_distsq = Vec2_Length_Sq(test_entity->pos - entity->pos);
            if (test_distsq < closest_player_distsq)
            {
              closest_player_distsq = test_distsq;
              closest_player = test_entity;
            }
          }
        }

        // NOTE: follow player
        if (closest_player && (closest_player_distsq > Square(1.5f)))
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
        F32 bob_sin = (0.5f * sinf(entity->bob_time) + 1.f);

        F32 entity_scale_mod = 0.5f;
        shadow_alpha = 0.3f * shadow_alpha + 0.25f * bob_sin;
        shadow_scale = (entity_scale_mod * 0.8f) * shadow_scale + 0.1f * bob_sin;
        Loaded_Bitmap* bmp = &game_state->shadow_bmp;
        Add_Bitmap_Render_Piece(&render_group, bmp, vec2(0.f, 0.f), 0, shadow_align, shadow_scale, shadow_alpha, 0.f);

        Add_Sprite_Render_Piece(&render_group, &game_state->eye_sprite_sheet, entity->sprite_index, vec2(0.f, 0.f),
                                0.5f * bob_sin - 1, unit_align, entity_scale_mod, 1.f);

        Draw_Health(&render_group, entity, entity_scale_mod);
      }
      break;
      case ENTITY_TYPE_SWORD:
      {

        if (entity->distance_limit <= 0.f)
        {
          Make_Entity_Nonspatial(entity);
          Clear_Collision_Rules_For(game_state, entity->storage_index);
        }

        F32 entity_scale_mod = 0.5f;
        shadow_scale *= entity_scale_mod;
        shadow_alpha *= entity_scale_mod;

        Add_Bitmap_Render_Piece(&render_group, &game_state->shadow_bmp, vec2(0.f, 1.f), 0, shadow_align, shadow_scale,
                                shadow_alpha, 0.f);
        Add_Bitmap_Render_Piece(&render_group, &game_state->shuriken_bmp, vec2(0.f, 0.f), 0, sprite_align,
                                entity_scale_mod, 1.f);
      }
      break;
      case ENTITY_TYPE_WALL:
      {
        if (camera_z == 0)
        {
          // Add_Bitmap_Render_Piece(&game_state->wall1_bmp, {{0.f, 0.f}}, 0, 1.f, 1.f);
          Add_Bitmap_Render_Piece(&render_group, &game_state->pillar_bmp, vec2(0.f, 0.f), 0, sprite1x2_align, 1.f, 1.f);
        }
        else if (camera_z == 1)
        {
          // Add_Bitmap_Render_Piece(&game_state->wall2_bmp, {{0.f, 0.f}}, 0, 1.f, 1.f);
          Add_Bitmap_Render_Piece(&render_group, &game_state->pillar_bmp, vec2(0.f, 0.f), 0, sprite1x2_align, 1.f, 1.f);
        }
      }
      break;
      case ENTITY_TYPE_STAIR:
      {
        if (camera_z == 0)
        {
          Add_Bitmap_Render_Piece(&render_group, &game_state->stair_down_bmp, vec2(0.f, 0.f), 0, sprite_align, 1.f,
                                  1.f);
        }
        else if (camera_z == 1)
        {
          Add_Bitmap_Render_Piece(&render_group, &game_state->stair_up_bmp, vec2(0.f, 0.f), 0, sprite_align, 1.f, 1.f);
        }
      }
      break;
      case ENTITY_TYPE_NULL:
      default:
      {
        Invalid_Code_Path;
      };
    }
    if (!Has_Flag(entity, ENTITY_FLAG_NONSPATIAL) && (Vec2_Length_Sq(accel) > 0.f || Vec2_Length_Sq(entity->vel) > 0.f))
    {
      Move_Entity(game_state, sim_region, entity, delta_time, &move_spec, accel);
    }
    Vec2 entity_pos = {
        {screen_center.x + meters_to_pixels * entity->pos.x, screen_center.y - meters_to_pixels * entity->pos.y}};

    F32 z = -meters_to_pixels * entity->z;

    // NOTE: Draw entity

    for (U32 piece_index = 0; piece_index < render_group.count; ++piece_index)
    {
      Entity_Render_Piece piece = render_group.pieces[piece_index];

      F32 x = entity_pos.x + piece.offset.x * sprite_scale * piece.scale;
      F32 y = entity_pos.y + (piece.offset.y + piece.offset_z) * sprite_scale * piece.scale + (piece.entity_cz * z);

      if (piece.bitmap)
      {
        Draw_BMP_Subset(buffer, piece.bitmap, x, y, piece.bmp_offset_x, piece.bmp_offset_y, (S32)piece.dim.x,
                        (S32)piece.dim.y, sprite_scale * piece.scale, true, piece.color.a);
      }
      else
      {
        Rect2 r = Rect_Min_Dim({{x, y}}, piece.dim * sprite_scale * piece.scale);
        Draw_Rect(buffer, r, piece.color.r, piece.color.g, piece.color.b);
      }
    }
    // NOTE: draw collision bounds

    Rect2 r = Rect_Center_Dim(entity_pos, Vec2{{entity->width, entity->height}} * (F32)meters_to_pixels);
    if (Has_Flag(entity, ENTITY_FLAG_COLLIDES))
    {
      Draw_Rect(buffer, r, 0.0f, 1.f, 0.8f, true);
    }
    else
    {
      // Draw_Rect(buffer, r, 0.8f, 1.0f, 0.8f, true);
    }
  }

  // Draw_BMP(buffer, &game_state->test_bmp, 10, 10, 2);
  // Draw_BMP(buffer, &game_state->test_bmp, 20, 20, 2);
  Draw_Inputs(buffer, input);
  End_Sim(sim_region, game_state);
  return;
}

extern "C" GAME_GET_SOUND_SAMPLES(Game_Get_Sound_Samples)
{
  Game_State* game_state = (Game_State*)memory->permanent_storage;
  // TODO: Allow sample offests for more robust platform options
  Game_Output_Sound(game_state, sound_buffer);
}
