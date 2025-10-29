#ifndef CENGINE_PLATFORM_H
#define CENGINE_PLATFORM_H

// NOTE: Services that the platform layer provides to the game
#include <mmdeviceapi.h>
#include <audioclient.h>

typedef struct Thread_Context Thread_Context;

struct Thread_Context
{
  int placeholder;
};

#if CENGINE_INTERNAL
typedef struct Debug_Read_File_Result Debug_Read_File_Result;

struct Debug_Read_File_Result
{
  U32 contents_size;
  void* contents;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(Thread_Context* thread, void* memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(Debug_Platform_Free_File_Memory_Func);
// DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUG_Platform_Free_File_Memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) Debug_Read_File_Result name(Thread_Context* thread, char* filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(Debug_Platform_Read_Entire_File_Func);
// DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUG_Platform_Read_Entire_File);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name)                                                                         \
  B32 name(Thread_Context* thread, char* filename, U32 memory_size, void* memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(Debug_Platform_Write_Entire_File_Func);

// DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUG_Platform_Write_Entire_File);

enum
{
  Debug_Cycle_Counter_Game_Update_And_Render,
  Debug_Cycle_Counter_Render_Group_To_Output,
  Debug_Cycle_Counter_Draw_Rect_Slowly_Hot,
  Debug_Cycle_Counter_Draw_Rect_Quickly_Hot,
  Debug_Cycle_Counter_Process_Pixel,
  Debug_Cycle_Counter_Count,
};

struct Debug_Cycle_Counter
{
  U64 cycle_count;
  U32 hit_count;
};

extern struct Game_Memory* debug_global_memory;

#if COMPILER_CLANG
// clang-format off
#define BEGIN_TIMED_BLOCK(ID) U64 start_##ID = __rdtsc();
#define END_TIMED_BLOCK(ID) do { \
    debug_global_memory->counters[Debug_Cycle_Counter_##ID].cycle_count += __rdtsc() - start_##ID; \
    debug_global_memory->counters[Debug_Cycle_Counter_##ID].hit_count++; \
} while (0)
#define END_TIMED_BLOCK_COUNTED(ID, count) do { \
    debug_global_memory->counters[Debug_Cycle_Counter_##ID].cycle_count += __rdtsc() - start_##ID; \
    debug_global_memory->counters[Debug_Cycle_Counter_##ID].hit_count += (count); \
} while (0)
#else
#define BEGIN_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK_COUNTED(ID, count)
#endif // #if CLANG
#else
#define BEGIN_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK(ID)
#endif /* CENGINE_INTERNAL */
// clang-format on

#define BITMAP_BYTES_PER_PIXEL 4

typedef struct Game_Offscreen_Buffer Game_Offscreen_Buffer;

struct Game_Offscreen_Buffer
{
  void* memory;
  S32 width;
  S32 height;
  S32 pitch_in_bytes;
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
  U8* sample_buffer;
  S32 sample_count;
  // constant per session
  S32 samples_per_second;
  Sample_Format sample_format;
  S32 bytes_per_sample;
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

struct Game_Input
{
  F32 delta_time_s;
  B32 executable_reloaded;

  Game_Button_State mouse_buttons[5];
  S32 mouse_x;
  S32 mouse_y;
  S32 mouse_z;

  Game_Controller_Input controllers[5];
};

internal Game_Controller_Input* Get_Controller(Game_Input* input, S32 controller_index)
{
  ASSERT(controller_index < (S32)Array_Count(input->controllers));
  Game_Controller_Input* result = &input->controllers[controller_index];
  return result;
}

struct Game_Memory
{
  B32 is_initialized;
  U64 permanent_storage_size;

  union
  {
    Arena* permanent_arena;
    void* permanent_storage; // NOTE: REQUIRED to be cleared to zero at startup
  };

  U64 transient_storage_size;
  void* transient_storage; // NOTE: REQUIRED to be cleared to zero at startup

  Debug_Platform_Free_File_Memory_Func* DEBUG_Platform_Free_File_Memory;
  Debug_Platform_Read_Entire_File_Func* DEBUG_Platform_Read_Entire_File;
  Debug_Platform_Write_Entire_File_Func* DEBUG_Platform_Write_Entire_File;

#if CENGINE_INTERNAL
  Debug_Cycle_Counter counters[256];
#endif
};

#define GAME_UPDATE_AND_RENDER(name)                                                                                   \
  void name(Thread_Context* thread, Game_Memory* memory, Game_Input* input, Game_Offscreen_Buffer* buffer)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render_Func);

#define GAME_GET_SOUND_SAMPLES(name)                                                                                   \
  void name(Thread_Context* thread, Game_Memory* memory, Game_Output_Sound_Buffer* sound_buffer)
typedef GAME_GET_SOUND_SAMPLES(Game_Get_Sound_Samples_Func);

#endif
