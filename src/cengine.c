#include "base/base_core.h"
#include "base/base_math.h"
#include "cengine.h"
#include <math.h>

internal void Game_Output_Sound(Game_State* game_state, Game_Output_Sound_Buffer* sound_buffer)
{
  F32 volume = game_state->volume;
  volume *= volume;

  if (sound_buffer->sample_count == 0)
  {
    return;
  }

  S32 channels = sound_buffer->channel_count;
  S32 sample_format_bytes = sound_buffer->bytes_per_sample;

  S32 bits_per_sample = sample_format_bytes * 8;

  F64 phase_increment = 2.0 * M_PI * game_state->frequency / sound_buffer->samples_per_second;

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
internal void Render_Player(Game_Offscreen_Buffer* buffer, S32 mouse_x, S32 mouse_y)
{
  Vec2 player_pos = {.x = (F32)mouse_x, .y = (F32)mouse_y};
  U32 color = 0xFFFFFFFF;
  S32 player_size = 25;
  S32 top = (S32)player_pos.y;
  S32 bottom = (S32)player_pos.y + player_size;

  for (S32 y = top; y < bottom; y++)
  {
    if (y >= 0 && y < buffer->height)
    {

      for (S32 x = (S32)player_pos.x; x < (S32)player_pos.x + player_size; x++)
      {
        if (x >= 0 && x < buffer->width)
        {

          U8* pixel = ((U8*)buffer->memory + y * buffer->pitch + x * buffer->bytes_per_pixel);
          *(U32*)(void*)pixel = color;
        }
      }
    }
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

GAME_UPDATE_AND_RENDER(Game_Update_And_Render)
{
  ASSERT((&input->controllers[0].button_count - &input->controllers[0].buttons[0]) ==
         (Array_Count(input->controllers[0].buttons)));
  ASSERT(sizeof(Game_State) < memory->permanent_storage_size);

  Game_State* game_state = (Game_State*)memory->permanent_storage;
  if (!memory->is_initialized)
  {
    memory->is_initialized = true;
    game_state->frequency = 261;
    game_state->sine_phase = 0.0;
    game_state->volume = 0.0f;
    game_state->player_pos = (Vec2){.x = 100.f, .y = 100.f};

    char* file_name = __FILE__;

    Debug_Read_File_Result bitmap_result = memory->DEBUG_Platform_Read_Entire_File(thread, file_name);
    if (bitmap_result.contents)
    {
      // memory->DEBUG_Platform_Write_Entire_File("data/test.c", bitmap_result.contents_size,
      //                                          bitmap_result.contents);
      memory->DEBUG_Platform_Free_File_Memory(thread, bitmap_result.contents);
    }
  }
  for (S32 controller_index = 0; controller_index < (S32)Array_Count(input->controllers); controller_index++)
  {

    Game_Controller_Input* controller = GetController(input, 0);
    game_state->blue_offset += (S32)(4.0 * controller->stick_left.x);
    game_state->frequency = 261 + 128.0 * (F64)(controller->stick_left.y);
    if (controller->action_down.ended_down)
    {
      game_state->green_offset += 1;
    }
    if (controller->dpad_up.ended_down)
    {
      game_state->volume += 0.001f;
    }
    else if (controller->dpad_down.ended_down)
    {
      game_state->volume -= 0.001f;
    }
    game_state->volume = CLAMP(game_state->volume, 0.0f, 0.5f);

    // Move player
    game_state->player_pos.x += 2.f * controller->stick_left.x;
    game_state->player_pos.y -= 2.f * controller->stick_left.y;

    // Jump player
    if (game_state->jump_timer > 0.f)
    {
      game_state->player_pos.y += 10.f * sinf(2.f * (F32)M_PI * game_state->jump_timer);
    }
    if (controller->start.ended_down && game_state->jump_timer <= 0.001f)
    {
      game_state->jump_timer = 1.0f;
    }
    game_state->jump_timer -= 0.033f * (0.5f);
  }
  Render_Weird_Gradient(buffer, game_state->blue_offset, game_state->green_offset);
  for (S32 i = 0; i < (S32)Array_Count(input->mouse_buttons); i++)
  {
    if (input->mouse_buttons[i].ended_down)
    {
      Render_Player(buffer, 10 + i * 50, 10);
    }
  }
  if (input->mouse_z > 0)
  {
    Render_Player(buffer, 10, 50);
  }
  else if (input->mouse_z < 0)
  {
    Render_Player(buffer, 60, 50);
  }
  Render_Player(buffer, input->mouse_x, input->mouse_y);
  return;
}

GAME_GET_SOUND_SAMPLES(Game_Get_Sound_Samples)
{
  Game_State* game_state = (Game_State*)memory->permanent_storage;
  // TODO: Allow sample offests for more robust platform options
  Game_Output_Sound(game_state, sound_buffer);
}
