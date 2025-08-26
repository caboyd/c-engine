#include "base/base_core.h"
#include "base/base_math.h"
#include "cengine.h"

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9

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

internal void Draw_Rect(Game_Offscreen_Buffer* buffer, F32 fmin_x, F32 fmin_y, F32 fmax_x, F32 fmax_y, F32 r, F32 g,
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

  U32 color = (F32_to_U32_255(r) << 16) | (F32_to_U32_255(g) << 8) | F32_to_U32_255(b) << 0;

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
      Draw_Rect(buffer, 10.f + offset, 10.f, 20.f + offset, 20.f, 1.f, 0.f, 0.f);
    }
  }
  if (input->mouse_z > 0)
  {
    Draw_Rect(buffer, 10.f, 30.f, 20.f, 40.f, 1.f, 0.f, 0.f);
  }
  else if (input->mouse_z < 0)
  {
    Draw_Rect(buffer, 30.f, 30.f, 40.f, 40.f, 1.f, 0.f, 0.f);
  }
  F32 mouse_x = (F32)input->mouse_x;
  F32 mouse_y = (F32)input->mouse_y;
  Draw_Rect(buffer, mouse_x, mouse_y, mouse_x + 5.0f, mouse_y + 5.0f, 0.f, 1.f, 0.f);
}

internal inline U32 Tile_Map_Get_Tile_Unchecked(World* world, Tile_Map* tile_map, S32 tile_x, S32 tile_y)
{
  ASSERT(tile_map);
  ASSERT((tile_x >= 0) && (tile_x < world->tile_count_x) && (tile_y >= 0) && (tile_y < world->tile_count_y));
  U32 tile = tile_map->tiles[(tile_y * world->tile_count_x) + tile_x];
  return tile;
}

internal inline Tile_Map* World_Get_Tile_Map(World* world, S32 tile_map_x, S32 tile_map_y)
{

  Tile_Map* tile_map = 0;
  if ((tile_map_x >= 0 && tile_map_x < world->tile_map_count_x) &&
      (tile_map_y >= 0 && tile_map_y < world->tile_map_count_x))
  {
    tile_map = &world->tile_maps[(tile_map_y * world->tile_map_count_x) + tile_map_x];
  }
  return tile_map;
}

internal B32 Is_Tile_Map_Point_Empty(World* world, Tile_Map* tile_map, S32 test_tile_x, S32 test_tile_y)
{
  B32 is_empty = false;
  if (tile_map)
  {
    if ((test_tile_x >= 0) && (test_tile_x < TILE_MAP_COUNT_X) && (test_tile_y >= 0) &&
        (test_tile_y < TILE_MAP_COUNT_Y))
    {
      U32 tile_map_value = Tile_Map_Get_Tile_Unchecked(world, tile_map, test_tile_x, test_tile_y);
      is_empty = (tile_map_value == 0);
    }
  }
  return is_empty;
}

internal void Recanonicalize_Coord(World* world, S32 tile_count, S32* tile_map, S32* tile, F32* tile_rel)
{
  // TODO: Need to fix rounding for very small tile_rel floats caused by float precision.
  //  Ex near -0.00000001 tile_rel + 60 to result in 60 wrapping to next tile
  //  Don't use divide multiple method
  //
  // TODO: Add bounds checking to prevent wrapping

  S32 tile_offset = Floor_F32_S32(*tile_rel / (F32)world->tile_size_in_meters);
  *tile += tile_offset;

  *tile_rel -= (F32)tile_offset * (F32)world->tile_size_in_meters;

  ASSERT(*tile_rel >= 0);
  if (*tile_rel >= (F32)world->tile_size_in_meters)
  {
    *tile_rel -= 0x1.0p-23f;
  }
  ASSERT(*tile_rel < (F32)world->tile_size_in_meters);

  if (*tile < 0)
  {
    *tile += tile_count;
    --*tile_map;
  }
  else if (*tile >= tile_count)
  {
    *tile -= tile_count;
    ++*tile_map;
  }
}

internal Canonical_Position RecanonicalPosition(World* world, Canonical_Position pos)
{
  Canonical_Position result = pos;

  Recanonicalize_Coord(world, world->tile_count_x, &result.tile_map_x, &result.tile_x, &result.tile_rel_x);
  Recanonicalize_Coord(world, world->tile_count_y, &result.tile_map_y, &result.tile_y, &result.tile_rel_y);

  return result;
}

internal B32 Is_World_Tile_Empty(World* world, Canonical_Position pos)
{
  B32 is_empty = false;

  Tile_Map* tile_map = World_Get_Tile_Map(world, pos.tile_map_x, pos.tile_map_y);

  U32 tile_map_value = Tile_Map_Get_Tile_Unchecked(world, tile_map, pos.tile_x, pos.tile_y);
  is_empty = (tile_map_value == 0);

  return is_empty;
}

GAME_UPDATE_AND_RENDER(Game_Update_And_Render)
{
  ASSERT((&input->controllers[0].button_count - &input->controllers[0].buttons[0]) ==
         (Array_Count(input->controllers[0].buttons)));
  ASSERT(sizeof(Game_State) < memory->permanent_storage_size);

  Game_State* game_state = (Game_State*)memory->permanent_storage;
  if (!memory->is_initialized)
  {
    memory->is_initialized = true;
    game_state->player_p.tile_map_x = 0;
    game_state->player_p.tile_map_y = 0;
    game_state->player_p.tile_x = 3;
    game_state->player_p.tile_y = 5;
    game_state->player_p.tile_rel_x = 0.1f;
    game_state->player_p.tile_rel_y = 0.1f;
  }
  F32 delta_time = input->delta_time_s;

  // clang-format off
  
  U32 tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
    {1, 1, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1},
    {1, 1, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 0},
    {1, 1, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1},
    {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0, 1},
    {1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
   };
  U32 tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
   };
  U32 tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
   };
  U32 tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
   };
  // clang-format on

  Tile_Map tile_maps[2][2] = {0};
  tile_maps[0][0].tiles = &tiles00[0][0];
  tile_maps[0][1].tiles = &tiles10[0][0];
  tile_maps[1][0].tiles = &tiles01[0][0];
  tile_maps[1][1].tiles = &tiles11[0][0];

  World world = {
      .tile_size_in_meters = 1.4f,
      .tile_size_in_pixels = 60,
      .tile_count_x = TILE_MAP_COUNT_X,
      .tile_count_y = TILE_MAP_COUNT_Y,

      .tile_map_count_x = 2,
      .tile_map_count_y = 2,
  };
  world.meters_to_pixels = (F32)world.tile_size_in_pixels / world.tile_size_in_meters;
  world.upper_left_x = -world.tile_size_in_pixels / 2;
  world.upper_left_y = 0;

  F32 player_height = 1.4f;
  F32 player_width = player_height * 0.75f;

  world.tile_maps = (Tile_Map*)tile_maps;

  Tile_Map* tile_map = World_Get_Tile_Map(&world, game_state->player_p.tile_map_x, game_state->player_p.tile_map_y);
  ASSERT(tile_map);

  for (S32 controller_index = 0; controller_index < (S32)Array_Count(input->controllers); controller_index++)
  {

    Game_Controller_Input* controller = GetController(input, 0);

    if (controller->dpad_up.ended_down)
    {
      game_state->volume += 0.001f;
    }
    else if (controller->dpad_down.ended_down)
    {
      game_state->volume -= 0.001f;
    }
    game_state->volume = CLAMP(game_state->volume, 0.0f, 0.5f);

    F32 player_speed = delta_time * 1.f;

    Canonical_Position new_player_p = game_state->player_p;
    new_player_p.tile_rel_x += (player_speed * controller->stick_left.x);
    new_player_p.tile_rel_y += (player_speed * controller->stick_left.y);
    new_player_p = RecanonicalPosition(&world, new_player_p);

    // TODO: Delta function that recanonicalizes

    F32 player_half_width = player_width / 2.f;

    Canonical_Position player_bottom_left = new_player_p;
    player_bottom_left.tile_rel_x -= player_half_width;
    player_bottom_left = RecanonicalPosition(&world, player_bottom_left);
    Canonical_Position player_bottom_right = new_player_p;
    player_bottom_right.tile_rel_x += player_half_width;
    player_bottom_right = RecanonicalPosition(&world, player_bottom_right);

    B32 is_valid = Is_World_Tile_Empty(&world, player_bottom_left) && Is_World_Tile_Empty(&world, player_bottom_right);
    if (is_valid)
    {
      game_state->player_p = new_player_p;
    }
  }

  // NOTE: Clear Buffer --------------------------------------------------------
  Draw_Rect(buffer, 0, 0, (F32)buffer->width, (F32)buffer->height, 0.35f, 0.58f, 0.93f);
  //------------------------------------
  for (S32 row = 0; row < world.tile_count_y; row++)
  {
    for (S32 col = 0; col < world.tile_count_x; col++)
    {
      U32 tile = Tile_Map_Get_Tile_Unchecked(&world, tile_map, col, row);
      F32 tile_color = 0.5f;
      if (tile == 1)
      {
        tile_color = 1.f;
      }
      if ((row == game_state->player_p.tile_y) && (col == game_state->player_p.tile_x))
      {
        tile_color = 0.22f;
      }
      F32 min_x = (F32)world.upper_left_x + ((F32)col * (F32)world.tile_size_in_pixels);
      F32 min_y = (F32)world.upper_left_y + ((F32)row * (F32)world.tile_size_in_pixels);

      F32 max_x = min_x + (F32)world.tile_size_in_pixels;
      F32 max_y = min_y + (F32)world.tile_size_in_pixels;

      Draw_Rect(buffer, min_x, min_y, max_x, max_y, tile_color, tile_color, tile_color);
    }
  }

  // NOTE:Draw player
  {
    // F32 player_tile_x = (F32)(world.upper_left_x + game_state->player_p.tile_x * world.tile_size_in_pixels);
    // F32 player_tile_y = (F32)(world.upper_left_y + game_state->player_p.tile_y * world.tile_size_in_pixels);
    // Draw_Rect(buffer, player_tile_x, player_tile_y, player_tile_x + (F32)world.tile_size_in_pixels,
    //           player_tile_y + (F32)world.tile_size_in_pixels, 0.f, .757f, .459f);

    F32 player_x = (F32)(world.upper_left_x + game_state->player_p.tile_x * world.tile_size_in_pixels) +
                   world.meters_to_pixels * game_state->player_p.tile_rel_x;
    F32 player_y = (F32)(world.upper_left_y + game_state->player_p.tile_y * world.tile_size_in_pixels) +
                   world.meters_to_pixels * game_state->player_p.tile_rel_y;
    F32 min_x = player_x - (.5f * player_width * world.meters_to_pixels);
    F32 min_y = player_y - player_height * world.meters_to_pixels;
    F32 max_x = min_x + player_width * world.meters_to_pixels;
    F32 max_y = min_y + player_height * world.meters_to_pixels;
    Draw_Rect(buffer, min_x, min_y, max_x, max_y, 1.23f, .757f, .459f);
    // Draw_Rect(buffer, min_x, game_state->player_y - 2.f, min_x + 2.0f, game_state->player_y, 0.0f, 1.f, 0.8f);
    // Draw_Rect(buffer, max_x - 2.f, game_state->player_y - 2.f, max_x, game_state->player_y, 0.0f, 1.f, 0.8f);
    Draw_Rect(buffer, player_x - 1.f, player_y - 2.f, player_x + 1.f, player_y, 0.0f, 1.f, 0.8f);
  }
  Draw_Inputs(buffer, input);
  return;
}

GAME_GET_SOUND_SAMPLES(Game_Get_Sound_Samples)
{
  Game_State* game_state = (Game_State*)memory->permanent_storage;
  // TODO: Allow sample offests for more robust platform options
  Game_Output_Sound(game_state, sound_buffer);
}
