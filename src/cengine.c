#include "base/base_core.h"
#include "base/base_math.h"
#include "cengine.h"
#include <math.h>

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

    U8* frame_ptr = sound_buffer->sample_buffer + frame * channels * sample_format_bytes;

    for (S32 ch = 0; ch < channels; ++ch)
    {
      U8* sample_ptr = frame_ptr + ch * sample_format_bytes;
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

  U8* row_in_bytes = (U8*)buffer->memory + min_y * buffer->pitch_in_bytes + min_x * buffer->bytes_per_pixel;

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
GAME_UPDATE_AND_RENDER(Game_Update_And_Render)
{
  ASSERT((&input->controllers[0].button_count - &input->controllers[0].buttons[0]) ==
         (Array_Count(input->controllers[0].buttons)));
  ASSERT(sizeof(Game_State) < memory->permanent_storage_size);

  Game_State* game_state = (Game_State*)memory->permanent_storage;
  if (!memory->is_initialized)
  {
    memory->is_initialized = true;
  }
  F32 delta_time = input->delta_time_s;

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
    game_state->player_x += player_speed * controller->stick_left.x;
    game_state->player_y -= player_speed * controller->stick_left.y;
  }

  // NOTE: Clear Buffer --------------------------------------------------------
  Draw_Rect(buffer, 0, 0, (F32)buffer->width, (F32)buffer->height, 0.35f, 0.58f, 0.93f);
  //------------------------------------
  // clang-format off
  U32 tile_map[9][17]=
  {
    {1, 1, 1, 1,  0, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
    {1, 1, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1},
    {1, 1, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1},
    {0, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 0},
    {1, 1, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1},
    {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0, 1},
    {1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
  };
  // clang-format on

  F32 upper_left_x = -30.f;
  F32 upper_left_y = 0.f;
  F32 tile_width = 60.f;
  F32 tile_height = 60.f;
  for (S32 row = 0; row < 9; row++)
  {
    for (S32 col = 0; col < 17; col++)
    {
      U32 tile = tile_map[row][col];
      F32 tile_color = 0.5f;
      if (tile == 1)
      {
        tile_color = 1.f;
      }
      F32 min_x = upper_left_x + ((F32)col * tile_width);
      F32 min_y = upper_left_y + ((F32)row * tile_height);

      F32 max_x = min_x + tile_width;
      F32 max_y = min_y + tile_height;

      Draw_Rect(buffer, min_x, min_y, max_x, max_y, tile_color, tile_color, tile_color);
    }
  }

  // NOTE:Draw player
  {
    F32 player_width = tile_width * 0.75f;
    F32 player_height = tile_height;
    F32 min_x = game_state->player_x;
    F32 min_y = game_state->player_y;
    F32 max_x = min_x + player_width;
    F32 max_y = min_y + player_height;
    Draw_Rect(buffer, min_x, min_y, max_x, max_y, 0.2f, 0.f, 0.8f);
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
