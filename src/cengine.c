#include "cengine.h"
#include "base/base_math.h"
#include <math.h>

internal S32 Sample_Format_Bytes(Sample_Format sample_format)
{
  if (sample_format == SF_F32)
    return 4;
  if (sample_format == SF_PCM_S24)
    return 3;
  return 2; // if(sample_format == SF_PCM_S16) return 2;
}

internal void Game_Output_Sound(Game_Output_Sound_Buffer *sound_buffer, F64 frequency)
{
  F32 volume = 0.1f;

  if (sound_buffer->sample_count == 0)
  {
    return;
  }

  S32 channels = sound_buffer->channel_count;
  S32 sample_format_bytes = Sample_Format_Bytes(sound_buffer->sample_format);
  S32 bits_per_sample = sample_format_bytes * 8;

  F64 phase_increment = 2.0 * M_PI * frequency / sound_buffer->samples_per_second;
  local_persist F64 sine_phase = 0.0;
  for (S32 frame = 0; frame < sound_buffer->sample_count; ++frame)
  {
    F32 sample_value = (F32)sin(sine_phase);

    sample_value *= volume;

    sine_phase += phase_increment;
    if (sine_phase > 2.0 * M_PI)
      sine_phase -= 2.0 * M_PI;

    U8 *frame_ptr = sound_buffer->sample_buffer + frame * channels * sample_format_bytes;

    for (S32 ch = 0; ch < channels; ++ch)
    {
      U8 *sample_ptr = frame_ptr + ch * sample_format_bytes;
      if (bits_per_sample == 32)
      {
        *((F32 *)(void *)sample_ptr) = sample_value;
      }
      else if (bits_per_sample == 16)
      {
        S16 sample = INT16_MAX * (S16)(sample_value);
        *((S16 *)(void *)sample_ptr) = sample;
      }
      else if (bits_per_sample == 24)
      {
        S32 sample = (S32)(8388607 * sample_value);
        S8 *temp_ptr = (S8 *)sample_ptr;
        *((S8 *)temp_ptr++) = sample & 0xFF;
        *((S8 *)temp_ptr++) = (sample >> 8) & 0xFF;
        *((S8 *)temp_ptr++) = (sample >> 16) & 0xFF;
      }
    }
  }
}

internal void Render_Weird_Gradient(Game_Offscreen_Buffer *buffer, S32 blue_offset,
                                    S32 green_offset)
{

  U32 *row = (U32 *)buffer->memory;
  for (int y = 0; y < buffer->height; ++y)
  {
    U32 *pixel = row;
    for (int x = 0; x < buffer->width; ++x)

    {
      // NOTE: Color      0x  AA RR GG BB
      //       Steel blue 0x  00 46 82 B4
      U32 blue = (U32)(x + blue_offset);
      U32 green = (U32)(y + green_offset);

      *pixel++ = (green << 8 | blue << 8) | blue | green;
    }
    // NOTE:because row is U32 we move 4 bytes * width
    row += buffer->width;
  }
}

internal void Game_Update_And_Render(Game_Memory *memory, Game_Input *input,
                                     Game_Offscreen_Buffer *buffer,
                                     Game_Output_Sound_Buffer *sound_buffer)
{
  ASSERT(sizeof(Game_State) < memory->permanent_storage_size);

  Game_State *game_state = (Game_State *)memory->permanent_storage;
  if (!memory->is_initialized)
  {
    memory->is_initialized = true;
    game_state->frequency = 261;
  }
  Game_Controller_Input *input0 = &input->controllers[0];
  if (input0->is_analog)
  {
    // NOTE: analog movement
    game_state->frequency = 261 + 128.0 * (F64)(input0->sticks[0].end.x);
    game_state->blue_offset += (S32)(4.0 * input0->sticks[0].end.x);
  }
  else
  {
    // NOTE: digital movement
  }

  if (input0->b_down.ended_down)
  {
    game_state->green_offset += 1;
  }

  // TODO: Allow sample offests for more robust platform options
  Game_Output_Sound(sound_buffer, game_state->frequency);
  Render_Weird_Gradient(buffer, game_state->blue_offset, game_state->green_offset);
  return;
}
