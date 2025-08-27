
#include "cengine.h"
#include "rand.h"
#include "tile.c"

#define TILES_PER_WIDTH 17
#define TILES_PER_HEIGHT 9

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

internal void Recanonicalize_Coord(Tile_Map* tile_map, U32* tile, F32* tile_rel)
{
  // TODO: Need to fix rounding for very small tile_rel floats caused by float precision.
  //  Ex near -0.00000001 tile_rel + 60 to result in 60 wrapping to next tile
  //  Don't use divide multiple method
  //
  // TODO: Add bounds checking to prevent wrapping
  // NOTE: Tile_Map is assumed to be toroidal if you walk off one edge you enter the other

  S32 tile_offset = Round_F32_S32(*tile_rel / (F32)tile_map->tile_size_in_meters);

  *tile += (U32)tile_offset;

  *tile_rel -= (F32)tile_offset * (F32)tile_map->tile_size_in_meters;

  ASSERT(*tile_rel >= -0.5f * tile_map->tile_size_in_meters);
  ASSERT(*tile_rel <= 0.5f * tile_map->tile_size_in_meters);
}

internal Tile_Map_Position RecanonicalizePosition(Tile_Map* tile_map, Tile_Map_Position pos)
{
  Tile_Map_Position result = pos;

  Recanonicalize_Coord(tile_map, &result.abs_tile_x, &result.tile_rel_x);
  Recanonicalize_Coord(tile_map, &result.abs_tile_y, &result.tile_rel_y);

  return result;
}

GAME_UPDATE_AND_RENDER(Game_Update_And_Render)
{
  ASSERT((&input->controllers[0].button_count - &input->controllers[0].buttons[0]) ==
         (Array_Count(input->controllers[0].buttons)));
  ASSERT(sizeof(Game_State) < memory->permanent_storage_size);

  Game_State* game_state = (Game_State*)memory->permanent_storage;
  if (!memory->is_initialized)
  {

    game_state->player_p.abs_tile_x = 2;
    game_state->player_p.abs_tile_y = 2;
    game_state->player_p.tile_rel_x = 0.1f;
    game_state->player_p.tile_rel_y = 0.1f;
    Initialize_Arena(&game_state->world_arena, memory->permanent_storage_size - sizeof(Game_State),
                     (U8*)memory->permanent_storage + sizeof(Game_State));
    game_state->world = Push_Struct(&game_state->world_arena, World);

    World* world = game_state->world;
    world->tile_map = Push_Struct(&game_state->world_arena, Tile_Map);

    Tile_Map* tile_map = world->tile_map;

    tile_map->chunk_shift = 4;
    tile_map->chunk_mask = (1u << tile_map->chunk_shift) - 1;
    tile_map->chunk_dim = (1u << tile_map->chunk_shift);
    tile_map->tile_chunk_count_x = 128;
    tile_map->tile_chunk_count_y = 128;
    tile_map->tile_chunk_count_z = 2;
    tile_map->tile_chunks = Push_Array(
        &game_state->world_arena,
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
          Set_Tile_Value(&game_state->world_arena, world->tile_map, abs_tile_x, abs_tile_y, abs_tile_z, tile_value);
        }
      }
      door_left = door_right;
      door_bottom = door_top;
      door_right = false;
      door_top = false;
      if (door_up)
      {
        door_down = true;
        door_up = false;
      }
      else if (door_down)
      {
        door_up = true;
        door_down = false;
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
  F32 meters_to_pixels = (F32)tile_size_in_pixels / world->tile_map->tile_size_in_meters;

  F32 player_height = world->tile_map->tile_size_in_meters;
  F32 player_width = player_height * 0.75f;

  Tile_Chunk_Position chunk_pos = Get_Chunk_Position(tile_map, game_state->player_p.abs_tile_x,
                                                     game_state->player_p.abs_tile_y, game_state->player_p.abs_tile_z);
  Tile_Chunk* tile_chunk =
      Tile_Map_Get_Tile_Chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z);
  ASSERT(tile_chunk);

  for (S32 controller_index = 0; controller_index < (S32)Array_Count(input->controllers); controller_index++)
  {

    Game_Controller_Input* controller = Get_Controller(input, 0);

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

    if (controller->action_up.ended_down)
    {
      player_speed *= 3;
    }

    Tile_Map_Position new_player_p = game_state->player_p;
    new_player_p.tile_rel_x += (player_speed * controller->stick_left.x);
    new_player_p.tile_rel_y += (player_speed * controller->stick_left.y);
    new_player_p = RecanonicalizePosition(tile_map, new_player_p);

    // TODO: Delta function that recanonicalizes

    F32 player_half_width = player_width / 2.f;

    Tile_Map_Position player_bottom_left = new_player_p;
    player_bottom_left.tile_rel_x -= player_half_width;
    player_bottom_left = RecanonicalizePosition(tile_map, player_bottom_left);
    Tile_Map_Position player_bottom_right = new_player_p;
    player_bottom_right.tile_rel_x += player_half_width;
    player_bottom_right = RecanonicalizePosition(tile_map, player_bottom_right);

    B32 is_valid =
        Is_Tile_Map_Tile_Empty(tile_map, player_bottom_left) && Is_Tile_Map_Tile_Empty(tile_map, player_bottom_right);
    if (is_valid)
    {
      game_state->player_p = new_player_p;
    }
  }

  // NOTE: Clear Buffer --------------------------------------------------------
  Draw_Rect(buffer, 0, 0, (F32)buffer->width, (F32)buffer->height, 0.35f, 0.58f, 0.93f);
  //------------------------------------
  //

  // NOTE: Draw Tile map
  F32 screen_center_x = .5f * (F32)buffer->width;
  F32 screen_center_y = .5f * (F32)buffer->height;

  for (S32 rel_row = -10; rel_row < 10; rel_row++)
  {
    for (S32 rel_col = -20; rel_col < 20; rel_col++)
    {
      U32 col = game_state->player_p.abs_tile_x + (U32)rel_col;
      U32 row = game_state->player_p.abs_tile_y + (U32)rel_row;
      U32 tile = Get_Tile_Value(tile_map, col, row, game_state->player_p.abs_tile_z);
      if (tile != 0)
      {
        F32 tile_color = 0.5f;
        if (tile == 4)
        {
          tile_color = 0.1f;
        }
        else if (tile == 3)
        {
          tile_color = 0.0f;
        }
        else if (tile == 2)
        {
          tile_color = 1.f;
        }
        if ((row == game_state->player_p.abs_tile_y) && (col == game_state->player_p.abs_tile_x))
        {
          tile_color = 0.22f;
        }
        F32 center_x = screen_center_x + ((F32)(rel_col) * (F32)tile_size_in_pixels) -
                       meters_to_pixels * game_state->player_p.tile_rel_x;
        F32 center_y = screen_center_y - ((F32)(rel_row) * (F32)tile_size_in_pixels) +
                       meters_to_pixels * game_state->player_p.tile_rel_y;
        F32 min_x = center_x - 0.5f * (F32)tile_size_in_pixels;
        F32 min_y = center_y - 0.5f * (F32)tile_size_in_pixels;
        F32 max_x = min_x + (F32)tile_size_in_pixels;
        F32 max_y = min_y + (F32)tile_size_in_pixels;

        Draw_Rect(buffer, min_x, min_y, max_x, max_y, tile_color, tile_color, tile_color);
      }
    }
  }

  // NOTE:Draw player
  {

    F32 player_x = screen_center_x;
    F32 player_y = screen_center_y;
    F32 min_x = player_x - (.5f * player_width * meters_to_pixels);
    F32 min_y = player_y - player_height * meters_to_pixels;
    F32 max_x = min_x + player_width * meters_to_pixels;
    F32 max_y = min_y + player_height * meters_to_pixels;
    Draw_Rect(buffer, min_x, min_y, max_x, max_y, 1.23f, .757f, .459f);

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
