#pragma once

typedef struct Game_Offscreen_Buffer Game_Offscreen_Buffer;
struct Game_Offscreen_Buffer
{
  void *memory;
  S32 width;
  S32 height;
  S32 pitch;
};

void Render_Weird_Gradient(Game_Offscreen_Buffer *buffer, S32 blue_offset, S32 green_offset)
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

void Game_Update_And_Render(Game_Offscreen_Buffer *buffer, S32 x_offset, S32 y_offset)
{
  Render_Weird_Gradient(buffer, x_offset, y_offset);
  return;
}
