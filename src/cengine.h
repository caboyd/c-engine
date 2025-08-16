#ifndef CENGINE_H
#define CENGINE_H

typedef struct Game_Offscreen_Buffer Game_Offscreen_Buffer;
struct Game_Offscreen_Buffer
{
  void *memory;
  S32 width;
  S32 height;
  S32 pitch;
};

typedef enum Sample_Format
{
  SF_PCM_S16,
  SF_PCM_S24,
  SF_F32
} Sample_Format;

typedef struct Game_Output_Sound_Buffer Game_Output_Sound_Buffer;
struct Game_Output_Sound_Buffer
{
  // Per frame
  U8 *sample_buffer;
  S32 sample_count;
  // constant per session
  S32 samples_per_second;
  Sample_Format sample_format;
  S32 channel_count;
};

typedef struct Game_Controller_Stick Game_Controller_Stick;
struct Game_Controller_Stick
{
  Vec2 start;
  Vec2 end;
  Vec2 min;
  Vec2 max;
};
typedef struct Game_Button_State Game_Button_State;
struct Game_Button_State
{
  // NOTE: if we know we ended down we can calculate how many presses
  // happened since last poll and the start state;
  S32 half_transition_count;
  B32 ended_down;
};
typedef struct Game_Controller_Input Game_Controller_Input;
struct Game_Controller_Input
{
  B32 is_analog;
  union
  {
    Game_Controller_Stick sticks[2];
    struct
    {
      Game_Controller_Stick stick_left;
      Game_Controller_Stick stick_right;
    };
  };
  union
  {
    Game_Button_State buttons[16];
    struct
    {
      Game_Button_State d_left;
      Game_Button_State d_right;
      Game_Button_State d_up;
      Game_Button_State d_down;

      Game_Button_State b_left;
      Game_Button_State b_right;
      Game_Button_State b_up;
      Game_Button_State b_down;

      Game_Button_State L1;
      Game_Button_State L2;
      Game_Button_State L3;

      Game_Button_State R1;
      Game_Button_State R2;
      Game_Button_State R3;

      Game_Button_State start;
      Game_Button_State select;
    };
  };
};

typedef struct Game_Input Game_Input;
struct Game_Input
{
  Game_Controller_Input controllers[4];
};

typedef struct Game_Memory Game_Memory;
struct Game_Memory
{
  B32 is_initialized;
  U64 permanent_storage_size;
  void *permanent_storage; //NOTE: REQUIRED to be cleared to zero at startup
  U64 transient_storage_size;
  void* transient_storage; //NOTE: REQUIRED to be cleared to zero at startup
};

internal void Game_Output_Sound(Game_Output_Sound_Buffer *sound_buffer, F64 frequency);

internal void Render_Weird_Gradient(Game_Offscreen_Buffer *buffer, S32 blue_offset,
                                    S32 green_offset);
internal void Game_Update_And_Render(Game_Memory *memory, Game_Input *input,
                                     Game_Offscreen_Buffer *buffer,
                                     Game_Output_Sound_Buffer *sound_buffer);

typedef struct Game_State Game_State;
struct Game_State
{
  S32 green_offset;
  S32 blue_offset;
  F64 frequency;
};

#endif
