#include "base/base_core.h"
#include "base/base_math.h"
#include "cengine.h"
#include <math.h>

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
    F32 sample_value = (F32)sin(game_state->sine_phase);

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
  S32 min_x = Round_F32_to_S32(fmin_x);
  S32 min_y = Round_F32_to_S32(fmin_y);
  S32 max_x = Round_F32_to_S32(fmax_x);
  S32 max_y = Round_F32_to_S32(fmax_y);

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

internal inline U32 Tile_Map_Get_Tile(Tile_Map* tile_map, S32 tile_x, S32 tile_y)
{
  U32 tile = tile_map->tiles[(tile_y * tile_map->count_x) + tile_x];
  return tile;
}

internal inline Tile_Map* World_Get_Tile_Map(World* world, S32 tile_map_x, S32 tile_map_y)
{

  Tile_Map* tile_map = 0;
  if ((tile_map_x >= 0 && tile_map_x < world->count_x) && (tile_map_y >= 0 && tile_map_y < world->count_x))
  {
    tile_map = &world->tile_maps[(tile_map_y * world->count_x) + tile_map_x];
  }
  return tile_map;
}

internal B32 Is_Tile_Empty(Tile_Map* tile_map, F32 test_x, F32 test_y)
{
  B32 is_empty = false;

  S32 tile_x = Trunc_F32_S32((test_x - tile_map->upper_left_x) / tile_map->tile_width);
  S32 tile_y = Trunc_F32_S32((test_y - tile_map->upper_left_y) / tile_map->tile_height);
  if ((tile_x >= 0) && (tile_x < TILE_MAP_COUNT_X) && (tile_y >= 0) && (tile_y < TILE_MAP_COUNT_Y))
  {
    U32 tile_map_value = Tile_Map_Get_Tile(tile_map, tile_x, tile_y);
    is_empty = (tile_map_value == 0);
  }
  return is_empty;
}
internal B32 Is_World_Tile_Empty(World* world, S32 tile_map_x, S32 tile_map_y, F32 test_x, F32 test_y)
{
  B32 is_empty = false;

  Tile_Map* tile_map = World_Get_Tile_Map(world, tile_map_x, tile_map_y);
  if (tile_map)
  {
    S32 tile_x = Trunc_F32_S32((test_x - tile_map->upper_left_x) / tile_map->tile_width);
    S32 tile_y = Trunc_F32_S32((test_y - tile_map->upper_left_y) / tile_map->tile_height);
    if ((tile_x >= 0) && (tile_x < TILE_MAP_COUNT_X) && (tile_y >= 0) && (tile_y < TILE_MAP_COUNT_Y))
    {
      U32 tile_map_value = Tile_Map_Get_Tile(tile_map, tile_x, tile_y);
      is_empty = (tile_map_value == 0);
    }
  }
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
    game_state->player_x = 200;
    game_state->player_y = 300;
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

  Tile_Map tile_maps[2][2];
  tile_maps[0][0] = (Tile_Map){
      .count_x = TILE_MAP_COUNT_X,
      .count_y = TILE_MAP_COUNT_Y,
      .upper_left_x = -30.f,
      .upper_left_y = 0.f,
      .tile_width = 60.f,
      .tile_height = 60.f,
      .tiles = (U32*)tiles00,
  };

  tile_maps[0][1] = tile_maps[0][0];
  tile_maps[0][1].tiles = (U32*)tiles01;

  tile_maps[1][0] = tile_maps[0][0];
  tile_maps[1][0].tiles = (U32*)tiles10;

  tile_maps[1][1] = tile_maps[0][0];
  tile_maps[1][1].tiles = (U32*)tiles11;

  Tile_Map* tile_map = &tile_maps[0][0];

  World world;
  world.count_x = 2;
  world.count_y = 2;

  world.tile_maps = (Tile_Map*)tile_maps;

  F32 player_width = tile_map->tile_width * 0.75f;
  F32 player_height = tile_map->tile_height;
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

    F32 player_speed = delta_time * 64.f;
    F32 new_player_x = game_state->player_x + (player_speed * controller->stick_left.x);
    F32 new_player_y = game_state->player_y - (player_speed * controller->stick_left.y);

    F32 player_half_width = player_width / 2.f;
    B32 is_valid = Is_Tile_Empty(tile_map, new_player_x - player_half_width, new_player_y) &&
                   Is_Tile_Empty(tile_map, new_player_x + player_half_width, new_player_y);
    if (is_valid)
    {
      game_state->player_x = new_player_x;
      game_state->player_y = new_player_y;
    }
  }

  // NOTE: Clear Buffer --------------------------------------------------------
  Draw_Rect(buffer, 0, 0, (F32)buffer->width, (F32)buffer->height, 0.35f, 0.58f, 0.93f);
  //------------------------------------
  for (S32 row = 0; row < tile_map->count_y; row++)
  {
    for (S32 col = 0; col < tile_map->count_x; col++)
    {
      U32 tile = Tile_Map_Get_Tile(tile_map, col, row);
      F32 tile_color = 0.5f;
      if (tile == 1)
      {
        tile_color = 1.f;
      }
      F32 min_x = tile_map->upper_left_x + ((F32)col * tile_map->tile_width);
      F32 min_y = tile_map->upper_left_y + ((F32)row * tile_map->tile_height);

      F32 max_x = min_x + tile_map->tile_width;
      F32 max_y = min_y + tile_map->tile_height;

      Draw_Rect(buffer, min_x, min_y, max_x, max_y, tile_color, tile_color, tile_color);
    }
  }

  // NOTE:Draw player
  {

    F32 min_x = game_state->player_x - (player_width / 2);
    F32 min_y = game_state->player_y - player_height;
    F32 max_x = min_x + player_width;
    F32 max_y = min_y + player_height;
    Draw_Rect(buffer, min_x, min_y, max_x, max_y, 0.2f, 0.f, 0.8f);
    // Draw_Rect(buffer, min_x, game_state->player_y - 2.f, min_x + 2.0f, game_state->player_y, 0.0f, 1.f, 0.8f);
    // Draw_Rect(buffer, max_x - 2.f, game_state->player_y - 2.f, max_x, game_state->player_y, 0.0f, 1.f, 0.8f);
    Draw_Rect(buffer, game_state->player_x - 1.f, game_state->player_y - 2.f, game_state->player_x + 1.f,
              game_state->player_y, 0.0f, 1.f, 0.8f);
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
