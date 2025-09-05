
#include "cengine.h"
#include "tile.cpp"

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
                              S32 bmp_y_offset, S32 bmp_x_dim, S32 bmp_y_dim, F32 scale, B32 alpha_blend)
{
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

  for (S32 y_index = min_y; y_index < max_y; y_index++)
  {
    U8* pixel = dest_row_in_bytes;
    S32 y_src_offset = Trunc_F32_S32((F32)(y_index - min_y + y_draw_offset) / scale);
    // NOTE: flip the bmp to render into buffer top to bottom
    S32 y_src = (bitmap->height - 1) - y_src_offset;

    for (S32 x_index = min_x; x_index < max_x; x_index++)
    {
      S32 x_src = Trunc_F32_S32((F32)(x_index - min_x + x_draw_offset) / scale);
      ASSERT(x_src < bitmap->width);
      ASSERT(y_src < bitmap->height);

      U8* src = (U8*)(void*)(bitmap->pixels + y_src * bitmap->width + x_src);
      Color4 out = *(Color4*)(void*)src;
      if (alpha_blend)
      {
        out = blend_normal_Color4(*(Color4*)(void*)pixel, *(Color4*)(void*)src);
      }

      // Note: BMP may not be 4 byte aligned so assign byte by byte
      *pixel++ = out.argb.b; // B
      *pixel++ = out.argb.g; // G
      *pixel++ = out.argb.r; // R
      *pixel++ = out.argb.a; // A
    }
    dest_row_in_bytes += buffer->pitch_in_bytes;
  }
}
internal void Draw_Sprite_Sheet_Sprite(Game_Offscreen_Buffer* buffer, Sprite_Sheet* sprite_sheet, S32 sprite_index,
                                       F32 x, F32 y, F32 scale, B32 alpha_blend)
{
  Sprite sprite = sprite_sheet->sprites[sprite_index];

  Draw_BMP_Subset(buffer, &sprite_sheet->bitmap, x, y, sprite.x, sprite.y, sprite.width, sprite.height, scale,
                  alpha_blend);
}
internal void Draw_Player_Sprite(Game_Offscreen_Buffer* buffer, Entity_Sprite* entity_sprite, F32 x, F32 y, F32 scale,
                                 B32 alpha_blend)
{
  Sprite sprite = entity_sprite->sprite_sheet->sprites[entity_sprite->sprite_index];

  Draw_BMP_Subset(buffer, &entity_sprite->sprite_sheet->bitmap, x + (scale * (F32)entity_sprite->align_x),
                  y + (scale * (F32)entity_sprite->align_y), sprite.x, sprite.y, sprite.width, sprite.height, scale,
                  alpha_blend);
}

internal void Draw_BMP(Game_Offscreen_Buffer* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, F32 scale, B32 alpha_blend)
{
  Draw_BMP_Subset(buffer, bitmap, x, y, 0, 0, bitmap->width, bitmap->height, scale, alpha_blend);
}

internal void Draw_Rectf(Game_Offscreen_Buffer* buffer, F32 fmin_x, F32 fmin_y, F32 fmax_x, F32 fmax_y, F32 r, F32 g,
                         F32 b)
{
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
      *pixel++ = color;
    }
    row_in_bytes += buffer->pitch_in_bytes;
  }
}
internal void Draw_Rect(Game_Offscreen_Buffer* buffer, Vec2 min, Vec2 max, F32 r, F32 g, F32 b)
{
  Draw_Rectf(buffer, min.x, min.y, max.y, max.y, r, g, b);
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
  Draw_Rectf(buffer, mouse_x, mouse_y, mouse_x + 5.0f, mouse_y + 5.0f, 0.f, 1.f, 0.f);
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
internal Entity* Get_Entity(Game_State* game_state, U32 entity_index)
{
  Entity* entity = 0;
  ASSERT(entity_index < Array_Count(game_state->entities));
  if (entity_index > 0 && entity_index < Array_Count(game_state->entities))
  {
    entity = &game_state->entities[entity_index];
  }
  return entity;
}
internal void Test_Wall(F32 wall_x, F32 rel_x, F32 rel_y, F32 player_delta_x, F32 player_delta_y, F32* t_min, F32 min_y,
                        F32 max_y)

{
  F32 t_epsilon = 0.01f;
  if (player_delta_x != 0.0f)
  {
    F32 t_result = (wall_x - rel_x) / player_delta_x;
    F32 y = rel_y + t_result * player_delta_y;
    if ((t_result >= 0.0f) && (*t_min > t_result))
    {
      if ((y > min_y) && (y <= max_y))
      {
        *t_min = MAX(0.0f, t_result - t_epsilon);
      }
    }
  }
}
internal void Move_Player(Game_State* game_state, Entity* player, F32 delta_time, Vec2 player_accel, B32 sprint)

{
  Tile_Map* tile_map = game_state->world->tile_map;

  F32 player_speed = 50.f;

  if (sprint)
  {
    player_speed *= 3.f;
  }

  if (Vec2_Length_Sq(player_accel) > 0)
  {
    player_accel = Vec2_Normalize(player_accel);
  }
  player_accel *= player_speed;
  // Friction
  player_accel += player->vel * -8.f;

  // NOTE: analytical closed-form kinematic equation
  //  new_pos = (0.5f * accel * dt^2) + vel * dt + old_pos
  Vec2 player_delta = player_accel * 0.5f * Square(delta_time) + player->vel * delta_time;
  player->vel = player->vel + player_accel * delta_time;

  Tile_Map_Position old_player_p = player->pos;
  Tile_Map_Position new_player_p = player->pos;
  new_player_p.offset += player_delta;
  new_player_p = RecanonicalizePosition(tile_map, new_player_p);
#if 0
  // TODO: Delta function that recanonicalizes
  // NOTE: Check if we can move player
  F32 player_half_width = player->width / 2.f;
  Tile_Map_Position player_bottom_left = new_player_p;
  player_bottom_left.offset.x -= player_half_width;
  player_bottom_left = RecanonicalizePosition(tile_map, player_bottom_left);

  Tile_Map_Position player_bottom_right = new_player_p;
  player_bottom_right.offset.x += player_half_width;
  player_bottom_right = RecanonicalizePosition(tile_map, player_bottom_right);

  B32 collided = false;
  B32 both_collided = false;
  Tile_Map_Position collided_p;
  if (!Is_Tile_Map_Position_Empty(tile_map, player_bottom_left))
  {
    collided_p = player_bottom_left;
    collided = true;
  }
  if (!Is_Tile_Map_Position_Empty(tile_map, player_bottom_right))
  {
    collided_p = player_bottom_right;
    if (collided)
    {
      both_collided = true;
    }
    collided = true;
  }

  if (collided)
  {
    Vec2 wall_normal = {};
    if (both_collided)
    {
      if (collided_p.abs_tile_y < player->pos.abs_tile_y)
      {
        // hit south wall
        wall_normal = {{0.f, -1.f}};
      }
      else if (collided_p.abs_tile_y > player->pos.abs_tile_y)
      {
        // hit north wall
        wall_normal = {{0.f, 1.f}};
      }
    }
    else
    {
      if (collided_p.abs_tile_x < player->pos.abs_tile_x)
      {
        // hit left wall
        wall_normal = {{-1.f, 0.f}};
      }
      else if (collided_p.abs_tile_x > player->pos.abs_tile_x)
      {
        // hit right wall
        wall_normal = {{1.f, 0.f}};
      }
    }
    player->vel = Vec2_SlideSafe(player->vel, wall_normal);
  }
  else
  {
    player->pos = new_player_p;
  }
#else
  U32 min_tile_x = MIN(old_player_p.abs_tile_x, new_player_p.abs_tile_x);
  U32 min_tile_y = MIN(old_player_p.abs_tile_y, new_player_p.abs_tile_y);
  U32 one_past_max_tile_x = MAX(old_player_p.abs_tile_x, new_player_p.abs_tile_x) + 1;
  U32 one_past_max_tile_y = MAX(old_player_p.abs_tile_y, new_player_p.abs_tile_y) + 1;
  U32 abs_tile_z = player->pos.abs_tile_z;
  F32 t_min = 1.0f;
  for (U32 abs_tile_y = min_tile_y; abs_tile_y != one_past_max_tile_y; ++abs_tile_y)
  {
    for (U32 abs_tile_x = min_tile_x; abs_tile_x != one_past_max_tile_x; ++abs_tile_x)

    {
      Tile_Map_Position test_tile_p = Centered_Tile_Point(abs_tile_x, abs_tile_y, abs_tile_z);
      U32 tile_value = Get_Tile_Value(tile_map, test_tile_p);
      if (!Is_Tile_Value_Empty(tile_value))
      {
        Vec2 min_corner = -0.5f * Vec2{{tile_map->tile_size_in_meters, tile_map->tile_size_in_meters}};
        Vec2 max_corner = 0.5f * Vec2{{tile_map->tile_size_in_meters, tile_map->tile_size_in_meters}};

        Tile_Map_Diff rel_new_player_p = Tile_Map_Pos_Subtract(tile_map, &old_player_p, &test_tile_p);
        Vec2 rel = rel_new_player_p.dxy;
        Test_Wall(min_corner.x, rel.x, rel.y, player_delta.x, player_delta.y, &t_min, min_corner.y, max_corner.y);
        Test_Wall(max_corner.x, rel.x, rel.y, player_delta.x, player_delta.y, &t_min, min_corner.y, max_corner.y);
        Test_Wall(min_corner.y, rel.y, rel.x, player_delta.y, player_delta.x, &t_min, min_corner.x, max_corner.x);
        Test_Wall(max_corner.y, rel.y, rel.x, player_delta.y, player_delta.x, &t_min, min_corner.x, max_corner.x);
      }
    }
  }

  new_player_p = old_player_p;
  new_player_p.offset += t_min * player_delta;
  player->pos = RecanonicalizePosition(tile_map, new_player_p);

#endif
  U32 new_tile = Get_Tile_Value(tile_map, player->pos);
  B32 not_same_tile = !(Is_On_Same_Tile(player->pos, old_player_p));
  // NOTE: Update Player Z
  if (not_same_tile)
  {
    if (new_tile == 3)
    {
      player->pos.abs_tile_z = 1;
    }
    else if (new_tile == 4)
    {
      player->pos.abs_tile_z = 0;
    }
  }

  // NOTE: Update player sprite direction
  //
  if (Abs_F32(player->vel.x) > Abs_F32(player->vel.y))
  {
    if (player->vel.x > 0)
    {
      player->sprite.sprite_index = E_CHAR_WALK_RIGHT_1;
    }
    else if (player->vel.x < 0)
    {
      player->sprite.sprite_index = E_CHAR_WALK_LEFT_1;
    }
  }
  else if (Abs_F32(player->vel.x) < Abs_F32(player->vel.y))
  {
    if (player->vel.y > 0)
    {
      player->sprite.sprite_index = E_CHAR_WALK_BACK_1;
    }
    else if (player->vel.y < 0)
    {
      player->sprite.sprite_index = E_CHAR_WALK_FRONT_1;
    }
  }
}

internal void Init_Player(Game_State* game_state, U32 entity_index)
{
  Entity* entity = Get_Entity(game_state, entity_index);

  entity->exists = true;
  entity->pos.abs_tile_x = 2;
  entity->pos.abs_tile_y = 2;
  entity->pos.offset.x = 2.0f;
  entity->pos.offset.y = 2.0f;
  entity->height = 1.4f;
  entity->width = 0.6f * entity->height;

  entity->sprite = {};
  entity->sprite.sprite_sheet = &game_state->knight_sprite_sheet;

  entity->sprite.align_x = -8;
  entity->sprite.align_y = -16;

  if (!Get_Entity(game_state, game_state->camera_follow_entity_index))
  {
    game_state->camera_follow_entity_index = entity_index;
  }
}
internal U32 Add_Entity(Game_State* game_state)
{
  U32 entity_index = game_state->entity_count++;
  ASSERT(game_state->entity_count < Array_Count(game_state->entities));
  Entity* entity = &game_state->entities[entity_index];
  *entity = {};
  return entity_index;
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
    Add_Entity(game_state);

    Initialize_Arena(&game_state->permananent_arena, memory->permanent_storage_size - sizeof(Game_State),
                     (U8*)memory->permanent_storage + sizeof(Game_State));
    Arena* world_arena = &game_state->permananent_arena;

    game_state->knight_sprite_sheet = DEBUG_Load_SpriteSheet_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                                                 memory->DEBUG_Platform_Free_File_Memory, world_arena,
                                                                 "assets/SpriteSheet_Knight.bmp", 16, 16);
    game_state->grass_sprite_sheet = DEBUG_Load_SpriteSheet_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                                                memory->DEBUG_Platform_Free_File_Memory, world_arena,
                                                                "assets/GrassFloorTileset.bmp", 16, 16);
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

    game_state->camera_p.abs_tile_x = 17 / 2;
    game_state->camera_p.abs_tile_y = 9 / 2;

    game_state->world = Push_Struct(&game_state->permananent_arena, World);

    World* world = game_state->world;
    world->tile_map = Push_Struct(&game_state->permananent_arena, Tile_Map);

    Tile_Map* tile_map = world->tile_map;

    tile_map->chunk_shift = 4;
    tile_map->chunk_mask = (1u << tile_map->chunk_shift) - 1;
    tile_map->chunk_dim = (1u << tile_map->chunk_shift);
    tile_map->tile_chunk_count_x = 128;
    tile_map->tile_chunk_count_y = 128;
    tile_map->tile_chunk_count_z = 2;
    tile_map->tile_chunks = Push_Array(
        &game_state->permananent_arena,
        tile_map->tile_chunk_count_x * tile_map->tile_chunk_count_y * tile_map->tile_chunk_count_z, Tile_Chunk);

    tile_map->tile_size_in_meters = 1.4f;

    // F32 lower_left_x = ((F32)-tile_map->tile_size_in_pixels * 0.5f);
    // F32 lower_left_y = (F32)buffer->height;

    // tile_map->tile_chunks = &(Tile_Chunk){.tiles = &tiles[0][0]};
    U32 random_index = 0;
    U32 screen_x = 0;
    U32 screen_y = 0;
    U32 abs_tile_z = 0;
    B32 door_left = false;
    B32 door_right = false;
    B32 door_top = false;
    B32 door_bottom = false;
    B32 door_up = false;
    B32 door_down = false;

    for (U32 screen_index = 0; screen_index < 100; ++screen_index)
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

      for (U32 tile_y = 0; tile_y < TILES_PER_HEIGHT; ++tile_y)
      {
        for (U32 tile_x = 0; tile_x < TILES_PER_WIDTH; ++tile_x)
        {
          U32 abs_tile_x = screen_x * TILES_PER_WIDTH + tile_x;
          U32 abs_tile_y = screen_y * TILES_PER_HEIGHT + tile_y;

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
          Set_Tile_Value(&game_state->permananent_arena, world->tile_map, abs_tile_x, abs_tile_y, abs_tile_z,
                         tile_value);
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
    memory->is_initialized = true;
  }
  F32 delta_time = input->delta_time_s;

  World* world = game_state->world;
  Tile_Map* tile_map = world->tile_map;

  S32 tile_size_in_pixels = 60;
  F32 sprite_scale = 4.f * ((F32)tile_size_in_pixels / 64.f);
  F32 meters_to_pixels = (F32)tile_size_in_pixels / world->tile_map->tile_size_in_meters;

  // Tile_Chunk_Position chunk_pos = Get_Chunk_Position(tile_map, game_state->player_p.abs_tile_x,
  //                                                    game_state->player_p.abs_tile_y,
  //                                                    game_state->player_p.abs_tile_z);
  // Tile_Chunk* tile_chunk =
  //     Tile_Map_Get_Tile_Chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z);
  // ASSERT(tile_chunk);

  // Entity* old_player = Get_Entity(game_state, game_state->player_index_for_controller[0]);
  // Tile_Map_Position old_player_pos = old_player->pos;
  //
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

    U32 player_index = game_state->player_index_for_controller[controller_index];
    Entity* controlling_entity = Get_Entity(game_state, player_index);
    if (controlling_entity)
    {
      // NOTE: Update all players

      Move_Player(game_state, controlling_entity, delta_time, controller->stick_left, controller->action_up.ended_down);
    }
    else
    {
      if (controller->start.ended_down)
      {
        U32 entity_index = Add_Entity(game_state);

        Init_Player(game_state, entity_index);
        game_state->player_index_for_controller[controller_index] = entity_index;
      }
    }
  }

  // NOTE: Update camera position

  Entity* camera_follow_entity = Get_Entity(game_state, game_state->camera_follow_entity_index);
  if (camera_follow_entity)
  {
    game_state->camera_p.abs_tile_z = camera_follow_entity->pos.abs_tile_z;
    Tile_Map_Diff diff = Tile_Map_Pos_Subtract(tile_map, &camera_follow_entity->pos, &game_state->camera_p);
    // TODO: Get main character old pos  for this
    B32 not_same_tile = true; //!(Is_On_Same_Tile(camera_follow_entity->pos, old_player_pos));

    if (not_same_tile)
    {
      F32 tile_size = tile_map->tile_size_in_meters;
      F32 tile_map_width = ((TILES_PER_WIDTH / 2) * tile_size + 0.5f * tile_size);
      F32 tile_map_height = ((TILES_PER_HEIGHT / 2) * tile_size + 0.5f * tile_size);
      if (diff.dxy.x > tile_map_width)
      {
        game_state->camera_p.abs_tile_x += TILES_PER_WIDTH;
      }
      else if (diff.dxy.x < -tile_map_width)
      {
        game_state->camera_p.abs_tile_x -= TILES_PER_WIDTH;
      }
      if (diff.dxy.y > tile_map_height)
      {
        game_state->camera_p.abs_tile_y += TILES_PER_HEIGHT;
      }
      else if (diff.dxy.y < -tile_map_height)

      {
        game_state->camera_p.abs_tile_y -= TILES_PER_HEIGHT;
      }
    }
  }
  // NOTE: Clear Buffer --------------------------------------------------------
  // Draw_Rectf(buffer, 0, 0, (F32)buffer->width, (F32)buffer->height, 0.35f, 0.58f, 0.93f);
  //------------------------------------
  //

  // NOTE: Draw Tile map

  Vec2 screen_center = Vec2{{0.5f * (F32)buffer->width, 0.5f * (F32)buffer->height}};

  for (S32 rel_row = -10; rel_row < 10; rel_row++)
  {
    for (S32 rel_col = -20; rel_col < 20; rel_col++)
    {
      U32 col = game_state->camera_p.abs_tile_x + (U32)rel_col;
      U32 row = game_state->camera_p.abs_tile_y + (U32)rel_row;
      U32 tile = Get_Tile_Value(tile_map, col, row, game_state->camera_p.abs_tile_z);
      if (tile != 0)
      {

        Vec2 center = {{screen_center.x + ((F32)(rel_col) * (F32)tile_size_in_pixels) -
                            meters_to_pixels * game_state->camera_p.offset.x,
                        screen_center.y - ((F32)(rel_row) * (F32)tile_size_in_pixels) +
                            meters_to_pixels * game_state->camera_p.offset.y}};
        Vec2 tile_size_pixels = {{(F32)tile_size_in_pixels, (F32)tile_size_in_pixels}};

        Vec2 min = center - 0.5f * tile_size_pixels;
        // Vec2 max = center + 0.5f * tile_size_pixels;

        if (game_state->camera_p.abs_tile_z == 0)
        {

          S32 grass_sprite_index = ((U32)col % 6 + (U32)row % 5) % 4;

          Draw_Sprite_Sheet_Sprite(buffer, &game_state->grass_sprite_sheet, grass_sprite_index, min.x, min.y,
                                   sprite_scale, false);
        }
        else
        {
          Draw_BMP(buffer, &game_state->stone_floor_bmp, min.x, min.y, sprite_scale, false);
        }

        if (tile == 2)
        {
          if (game_state->camera_p.abs_tile_z == 0)
          {
            Draw_BMP(buffer, &game_state->wall1_bmp, min.x, min.y, sprite_scale, true);
          }
          else
          {
            Draw_BMP(buffer, &game_state->wall2_bmp, min.x, min.y, sprite_scale, true);
          }
        }
        else if (tile == 3)
        {
          Draw_BMP(buffer, &game_state->stair_down_bmp, min.x, min.y, sprite_scale, true);
        }
        else if (tile == 4)
        {
          Draw_BMP(buffer, &game_state->stair_up_bmp, min.x, min.y, sprite_scale, true);
        }
      }
    }
  }

  // NOTE:Draw entities
  Entity* entity = game_state->entities;
  for (U32 entity_index = 0; entity_index < game_state->entity_count; ++entity_index, ++entity)
  {
    // TODO: Culling of entities based on Z / camera view
    if (entity->exists)
    {
      Tile_Map_Diff diff = Tile_Map_Pos_Subtract(game_state->world->tile_map, &entity->pos, &game_state->camera_p);

      Vec2 screen_pos = {
          {screen_center.x + meters_to_pixels * diff.dxy.x, screen_center.y - meters_to_pixels * diff.dxy.y}};

      Draw_Player_Sprite(buffer, &entity->sprite, screen_pos.x, screen_pos.y, sprite_scale, true);

      F32 entity_left = screen_pos.x - ((F32)meters_to_pixels * entity->width) / 2;
      F32 entity_right = screen_pos.x + ((F32)meters_to_pixels * entity->width) / 2;

      Draw_Rectf(buffer, entity_left, screen_pos.y - 1.f, entity_right, screen_pos.y, 0.0f, 1.f, 0.8f);
    }
  }

  // Draw_BMP(buffer, &game_state->test_bmp, 10, 10, 2);
  // Draw_BMP(buffer, &game_state->test_bmp, 20, 20, 2);
  Draw_Inputs(buffer, input);
  return;
}

extern "C" GAME_GET_SOUND_SAMPLES(Game_Get_Sound_Samples)
{
  Game_State* game_state = (Game_State*)memory->permanent_storage;
  // TODO: Allow sample offests for more robust platform options
  Game_Output_Sound(game_state, sound_buffer);
}
