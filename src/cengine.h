#ifndef CENGINE_H
#define CENGINE_H

// NOTE: Services that the platform layer provides to the game

#ifdef CENGINE_INTERNAL

typedef struct Debug_Read_File_Result Debug_Read_File_Result;
struct Debug_Read_File_Result
{
  U32 contents_size;
  void *contents;
};

internal Debug_Read_File_Result DEBUG_Platform_Read_Entire_File(char *filename);
internal void DEBUG_Plaftorm_Free_File_Memory(void *memory);
internal B32 DEBUG_Platform_Write_Entire_File(char *filename, U32 memory_size, void *memory);

#endif

// NOTE: Services that the game provides to the platform layer

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
  B32 is_connected;

  union
  {
    Vec2 sticks[2];
    struct
    {
      Vec2 stick_left;
      Vec2 stick_right;
    };
  };
  union
  {
    Game_Button_State buttons[16];
    struct
    {
      Game_Button_State dpad_left;
      Game_Button_State dpad_right;
      Game_Button_State dpad_up;
      Game_Button_State dpad_down;

      Game_Button_State action_left;
      Game_Button_State action_right;
      Game_Button_State action_up;
      Game_Button_State action_down;

      Game_Button_State L1;
      Game_Button_State L2;
      Game_Button_State L3;

      Game_Button_State R1;
      Game_Button_State R2;
      Game_Button_State R3;

      Game_Button_State back;
      Game_Button_State start;

      // NOTE:all buttons must be above this line
      Game_Button_State button_count;
    };
  };
};

typedef struct Game_Input Game_Input;
struct Game_Input
{
  Game_Controller_Input controllers[5];
};

internal Game_Controller_Input *GetController(Game_Input *input, S32 controller_index)
{
  ASSERT(controller_index < (S32)Array_Count(input->controllers));
  Game_Controller_Input *result = &input->controllers[controller_index];
  return result;
}

typedef struct Game_Memory Game_Memory;
struct Game_Memory
{
  B32 is_initialized;
  U64 permanent_storage_size;
  void *permanent_storage; // NOTE: REQUIRED to be cleared to zero at startup
  U64 transient_storage_size;
  void *transient_storage; // NOTE: REQUIRED to be cleared to zero at startup
};

internal void Game_Output_Sound(Game_Output_Sound_Buffer *sound_buffer, F64 frequency);

internal void Render_Weird_Gradient(Game_Offscreen_Buffer *buffer, S32 blue_offset,
                                    S32 green_offset);
internal void Game_Update_And_Render(Game_Memory *memory, Game_Input *input,
                                     Game_Offscreen_Buffer *buffer);
internal void Game_Get_Sound_Samples(Game_Memory *memory, Game_Output_Sound_Buffer *sound_buffer);

typedef struct Game_State Game_State;
struct Game_State
{
  S32 green_offset;
  S32 blue_offset;
  F64 frequency;
};

#endif
