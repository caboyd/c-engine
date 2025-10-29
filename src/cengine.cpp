
#undef CLANGD
#include "cengine.h"
#include "hot.h"
// #include "hot.cpp"
#include "render_group.cpp"
#include "world.cpp"
#include "sim_region.cpp"
#include "entity.cpp"

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

inline Vec2 Top_Down_Align_Percentage(S32 bitmap_width, S32 bitmap_height, Vec2 align)
{
  align.x = Safe_Ratio0(align.x + 0.5f, (F32)bitmap_width);
  align.y = ((F32)bitmap_height - 1.f) - align.y;
  align.y = Safe_Ratio0(align.y + 0.5f, (F32)bitmap_height);
  return align;
}

internal Loaded_Bitmap DEBUG_Load_BMP(Thread_Context* thread, Debug_Platform_Read_Entire_File_Func* Read_Entire_File,
                                      Debug_Platform_Free_File_Memory_Func* Free_File_Memory, Arena* arena,
                                      char* file_name, Vec2 align)
{

  Loaded_Bitmap result = {};

  // NOTE: byte order in memory is AA RR GG BB
  Debug_Read_File_Result read_result = Read_Entire_File(thread, file_name);
  if (read_result.contents_size != 0)
  {
    void* contents = read_result.contents;

    Bitmap_Header* header = (Bitmap_Header*)contents;
    U32* pixels = (U32*)(void*)((U8*)contents + header->OffBits);
    result.width = header->Width;
    result.height = header->Height;

    result.width_over_height = Safe_Ratio0((F32)result.width, (F32)result.height);
    result.align_percentage = Top_Down_Align_Percentage(result.width, result.height, align);

    // TODO: support flipped bitmaps
    ASSERT(result.height >= 0);
    ASSERT(header->Compression == 3);

    // NOTE: memory copy to dest first because pixels may not be 4 byte aligned
    // and may cause a crash if read as 4 bytes.
    S32 pixels_count = result.width * result.height;
    result.memory = Push_Array(arena, (U64)pixels_count, U32);

    Memory_Copy(result.memory, pixels, (pixels_count * (S32)sizeof(U32)));

    // NOTE: Shift down to bottom bits and shift up to match
    //   BB GG RR AA
    //   low bits to high bits

    Bit_Scan_Result blue_scan = Find_Least_Significant_Set_Bit(header->BlueMask);
    Bit_Scan_Result red_scan = Find_Least_Significant_Set_Bit(header->RedMask);
    Bit_Scan_Result green_scan = Find_Least_Significant_Set_Bit(header->GreenMask);
    Bit_Scan_Result alpha_scan = Find_Least_Significant_Set_Bit(header->AlphaMask);

    ASSERT(blue_scan.found);
    ASSERT(red_scan.found);
    ASSERT(green_scan.found);
    ASSERT(alpha_scan.found);

    S32 blue_shift_down = (S32)blue_scan.index;
    S32 green_shift_down = (S32)green_scan.index;
    S32 red_shift_down = (S32)red_scan.index;
    S32 alpha_shift_down = (S32)alpha_scan.index;

    U32* p = (U32*)result.memory;
    for (S32 i = 0; i < pixels_count; ++i)
    {

      Color4F texel = Color4F_from_RGB255(
          ((p[i] & header->RedMask) >> red_shift_down), ((p[i] & header->GreenMask) >> green_shift_down),
          ((p[i] & header->BlueMask) >> blue_shift_down), ((p[i] & header->AlphaMask) >> alpha_shift_down));
      texel = Color4F_SRGB_To_Linear(texel);
      texel.rgb *= texel.a;
      ASSERT(texel.r <= texel.a && texel.g <= texel.a && texel.b <= texel.a);
      texel = Color4F_Linear_To_SRGB(texel);

      p[i] = Color4F_To_Color4(texel).u32;
    }

    result.pitch_in_bytes = result.width * BITMAP_BYTES_PER_PIXEL;
#if 0
    result.pitch_in_bytes = -result.width * BITMAP_BYTES_PER_PIXEL;
    result.memory = (U8*)result.memory - result.pitch_in_bytes * (result.height - 1);
#endif
    Free_File_Memory(thread, read_result.contents);
  }
  return result;
}

internal Loaded_Bitmap DEBUG_Load_BMP(Thread_Context* thread, Debug_Platform_Read_Entire_File_Func* Read_Entire_File,
                                      Debug_Platform_Free_File_Memory_Func* Free_File_Memory, Arena* arena,
                                      char* file_name)
{
  Loaded_Bitmap result = DEBUG_Load_BMP(thread, Read_Entire_File, Free_File_Memory, arena, file_name, vec2(0));
  result.align_percentage = vec2(0.5);
  return result;
}

internal Sprite_Sheet DEBUG_Load_SpriteSheet_BMP(Thread_Context* thread,
                                                 Debug_Platform_Read_Entire_File_Func* Read_Entire_File,
                                                 Debug_Platform_Free_File_Memory_Func* Free_File_Memory, Arena* arena,
                                                 char* file_name, S32 sprite_width, S32 sprite_height,
                                                 Vec2 align = vec2(0))
{
  ASSERT(sprite_width > 0);
  ASSERT(sprite_height > 0);

  Sprite_Sheet result = {};
  result.bitmap = DEBUG_Load_BMP(thread, Read_Entire_File, Free_File_Memory, arena, file_name, vec2(0));

  result.sprite_height = sprite_height;
  result.sprite_width = sprite_width;

  ASSERT(result.bitmap.width % sprite_width == 0);
  ASSERT(result.bitmap.height % sprite_height == 0);

  result.sprite_count = (result.bitmap.width / sprite_width) * (result.bitmap.height / sprite_height);
  result.sprite_bitmaps = Push_Array(arena, (U64)result.sprite_count, Loaded_Bitmap);

  S32 sprite_index = 0;
  for (S32 y = result.bitmap.height - sprite_height; y >= 0; y -= sprite_height)
  {
    for (S32 x = 0; x < result.bitmap.width; x += sprite_width)
    {
      ASSERT(sprite_index < result.sprite_count);
      Loaded_Bitmap* sprite_bitmap = &result.sprite_bitmaps[sprite_index++];
      sprite_bitmap->width = sprite_width;
      sprite_bitmap->height = sprite_height;
      sprite_bitmap->width_over_height = Safe_Ratio0((F32)sprite_width, (F32)sprite_height);
      sprite_bitmap->pitch_in_bytes = result.bitmap.pitch_in_bytes;
      sprite_bitmap->memory = (U8*)result.bitmap.memory + y * sprite_bitmap->pitch_in_bytes + x * (S32)sizeof(U32);
      sprite_bitmap->align_percentage = Top_Down_Align_Percentage(sprite_width, sprite_height, align);
    }
  }
  ASSERT(sprite_index == result.sprite_count); // sanity check
  return result;
}

struct Add_Low_Entity_Result
{
  Low_Entity* low;
  U32 low_index;
};

internal Add_Low_Entity_Result Add_Low_Entity(Game_State* game_state, Entity_Type type, World_Position pos)
{
  ASSERT(game_state->low_entity_count < Array_Count(game_state->low_entities));
  U32 entity_index = game_state->low_entity_count++;
  Low_Entity* entity_low = game_state->low_entities + entity_index;

  *entity_low = {};
  entity_low->sim.type = type;
  entity_low->pos = Null_Position();
  entity_low->sim.collision = game_state->null_collision;

  Change_Entity_Location(&game_state->world_arena, game_state->world, entity_index, entity_low, pos);

  Add_Low_Entity_Result result;
  result.low = entity_low;
  result.low_index = entity_index;

  return result;
}

internal World_Position Chunk_Position_From_Tile_Position(World* world, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z,
                                                          Vec3 additional_offset = vec3(0))

{
  World_Position base_pos = {};

  // NOTE: This is for world gen only really
  F32 tile_size_in_meters = TILE_SIZE_IN_METERS;
  F32 tile_depth_in_meters = TILE_DEPTH_IN_METERS;

  Vec3 tile_dim = vec3(tile_size_in_meters, tile_size_in_meters, tile_depth_in_meters);
  Vec3 offset = tile_dim * vec3((F32)abs_tile_x, (F32)abs_tile_y, (F32)abs_tile_z);

  World_Position result = Map_Into_Chunk_Space(world, base_pos, offset + additional_offset);

  ASSERT(Is_Canonical(world, result.offset_));

  return result;
}

internal Add_Low_Entity_Result Add_Grounded_Entity(Game_State* game_state, Entity_Type type, World_Position pos,
                                                   Sim_Entity_Collision_Volume_Group* collision)

{
  Add_Low_Entity_Result result = Add_Low_Entity(game_state, type, pos);
  result.low->sim.collision = collision;
  return result;
}

internal Add_Low_Entity_Result Add_Sword(Game_State* game_state)

{
  Add_Low_Entity_Result entity =
      Add_Grounded_Entity(game_state, ENTITY_TYPE_SWORD, Null_Position(), game_state->sword_collision);

  Set_Flag(&entity.low->sim, ENTITY_FLAG_MOVEABLE);
  return entity;
}

internal Add_Low_Entity_Result Add_Player(Game_State* game_state)
{
  World_Position pos = game_state->camera_pos;
  Add_Low_Entity_Result entity = Add_Grounded_Entity(game_state, ENTITY_TYPE_PLAYER, pos, game_state->player_collision);

  entity.low->sim.max_health = 25;
  entity.low->sim.health = entity.low->sim.max_health;

  Set_Flag(&entity.low->sim, ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVEABLE);

  Add_Low_Entity_Result sword = Add_Sword(game_state);
  entity.low->sim.sword.index = sword.low_index;

  if (game_state->camera_follow_entity_index == 0)
  {
    game_state->camera_follow_entity_index = entity.low_index;
  }
  return entity;
}

internal Add_Low_Entity_Result Add_Space(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)
{

  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity =
      Add_Grounded_Entity(game_state, ENTITY_TYPE_SPACE, pos, game_state->standard_room_collision);

  Set_Flag(&entity.low->sim, ENTITY_FLAG_TRAVERSABLE);

  return entity;
}

internal Add_Low_Entity_Result Add_Wall(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)
{

  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity = Add_Grounded_Entity(game_state, ENTITY_TYPE_WALL, pos, game_state->wall_collision);

  Set_Flag(&entity.low->sim, ENTITY_FLAG_COLLIDES);

  return entity;
}

internal Add_Low_Entity_Result Add_Stair(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)
{
  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity = Add_Grounded_Entity(game_state, ENTITY_TYPE_STAIR, pos, game_state->stair_collision);

  entity.low->sim.walkable_height = game_state->typical_floor_height;
  entity.low->sim.walkable_dim = entity.low->sim.collision->total_volume.dim.xy;
  Set_Flag(&entity.low->sim, ENTITY_FLAG_COLLIDES);

  return entity;
}

internal U32 Add_Monster(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)

{
  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity =
      Add_Grounded_Entity(game_state, ENTITY_TYPE_MONSTER, pos, game_state->monster_collision);

  entity.low->sim.max_health = 30;
  entity.low->sim.health = entity.low->sim.max_health;

  Set_Flag(&entity.low->sim, ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVEABLE);

  return entity.low_index;
}

internal U32 Add_Familiar(Game_State* game_state, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)

{
  World_Position pos = Chunk_Position_From_Tile_Position(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
  Add_Low_Entity_Result entity =
      Add_Grounded_Entity(game_state, ENTITY_TYPE_FAMILIAR, pos, game_state->familiar_collision);

  entity.low->sim.max_health = 12;
  entity.low->sim.health = entity.low->sim.max_health / 2;

  return entity.low_index;
}

internal inline void Render_Health(Render_Group* group, Sim_Entity* entity, F32 bar_scale = 1.f,
                                   F32 bar_width_scale = 1.f)

{
  F32 width = 1.f;
  F32 offset_top = 1.2f * bar_scale;

  F32 height = 0.13f;
  Vec2 scale = vec2(bar_width_scale * bar_scale, 1.f);
  F32 health_ratio = (F32)entity->health / (F32)entity->max_health;

  // Move hp bar above unit
  Vec3 max_hp_offset = vec3(0, offset_top, 0);

  Vec2 max_hp_bar_dim = vec2(width, height) * scale;

  Vec3 hp_offset = max_hp_offset;
  hp_offset.x += (1.f - health_ratio) * 0.5f * max_hp_bar_dim.x;

  Vec2 hp_bar_dim = vec2(max_hp_bar_dim.x * health_ratio, max_hp_bar_dim.y);

  // red hp background
  Push_Render_Rectangle(group, max_hp_bar_dim, max_hp_offset, vec4(0.8f, 0.2f, 0.2f, 1.f));
  // green hp
  Push_Render_Rectangle(group, hp_bar_dim, hp_offset, vec4(0.2f, 0.8f, 0.2f, 1.f));
}

internal void Fill_Ground_Chunk(Transient_State* transient_state, Game_State* game_state, Ground_Buffer* ground_buffer,
                                World_Position* chunk_pos)
{
  Temporary_Memory ground_memory = Begin_Temp_Memory(&transient_state->transient_arena);

  Loaded_Bitmap* buffer = &ground_buffer->bitmap;
  buffer->align_percentage = vec2(0.5f);
  buffer->width_over_height = 1.f;
  Render_Group* render_group =
      Allocate_Render_Group(&transient_state->transient_arena, Megabytes(4), (F32)buffer->width, (F32)buffer->height);

  Push_Render_Clear(render_group, Color_Pastel_Yellow);

  ground_buffer->pos = *chunk_pos;
#if 1
  Vec2 dim = Rect_Get_Dim(Get_Camera_Rect_At_Target(render_group));
  F32 width = dim.x;
  F32 height = dim.y;
  Vec2 half_dim = 0.5f * dim;

  for (S32 chunk_offset_y = -1; chunk_offset_y <= 1; ++chunk_offset_y)
  {
    for (S32 chunk_offset_x = -1; chunk_offset_x <= 1; ++chunk_offset_x)
    {

      S32 chunk_x = chunk_pos->chunk_x + chunk_offset_x;
      S32 chunk_y = chunk_pos->chunk_y + chunk_offset_y;
      S32 chunk_z = chunk_pos->chunk_z;

      // TODO: look into wang hashing
      Random_Series series = Seed(397 * (U32)chunk_x + 503 * (U32)chunk_y + 37 * (U32)chunk_z);

      Vec2 center = vec2(width * (F32)chunk_offset_x, height * (F32)chunk_offset_y);

      for (U32 grass_index = 0; grass_index < 90; ++grass_index)
      {
        Loaded_Bitmap* stamp;
        if (Random_0_To_1(&series) > 0.25f && chunk_pos->chunk_z >= 0)
        {

          stamp = game_state->grass + Random_Choice(&series, Array_Count(game_state->grass));
        }
        else
        {
          stamp = game_state->ground + Random_Choice(&series, Array_Count(game_state->ground));
        }

        Vec2 offset = half_dim * vec2(Random_Neg1_To_1(&series), Random_Neg1_To_1(&series));
        Vec2 pos = offset + center;

        // Draw_BMP(buffer, stamp, pos.x, pos.y, 1.f);
        Push_Render_Bitmap(render_group, stamp, vec3(pos, 0), 18.f);
      }
    }
  }
  for (S32 chunk_offset_y = -1; chunk_offset_y <= 1; ++chunk_offset_y)
  {
    for (S32 chunk_offset_x = -1; chunk_offset_x <= 1; ++chunk_offset_x)
    {

      S32 chunk_x = chunk_pos->chunk_x + chunk_offset_x;
      S32 chunk_y = chunk_pos->chunk_y + chunk_offset_y;
      S32 chunk_z = chunk_pos->chunk_z;

      // TODO: look into wang hashing
      Random_Series series = Seed(997 * (U32)chunk_x + 503 * (U32)chunk_y + 11 * (U32)chunk_z);

      Vec2 center = vec2(width * (F32)chunk_offset_x, height * (F32)chunk_offset_y);

      for (U32 grass_index = 0; grass_index < 40; ++grass_index)
      {
        Loaded_Bitmap* stamp;

        stamp = game_state->tuft + Random_Choice(&series, Array_Count(game_state->tuft));

        Vec2 offset = half_dim * vec2(Random_Neg1_To_1(&series), Random_Neg1_To_1(&series));
        Vec2 pos = offset + center;

        Push_Render_Bitmap(render_group, stamp, vec3(pos, 0), 2.5f);
      }
    }
  }
#endif
  Render_Group_To_Output(render_group, buffer);
  End_Temp_Memory(&transient_state->transient_arena, ground_memory);
}

internal void Clear_Collision_Rules_For(Game_State* game_state, U32 storage_index)
{
  for (U32 rule_pp_index = 0; rule_pp_index < Array_Count(game_state->collision_rule_hash); ++rule_pp_index)
  {
    for (Pairwise_Collision_Rule** rule_pp = &game_state->collision_rule_hash[rule_pp_index]; *rule_pp;)
    {
      if (((*rule_pp)->storage_index_a == storage_index) || ((*rule_pp)->storage_index_b == storage_index))
      {
        Pairwise_Collision_Rule* rule_to_delete = *rule_pp;
        // Delete
        *rule_pp = (*rule_pp)->next_in_hash;
        game_state->collision_rule_count--;

        // push to free list
        rule_to_delete->next_in_hash = game_state->first_free_collision_rule;
        game_state->first_free_collision_rule = rule_to_delete;
      }
      else
      {
        rule_pp = &(*rule_pp)->next_in_hash;
      }
    }
  }
}

internal B32 Remove_Collision_Rule(Game_State* game_state, U32 storage_index_a, U32 storage_index_b)
{
  return false;
}

internal B32 Add_Collision_Rule(Game_State* game_state, U32 storage_index_a, U32 storage_index_b, B32 should_collide)
{
  B32 added = false;
  if (storage_index_a > storage_index_b)

  {
    U32 temp = storage_index_a;
    storage_index_a = storage_index_b;
    storage_index_b = temp;
  }

  Pairwise_Collision_Rule* found = 0;
  U32 hash_bucket = storage_index_a & (Array_Count(game_state->collision_rule_hash) - 1);
  for (Pairwise_Collision_Rule* rule = game_state->collision_rule_hash[hash_bucket]; rule; rule = rule->next_in_hash)
  {
    if ((rule->storage_index_a == storage_index_a) && (rule->storage_index_b == storage_index_b))
    {
      found = rule;
      break;
    }
  }
  if (!found)
  {
    found = game_state->first_free_collision_rule;
    if (found)
    {
      game_state->first_free_collision_rule = found->next_in_hash;
    }
    else
    {
      found = Push_Struct(&game_state->world_arena, Pairwise_Collision_Rule);
    }

    found->next_in_hash = game_state->collision_rule_hash[hash_bucket];
    game_state->collision_rule_hash[hash_bucket] = found;
    game_state->collision_rule_count++;

    added = true;
  }

  if (found)
  {
    found->storage_index_a = storage_index_a;
    found->storage_index_b = storage_index_b;
    found->should_collide = should_collide;
  }
  return added;
}

inline Sim_Entity_Collision_Volume_Group* Make_Null_Collision_Volume(Game_State* game_state)
{
  Sim_Entity_Collision_Volume_Group* result = Push_Struct(&game_state->world_arena, Sim_Entity_Collision_Volume_Group);
  result->volumes = 0;
  result->volume_count = 0;
  result->total_volume.dim = vec3(0);
  result->total_volume.offset_pos = vec3(0);

  return result;
}

inline Sim_Entity_Collision_Volume_Group* Make_Simple_Grounded_Collision_Volume(Game_State* game_state, Vec3 dim)
{
  Sim_Entity_Collision_Volume_Group* result = Push_Struct(&game_state->world_arena, Sim_Entity_Collision_Volume_Group);
  result->volume_count = 1;
  result->volumes = Push_Array(&game_state->world_arena, result->volume_count, Sim_Entity_Collision_Volume);
  result->volumes[0].dim = dim;
  result->volumes[0].offset_pos = vec3(0, 0, 0.5f * dim.z);
  result->total_volume = result->volumes[0];

  return result;
}

internal void Clear_Bitmap(Loaded_Bitmap* bitmap)
{
  if (bitmap->memory)
  {
    S32 total_bitmap_size = bitmap->width * bitmap->height * BITMAP_BYTES_PER_PIXEL;
    Zero_Size((size_t)total_bitmap_size, bitmap->memory);
  }
}

internal Loaded_Bitmap Make_Empty_Bitmap(Arena* arena, S32 width, S32 height, B32 clear_to_zero = false)
{
  Loaded_Bitmap result = {};
  result.width = width;
  result.height = height;
  result.pitch_in_bytes = width * BITMAP_BYTES_PER_PIXEL;
  S32 total_bitmap_size = width * height * BITMAP_BYTES_PER_PIXEL;
  result.memory = Push_Size(arena, (size_t)total_bitmap_size);
  if (clear_to_zero)
  {
    Clear_Bitmap(&result);
  }

  return result;
}

internal void Make_Sphere_Diffuse_Map(Loaded_Bitmap* bitmap, F32 cx = 1.f, F32 cy = 1.f)

{
  F32 inv_width = 1.f / (F32)(bitmap->width - 1);
  F32 inv_height = 1.f / (F32)(bitmap->height - 1);

  U8* row_in_bytes = (U8*)bitmap->memory;
  for (S32 y = 0; y < bitmap->height; ++y)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;
    for (S32 x = 0; x < bitmap->width; ++x)
    {

      Vec2 bitmap_uv = vec2(inv_width * (F32)x, inv_height * (F32)y);

      Vec2 snorm = 2.f * bitmap_uv - vec2(1.f, 1.f);
      snorm *= vec2(cx, cy);

      F32 dist_sq = Vec_Length_Sq(snorm);

      Vec3 base_color = vec3(0.f, 0.f, 0.f);

      F32 alpha = 0.f;
      if (dist_sq <= 1.f)
      {
        alpha = 1.f;
      }

      Vec4 color = vec4(alpha * base_color, alpha);

      *pixel++ = Color4F_To_Color4(color).u32;
    }
    row_in_bytes += bitmap->pitch_in_bytes;
  }
}

internal void Make_Sphere_Normal_Map(Loaded_Bitmap* bitmap, F32 roughness, F32 cx = 1.f, F32 cy = 1.f)

{
  F32 inv_width = 1.f / (F32)(bitmap->width - 1);
  F32 inv_height = 1.f / (F32)(bitmap->height - 1);

  U8* row_in_bytes = (U8*)bitmap->memory;
  for (S32 y = 0; y < bitmap->height; ++y)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;
    for (S32 x = 0; x < bitmap->width; ++x)
    {

      Vec2 bitmap_uv = vec2(inv_width * (F32)x, inv_height * (F32)y);

      Vec2 snorm = 2.f * bitmap_uv - vec2(1.f, 1.f);
      snorm *= vec2(cx, cy);

      F32 dist_sq = Vec_Length_Sq(snorm);

      Vec3 normal = vec3(0.f, SQRT2_2F, SQRT2_2F);

      if (dist_sq <= 1.f)
      {
        normal = vec3(snorm.x, snorm.y, Sqrt_F32(1.f - dist_sq));
        normal = Vec_Normalize(normal);
      }

      Vec4 color = vec4(Snorm_To_Unorm(normal.x), Snorm_To_Unorm(normal.y), Snorm_To_Unorm(normal.z), roughness);

      *pixel++ = Color4F_To_Color4(color).u32;
    }
    row_in_bytes += bitmap->pitch_in_bytes;
  }
}

internal void Make_Pyramid_Normal_Map(Loaded_Bitmap* bitmap, F32 roughness)
{
  U8* row_in_bytes = (U8*)bitmap->memory;
  for (S32 y = 0; y < bitmap->height; ++y)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;
    for (S32 x = 0; x < bitmap->width; ++x)
    {

      S32 inv_x = (bitmap->width - 1) - x;

      Vec3 normal = vec3(0.f, SQRT2_2F, SQRT2_2F);

      if (x < y)
      {
        if (inv_x < y)
        {
          // bottom face
          normal = vec3(0, SQRT2_2F, SQRT2_2F);
        }
        else
        {
          // left face
          normal = vec3(-SQRT2_2F, 0.0f, SQRT2_2F);
        }
      }
      else
      {
        if (inv_x < y)
        {
          // right face
          normal = vec3(SQRT2_2F, 0.f, SQRT2_2F);
        }
        else
        {
          // top face
          normal = vec3(0.f, -SQRT2_2F, SQRT2_2F);
        }
      }

      Vec4 color = vec4(Snorm_To_Unorm(normal.x), Snorm_To_Unorm(normal.y), Snorm_To_Unorm(normal.z), roughness);

      *pixel++ = Color4F_To_Color4(color).u32;
    }
    row_in_bytes += bitmap->pitch_in_bytes;
  }
}

inline Loaded_Bitmap* Get_Sprite_Sheet_Sprite_Bitmap(Sprite_Sheet* sprite_sheet, U32 sprite_index)
{
  ASSERT(sprite_index < (U32)sprite_sheet->sprite_count);
  Loaded_Bitmap* result = sprite_sheet->sprite_bitmaps + sprite_index;
  return result;
}

internal void Request_Ground_Buffers(World_Position center_pos, Rect3 bounds)
{
  bounds = Rect_Add_Offset(bounds, center_pos.offset_);
  center_pos.offset_ = vec3(0);

  // TODO: test fill
  // Fill_Ground_Chunk(transient_state, game_state, transient_state->ground_buffers, &game_state->camera_pos);
}
#if CENGINE_INTERNAL
Game_Memory* debug_global_memory;
#endif
extern "C" GAME_UPDATE_AND_RENDER(Game_Update_And_Render)
{
#if CENGINE_INTERNAL
  debug_global_memory = memory;
#endif

  BEGIN_TIMED_BLOCK(Game_Update_And_Render);
  ASSERT((&input->controllers[0].button_count - &input->controllers[0].buttons[0]) ==
         (Array_Count(input->controllers[0].buttons)));
  ASSERT(sizeof(Game_State) < memory->permanent_storage_size);

  S32 ground_buffer_width = 256;
  S32 ground_buffer_height = 256;

  Game_State* game_state = (Game_State*)memory->permanent_storage;
  if (!memory->is_initialized)
  {

    Initialize_Arena(&game_state->world_arena, memory->permanent_storage_size - sizeof(Game_State),
                     (U8*)memory->permanent_storage + sizeof(Game_State));

    Arena* world_arena = &game_state->world_arena;

    game_state->typical_floor_height = 3.f;

    // TODO: get this from renderer
    // TODO: remove pixels to meters
    F32 pixels_to_meters = 1.f / 42.f;
    Vec3 world_chunk_dim_in_meters =
        vec3((F32)ground_buffer_width * pixels_to_meters, (F32)ground_buffer_height * pixels_to_meters,
             game_state->typical_floor_height);

    game_state->world = Push_Struct(world_arena, World);
    World* world = game_state->world;
    Initialize_World(world, world_chunk_dim_in_meters);

    // F32 tile_size_in_pixels = TILE_SIZE_IN_PIXELS;
    F32 tile_size_in_meters = TILE_SIZE_IN_METERS;
    F32 tile_depth_in_meters = game_state->typical_floor_height;

    game_state->null_collision = Make_Null_Collision_Volume(game_state);
    game_state->sword_collision = Make_Simple_Grounded_Collision_Volume(game_state, vec3(0.5f, 0.5f, 0.1f));
    game_state->player_collision = Make_Simple_Grounded_Collision_Volume(game_state, vec3(1.f, 0.6f, 0.7f));
    game_state->stair_collision = Make_Simple_Grounded_Collision_Volume(
        game_state, vec3(tile_size_in_meters, 2.f * tile_size_in_meters, 1.001f * tile_depth_in_meters));
    game_state->familiar_collision = Make_Simple_Grounded_Collision_Volume(game_state, vec3(0.3f, 0.3f, 0.3f));
    game_state->wall_collision = Make_Simple_Grounded_Collision_Volume(
        game_state, vec3(tile_size_in_meters, tile_size_in_meters, tile_depth_in_meters * 0.99f));
    game_state->monster_collision = Make_Simple_Grounded_Collision_Volume(game_state, vec3(0.95f, 0.5f, 1.f));
    game_state->standard_room_collision = Make_Simple_Grounded_Collision_Volume(
        game_state, vec3(tile_size_in_meters * TILES_PER_WIDTH, tile_size_in_meters * TILES_PER_HEIGHT,
                         0.9f * tile_depth_in_meters * TILES_PER_DEPTH));

    // NOTE: first entity is NULL entity
    Add_Low_Entity(game_state, ENTITY_TYPE_NULL, Null_Position());

    Vec2 shadow_align = vec2(6, 1);
    Vec2 sprite_align = vec2(8, 8);
    Vec2 unit_align = vec2(8, 12);
    // Vec2 unit_align_4x = vec2(32, 48);
    Vec2 sprite1x2_align = vec2(8, 23);

    game_state->knight_sprite_sheet = DEBUG_Load_SpriteSheet_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                                                 memory->DEBUG_Platform_Free_File_Memory, world_arena,
                                                                 "assets/knight.bmp", 16, 16, unit_align);

    game_state->slime_sprite_sheet = DEBUG_Load_SpriteSheet_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                                                memory->DEBUG_Platform_Free_File_Memory, world_arena,
                                                                "assets/slime1.bmp", 16, 16, unit_align);
    game_state->eye_sprite_sheet = DEBUG_Load_SpriteSheet_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                                              memory->DEBUG_Platform_Free_File_Memory, world_arena,
                                                              "assets/eye.bmp", 16, 16, sprite_align);

    game_state->grass_sprite_sheet = DEBUG_Load_SpriteSheet_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                                                memory->DEBUG_Platform_Free_File_Memory, world_arena,
                                                                "assets/grass_tileset.bmp", 16, 16);

    game_state->shadow_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/shadow.bmp", shadow_align);
    game_state->stone_floor_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/stone_floor.bmp");
    game_state->stair_up_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/stair_up.bmp");
    game_state->stair_down_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/stair_down.bmp");
    game_state->test_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/structured_art.bmp");
    game_state->wall1_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/rock.bmp", sprite_align);
    game_state->wall2_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/stone_wall.bmp", sprite_align);
    game_state->pillar_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/pillar.bmp", sprite1x2_align);
    game_state->shuriken_bmp =
        DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File, memory->DEBUG_Platform_Free_File_Memory,
                       world_arena, "assets/shuriken.bmp", sprite_align);

    game_state->test_tree_bmp = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                               memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/tree00.bmp");

    game_state->grass[0] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                          memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/grass00.bmp");
    game_state->grass[1] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                          memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/grass01.bmp");
    game_state->ground[0] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                           memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/ground00.bmp");

    game_state->ground[1] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                           memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/ground01.bmp");

    game_state->ground[2] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                           memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/ground02.bmp");

    game_state->ground[3] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                           memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/ground03.bmp");
    game_state->tuft[0] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                         memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/tuft00.bmp");

    game_state->tuft[1] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                         memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/tuft01.bmp");

    game_state->tuft[2] = DEBUG_Load_BMP(thread, memory->DEBUG_Platform_Read_Entire_File,
                                         memory->DEBUG_Platform_Free_File_Memory, world_arena, "test/tuft02.bmp");

    Random_Series series = {1234};

    S32 screen_base_x = 0;
    S32 screen_base_y = 0;
    S32 screen_base_z = 0;

    S32 screen_x = screen_base_x;
    S32 screen_y = screen_base_y;
    S32 abs_tile_z = screen_base_z;

    B32 door_left = false;
    B32 door_right = false;
    B32 door_top = false;
    B32 door_bottom = false;
    B32 door_up = false;
    B32 door_down = false;

    for (U32 screen_index = 0; screen_index < 1000; ++screen_index)
    {

      U32 door_direction = Random_Choice(&series, (door_up || door_down) ? 2 : 4);

      B32 created_z_door = false;
      if (door_direction == 3)
      {
        created_z_door = true;
        door_down = true;
      }
      else if (door_direction == 2)
      {
        created_z_door = true;
        door_up = true;
      }
      else if (door_direction == 1)
      {
        door_right = true;
      }
      else
      {
        door_top = true;
      }

      Add_Space(game_state, (screen_x * TILES_PER_WIDTH + TILES_PER_WIDTH / 2),
                (screen_y * TILES_PER_HEIGHT + TILES_PER_HEIGHT / 2), abs_tile_z);

      for (S32 tile_y = 0; tile_y < TILES_PER_HEIGHT; ++tile_y)
      {
        for (S32 tile_x = 0; tile_x < TILES_PER_WIDTH; ++tile_x)
        {
          S32 abs_tile_x = screen_x * TILES_PER_WIDTH + tile_x;
          S32 abs_tile_y = screen_y * TILES_PER_HEIGHT + tile_y;

          B32 wall = false;
          if ((tile_x == 0) && (!door_left || (tile_y != TILES_PER_HEIGHT / 2)))
          {
            wall = true;
          }
          if ((tile_x == (TILES_PER_WIDTH - 1)) && (!door_right || (tile_y != TILES_PER_HEIGHT / 2)))
          {
            wall = true;
          }

          if ((tile_y == 0) && (!door_bottom || (tile_x != TILES_PER_WIDTH / 2)))
          {
            wall = true;
          }
          if ((tile_y == (TILES_PER_HEIGHT - 1)) && (!door_top || (tile_x != TILES_PER_WIDTH / 2)))
          {
            wall = true;
          }
          if (wall)
          {
            Add_Wall(game_state, abs_tile_x, abs_tile_y, abs_tile_z);
          }
          else if (created_z_door)
          {
            if ((tile_x == 10) && (tile_y == 5))
            {

              Add_Stair(game_state, abs_tile_x + (2 * (S32)(screen_index % 2)), abs_tile_y,
                        door_down ? abs_tile_z - 1 : abs_tile_z);
            }
          }
        }
      }
      door_left = door_right;
      door_bottom = door_top;
      door_right = false;
      door_top = false;
      if (door_direction == 2)
      {
        door_down = !door_down;
        door_up = !door_up;
      }
      else
      {
        door_up = door_down = false;
      }

      if (door_direction == 3)
      {
        abs_tile_z -= 1;
      }
      else if (door_direction == 2)
      {
        abs_tile_z += 1;
      }
      else if (door_direction == 1)

      {
        screen_x += 1;
      }
      else
      {
        screen_y += 1;
      }
    }

    World_Position new_camera_pos = {};
    S32 camera_tile_x = screen_base_x * TILES_PER_WIDTH + 17 / 2;
    S32 camera_tile_y = screen_base_y * TILES_PER_HEIGHT + 9 / 2;
    S32 camera_tile_z = screen_base_z;

    new_camera_pos = Chunk_Position_From_Tile_Position(world, camera_tile_x, camera_tile_y, camera_tile_z);

    Add_Monster(game_state, camera_tile_x + 2, camera_tile_y + 2, camera_tile_z);
    for (U32 familiar_index = 0; familiar_index < 3; ++familiar_index)
    {
      S32 x = camera_tile_x + Random_Between(&series, -7, 7);
      S32 y = camera_tile_y + Random_Between(&series, -3, 1);

      Add_Familiar(game_state, x, y, camera_tile_z);
    }

    game_state->camera_pos = new_camera_pos;

    memory->is_initialized = true;
  }

  ASSERT(sizeof(Transient_State) < memory->transient_storage_size);
  Transient_State* transient_state = (Transient_State*)memory->transient_storage;
  if (!transient_state->is_initialized)
  {
    Initialize_Arena(&transient_state->transient_arena, memory->transient_storage_size - sizeof(Transient_State),
                     (U8*)memory->transient_storage + sizeof(Transient_State));

    transient_state->ground_buffer_count = 128; // 128
    transient_state->ground_buffers =
        Push_Array(&transient_state->transient_arena, transient_state->ground_buffer_count, Ground_Buffer);
    transient_state->ground_bitmap_template =
        Make_Empty_Bitmap(&transient_state->transient_arena, ground_buffer_width, ground_buffer_height, true);
    for (U32 ground_buffer_index = 0; ground_buffer_index < transient_state->ground_buffer_count; ++ground_buffer_index)
    {
      Ground_Buffer* ground_buffer = transient_state->ground_buffers + ground_buffer_index;

      ground_buffer->bitmap =
          Make_Empty_Bitmap(&transient_state->transient_arena, ground_buffer_width, ground_buffer_height, true);
      ground_buffer->pos = Null_Position();
    }

    game_state->test_diffuse = Make_Empty_Bitmap(&transient_state->transient_arena, 256, 256);
    Draw_Rect(&game_state->test_diffuse, vec2(0),
              vec2i(game_state->test_diffuse.width, game_state->test_diffuse.height), vec4(0.5, 0.5, 0.5, 1.f));
    game_state->test_normal = Make_Empty_Bitmap(&transient_state->transient_arena, game_state->test_diffuse.width,
                                                game_state->test_diffuse.height);
    Make_Sphere_Normal_Map(&game_state->test_normal, 0.f);
    Make_Sphere_Diffuse_Map(&game_state->test_diffuse);

    transient_state->env_map_width = 512;
    transient_state->env_map_height = 256;
    for (U32 map_index = 0; map_index < Array_Count(transient_state->env_maps); ++map_index)
    {
      Environment_Map* map = transient_state->env_maps + map_index;
      U32 width = transient_state->env_map_width;
      U32 height = transient_state->env_map_height;
      for (U32 lod_index = 0; lod_index < Array_Count(map->lod); ++lod_index)
      {
        map->lod[lod_index] = Make_Empty_Bitmap(&transient_state->transient_arena, (S32)width, (S32)height);
        width >>= 1;
        height >>= 1;
      }
    }

    transient_state->is_initialized = true;
  }

  if (input->executable_reloaded)
  {
    for (U32 ground_buffer_index = 0; ground_buffer_index < transient_state->ground_buffer_count; ++ground_buffer_index)
    {
      Ground_Buffer* ground_buffer = transient_state->ground_buffers + ground_buffer_index;
      ground_buffer->bitmap =
          Make_Empty_Bitmap(&transient_state->transient_arena, ground_buffer_width, ground_buffer_height, true);
      ground_buffer->pos = Null_Position();
    }
  }

  F32 delta_time = input->delta_time_s;

  for (S32 controller_index = 0; controller_index < (S32)Array_Count(input->controllers); controller_index++)
  {
    Game_Controller_Input* controller = Get_Controller(input, controller_index);
    if (controller->dpad_up.ended_down)
    {
      game_state->volume += 0.001f;
    }
    else if (controller->dpad_down.ended_down)
    {
      game_state->volume -= 0.001f;
    }
    game_state->volume = CLAMP(game_state->volume, 0.0f, 0.5f);

    Controlled_Player* con_player = game_state->controlled_players + controller_index;
    {
      U32 entity_index = con_player->entity_index;
      *con_player = {};
      con_player->entity_index = entity_index;
    }

    if (con_player->entity_index == 0)
    {
      if (controller->start.ended_down)
      {
        *con_player = {};
        con_player->entity_index = Add_Player(game_state).low_index;
      }
    }
    else
    {
      // NOTE: player movement

      con_player->move_dir = controller->stick_left;

      if (controller->action_up.ended_down)
      {
        con_player->sprint = true;
      }
      // NOTE: player jump
      if (controller->start.ended_down)
      {
        con_player->jump_vel = 3.f;
      }

      if (controller->action_up.ended_down)
      {
        con_player->attack_dir = vec2(0.f, 1.f);
      }
      if (controller->action_down.ended_down)
      {
        con_player->attack_dir = vec2(0.f, -1.f);
      }
      if (controller->action_left.ended_down)
      {
        con_player->attack_dir = vec2(-1.f, 0.f);
      }
      if (controller->action_right.ended_down)
      {
        con_player->attack_dir = vec2(1.f, 0.f);
      }
    }
  }

  World* world = game_state->world;

  Temporary_Memory render_memory = Begin_Temp_Memory(&transient_state->transient_arena);

  Loaded_Bitmap draw_buffer_ = {};
  Loaded_Bitmap* draw_buffer = &draw_buffer_;
  draw_buffer->width = buffer->width;
  draw_buffer->height = buffer->height;
  draw_buffer->pitch_in_bytes = buffer->pitch_in_bytes;
  draw_buffer->memory = buffer->memory;

  Render_Group* render_group = Allocate_Render_Group(&transient_state->transient_arena, Megabytes(4),
                                                     (F32)draw_buffer->width, (F32)draw_buffer->height);

  // NOTE: Clear Buffer --------------------------------------------------------
  Push_Render_Clear(render_group, vec4(0.35f, 0.58f, 0.93f, 1.0f));
  // Draw_Rectf(draw_buffer, 0, 0, (F32)draw_buffer->width, (F32)draw_buffer->height, 0.35f, 0.58f, 0.93f);
  //------------------------------------

  Rect2 screen_bounds = Get_Camera_Rect_At_Target(render_group);
  Rect3 camera_bounds_in_meters = Rect_Min_Max(vec3(screen_bounds.min, 0), vec3(screen_bounds.max, 0));
  camera_bounds_in_meters.min.z = -0.f * game_state->typical_floor_height;
  camera_bounds_in_meters.max.z = 1.f * game_state->typical_floor_height;

  // camera_bounds_in_meters =
  //     Rect_Add_Radius(camera_bounds_in_meters, game_state->world->chunk_dim_in_meters.x * vec3(0.5f, 0.5f, 0));
  // NOTE: Draw chunk bounds
  {
    World_Position min_chunk_pos =
        Map_Into_Chunk_Space(world, game_state->camera_pos, Rect_Get_Min_Corner(camera_bounds_in_meters));
    World_Position max_chunk_pos =
        Map_Into_Chunk_Space(world, game_state->camera_pos, Rect_Get_Max_Corner(camera_bounds_in_meters));

    for (S32 chunk_z = min_chunk_pos.chunk_z; chunk_z <= max_chunk_pos.chunk_z; ++chunk_z)
    {
      for (S32 chunk_y = min_chunk_pos.chunk_y; chunk_y <= max_chunk_pos.chunk_y; ++chunk_y)
      {
        for (S32 chunk_x = min_chunk_pos.chunk_x; chunk_x <= max_chunk_pos.chunk_x; ++chunk_x)
        {
          World_Position chunk_center_pos = Centered_Chunk_Point(chunk_x, chunk_y, chunk_z);
          Ground_Buffer* furthest_buffer = 0;
          F32 furthest_distance_from_camera = 0.f;
          for (U32 ground_buffer_index = 0; ground_buffer_index < transient_state->ground_buffer_count;
               ++ground_buffer_index)
          {
            Ground_Buffer* ground_buffer = transient_state->ground_buffers + ground_buffer_index;
            if (Are_In_Same_Chunk(world, &ground_buffer->pos, &chunk_center_pos))
            {
              furthest_buffer = 0;
              break;
            }
            else if (Is_Valid(ground_buffer->pos))
            {
              Vec3 relative_pos = World_Pos_Subtract(world, &ground_buffer->pos, &game_state->camera_pos);
              F32 distance_from_camera = Vec_Length_Sq(relative_pos.xy);
              if (distance_from_camera > furthest_distance_from_camera)
              {
                furthest_distance_from_camera = distance_from_camera;
                furthest_buffer = ground_buffer;
              }
            }
            else
            {
              furthest_buffer = ground_buffer;
              furthest_distance_from_camera = F32_MAX;
            }
          }
          if (furthest_buffer)
          {

            Fill_Ground_Chunk(transient_state, game_state, furthest_buffer, &chunk_center_pos);
          }
        }
      }
    }
  }

  // NOTE: Draw floor
  for (U32 ground_buffer_index = 0; ground_buffer_index < transient_state->ground_buffer_count; ++ground_buffer_index)
  {
    Ground_Buffer* ground_buffer = transient_state->ground_buffers + ground_buffer_index;
    if (Is_Valid(ground_buffer->pos))
    {
      Loaded_Bitmap* bitmap = &ground_buffer->bitmap;

      Vec3 offset = World_Pos_Subtract(game_state->world, &ground_buffer->pos, &game_state->camera_pos);

      // if (ground_buffer->pos.chunk_z == game_state->camera_pos.chunk_z)
      {
        render_group->default_basis = Push_Struct(&transient_state->transient_arena, Render_Basis);
        render_group->default_basis->pos = offset;

        F32 ground_size_in_meters = world->chunk_dim_in_meters.x;
        Push_Render_Bitmap(render_group, bitmap, vec3(0), ground_size_in_meters);

        Push_Render_Rectangle_Pixel_Outline(render_group, world->chunk_dim_in_meters.xy, vec3(0), vec4(1, 1, 0, 1),
                                            2.f);
      }
    }
  }

  // TODO: how big do we want to expand here
  Vec3 sim_bounds_expansion = vec3(world->chunk_dim_in_meters.xy, 0);
  Rect3 sim_bounds = Rect_Add_Radius(camera_bounds_in_meters, sim_bounds_expansion);

  Temporary_Memory sim_memory = Begin_Temp_Memory(&transient_state->transient_arena);
  World_Position sim_center_pos = game_state->camera_pos;
  Sim_Region* sim_region = Begin_Sim(&transient_state->transient_arena, game_state, game_state->world, sim_center_pos,
                                     sim_bounds, input->delta_time_s);
  Vec3 camera_pos = World_Pos_Subtract(world, &game_state->camera_pos, &sim_center_pos);

  render_group->default_basis = Push_Struct(&transient_state->transient_arena, Render_Basis);
  render_group->default_basis->pos = vec3(0);
  Push_Render_Rectangle_Outline(render_group, Rect_Get_Dim(screen_bounds), vec3(0), vec4(1), 0.2f);

  Push_Render_Rectangle_Outline(render_group, Rect_Get_Dim(sim_bounds).xy, vec3(0), Color_Pastel_Red);
  Push_Render_Rectangle_Outline(render_group, Rect_Get_Dim(sim_region->update_bounds).xy, vec3(0), Color_Pastel_Green);
  Push_Render_Rectangle_Outline(render_group, Rect_Get_Dim(sim_region->bounds).xy, vec3(0), Color_Pastel_Pink);

  // NOTE:Draw entities
  for (U32 entity_index = 0; entity_index < sim_region->entity_count; ++entity_index)
  {
    Sim_Entity* entity = sim_region->entities + entity_index;

    if (!entity->updatable)
    {
      continue;
    }

    // TODO: bad design, this should be computed after update
    F32 base_z = entity->pos.z;
    F32 shadow_alpha = CLAMP((1.0f - 0.8f * base_z), 0.5f, 0.8f);
    F32 shadow_scale = shadow_alpha * 0.2f;

    Move_Spec move_spec = Default_Move_Spec();
    Vec3 accel = {};

    Render_Basis* basis = Push_Struct(&transient_state->transient_arena, Render_Basis);
    render_group->default_basis = basis;

    // TODO: this probably indicates we want to separate update and render for entities soon
    Vec3 camera_relative_ground_pos = Get_Entity_Ground_Point(entity) - camera_pos;
    F32 fh = game_state->typical_floor_height;
    F32 fade_top_end_z = 0.75f * fh;
    F32 fade_top_start_z = 0.1f * fh;
    F32 fade_bottom_start_z = -1.1f * fh;
    F32 fade_bottom_end_z = -3.25f * fh;

    render_group->global_alpha = 1.f;
    if (camera_relative_ground_pos.z > fade_top_start_z)
    {
      render_group->global_alpha =
          1.f - Clamp01_Map_To_Range(fade_top_start_z, fade_top_end_z, camera_relative_ground_pos.z);
    }
    else if (camera_relative_ground_pos.z < fade_bottom_start_z)
    {
      render_group->global_alpha =
          Clamp01_Map_To_Range(fade_bottom_end_z, fade_bottom_start_z, camera_relative_ground_pos.z);
    }

    switch (entity->type)
    {
      case ENTITY_TYPE_PLAYER:
      {
        for (U32 control_index = 0; control_index < Array_Count(game_state->controlled_players); ++control_index)
        {
          Controlled_Player* con_player = game_state->controlled_players + control_index;
          if (entity->storage_index == con_player->entity_index)
          {
            if (entity->vel.z == 0.f && con_player->jump_vel > 0.f)

            {
              // entity->vel.z = con_player->jump_vel;
            }

            move_spec.normalize_accel = true;
            move_spec.drag = 8.f;
            move_spec.speed = 50.f;
            move_spec.speed *= con_player->sprint ? 3.f : 1.f;
            accel = vec3(con_player->move_dir, 0.f);

            if (Vec_Length_Sq(con_player->attack_dir) > 0.f)
            {
              Sim_Entity* sword = entity->sword.ptr;
              if (sword && Has_Flag(sword, ENTITY_FLAG_NONSPATIAL))
              {
                sword->pos = entity->pos;
                sword->distance_limit = 5.f;
                Vec3 vel = vec3(10.f * con_player->attack_dir, 0.f);
                Make_Entity_Spatial(sword, entity->pos, vel);
                Add_Collision_Rule(game_state, entity->storage_index, sword->storage_index, false);
              }
            }
          }
        }

        F32 height = 1.3f;
        Push_Render_Bitmap(render_group, &game_state->shadow_bmp, vec3(0), shadow_scale, vec4(1, 1, 1, shadow_alpha));
        Loaded_Bitmap* knight_bmp =
            Get_Sprite_Sheet_Sprite_Bitmap(&game_state->knight_sprite_sheet, entity->sprite_index);
        Push_Render_Bitmap(render_group, knight_bmp, vec3(0), height);

        Render_Health(render_group, entity, 1.f);
      }
      break;
      case ENTITY_TYPE_MONSTER:
      {

        entity->sprite_index++;
        entity->sprite_index = entity->sprite_index % ((S32)E_SPRITE_WALK_COUNT);

        Push_Render_Bitmap(render_group, &game_state->shadow_bmp, vec3(0), shadow_scale, vec4(1, 1, 1, shadow_alpha));

        F32 height = 1.2f;
        Loaded_Bitmap* sprite_bmp =
            Get_Sprite_Sheet_Sprite_Bitmap(&game_state->slime_sprite_sheet, entity->sprite_index);
        Push_Render_Bitmap(render_group, sprite_bmp, vec3(0), height);
        Render_Health(render_group, entity, 1.f);
      }
      break;
      case ENTITY_TYPE_FAMILIAR:
      {
        Sim_Entity* closest_player = {};
        F32 closest_player_distsq = Square(10.f); // NOTE: 10m max search

        // TODO: make spatial queries easy for things!
        Sim_Entity* test_entity = sim_region->entities;
        for (U32 test_entity_index = 0; test_entity_index < sim_region->entity_count;
             ++test_entity_index, ++test_entity)

        {
          if (test_entity->type == ENTITY_TYPE_PLAYER)
          {
            F32 test_distsq = Vec_Length_Sq(test_entity->pos - entity->pos);
            if (test_distsq < closest_player_distsq)
            {
              closest_player_distsq = test_distsq;
              closest_player = test_entity;
            }
          }
        }

        // NOTE: follow player
        if (closest_player && (closest_player_distsq > Square(3.5f)))
        {
          accel = (closest_player->pos - entity->pos);
        }

        move_spec.normalize_accel = true;
        move_spec.drag = 2.f;
        move_spec.speed = 6.f;

        entity->bob_time += delta_time;
        if (entity->bob_time > M_2PI)
        {
          entity->bob_time -= (F32)M_2PI;
        }

        F32 familiar_height = 0.6f;
        F32 entity_scale_mod = 0.5f;
        F32 bob_sin = (0.5f * sinf(entity->bob_time) + 1.f);
        F32 bob_offset = (0.15f * -bob_sin + 0.45f);

        Vec3 familiar_offset = 2.f * vec3(0, bob_offset, 0);

        shadow_alpha = 0.3f * shadow_alpha + 0.25f * bob_sin;
        shadow_scale = (0.9f) * shadow_scale + 0.05f * bob_sin;
        Push_Render_Bitmap(render_group, &game_state->shadow_bmp, vec3(0), shadow_scale, vec4(1, 1, 1, shadow_alpha));

        Loaded_Bitmap* sprite_bmp = Get_Sprite_Sheet_Sprite_Bitmap(&game_state->eye_sprite_sheet, entity->sprite_index);
        Push_Render_Bitmap(render_group, sprite_bmp, familiar_offset, familiar_height);

        Render_Health(render_group, entity, 1.f, entity_scale_mod);
      }
      break;
      case ENTITY_TYPE_SWORD:
      {

        if (entity->distance_limit <= 0.f)
        {
          Make_Entity_Nonspatial(entity);
          Clear_Collision_Rules_For(game_state, entity->storage_index);
          continue;
        }

        F32 sword_height = 0.5f;
        F32 entity_scale_mod = 0.5f;
        shadow_scale *= entity_scale_mod;
        // shadow_alpha *= entity_scale_mod;

        Push_Render_Bitmap(render_group, &game_state->shadow_bmp, vec3(0), shadow_scale, vec4(1, 1, 1, shadow_alpha));
        Push_Render_Bitmap(render_group, &game_state->shuriken_bmp, vec3(0), sword_height);
      }
      break;
      case ENTITY_TYPE_WALL:
      {

        if (entity->chunk_z <= 1)
        {
          Push_Render_Bitmap(render_group, &game_state->pillar_bmp, vec3(0), 2 * TILE_SIZE_IN_METERS);
        }
        else if (entity->chunk_z > 1)
        {
          Push_Render_Bitmap(render_group, &game_state->wall2_bmp, vec3(0), TILE_SIZE_IN_METERS);
        }
      }
      break;
      case ENTITY_TYPE_STAIR:
      {
        if (entity->pos.z < 0)
        {

          Push_Render_Rectangle(render_group, entity->walkable_dim, vec3(0, 0, 0), Color_Pastel_Red);
          // Push_Render_Rectangle(render_group, entity->walkable_dim, vec3(0, -entity->walkable_height, 0), 1.f,
          //                       Color_Pastel_Yellow);
          // Push_Render_Rectangle_Pixel_Outline(render_group, entity->walkable_dim, vec3(0, entity->walkable_height,
          // 0),
          //                                     1.f, Color_Pastel_Cyan);
        }
        else
        {
          Push_Render_Rectangle(render_group, entity->walkable_dim, vec3(0), Color_Pastel_Red);
          // Push_Render_Rectangle_Pixel_Outline(render_group, entity->walkable_dim, vec3(0, entity->walkable_height,
          // 0),
          //                                     1.f, Color_Pastel_Yellow);
        }
      }
      break;
      case ENTITY_TYPE_SPACE:
      {
        for (U32 volume_index = 0; volume_index < entity->collision->volume_count; ++volume_index)
        {
          Sim_Entity_Collision_Volume* volume = entity->collision->volumes + volume_index;
          F32 offset = ((F32)entity->chunk_z - (F32)game_state->camera_pos.chunk_z);
          F32 thickness = MAX(1.f, offset + 2.f);

          Push_Render_Rectangle_Pixel_Outline(render_group, volume->dim.xy,
                                              volume->offset_pos - 0.5f * vec3(0, 0, volume->dim.z), Color_Pastel_Pink,
                                              thickness);
        }
      }
      break;
      case ENTITY_TYPE_NULL:
      default:
      {
        Invalid_Code_Path;
      };
    }
    // NOTE: Draw collision boxes
    {
      // NOTE: this floor is at z == 0 so check if entity collides with it in z
      F32 entity_collision_z = entity->collision->total_volume.dim.z + entity->pos.z;
      B32 is_on_this_floor = ((entity->pos.z >= 0.f) && (entity->pos.z < game_state->typical_floor_height)) ||
                             ((entity_collision_z >= 0.f) && (entity_collision_z < game_state->typical_floor_height));

      if (Has_Flag(entity, ENTITY_FLAG_COLLIDES) && is_on_this_floor)
      {
        Push_Render_Rectangle_Pixel_Outline(render_group, entity->collision->total_volume.dim.xy, vec3(0),
                                            Color_Pastel_Cyan);
      }
    }
    // NOTE: Update Entity
    if (!Has_Flag(entity, ENTITY_FLAG_NONSPATIAL) && Has_Flag(entity, ENTITY_FLAG_MOVEABLE))
    {
      Move_Entity(game_state, sim_region, entity, delta_time, &move_spec, accel);
    }
    basis->pos = Get_Entity_Ground_Point(entity);
  }
  render_group->global_alpha = 1.f;

  Vec2 bottom_left = Get_Camera_Rect_At_Distance(render_group, render_group->render_camera.distance_above_target).min;
  Render_Basis top_left = {vec3(bottom_left.x, -bottom_left.y, 0)};
  render_group->default_basis = &top_left;
  Render_Inputs(render_group, input);
#if 0
  game_state->time += delta_time;
  F32 t = game_state->time;
  Vec2 displacement = vec2(100.f * Cosf(t), 100.f * Sinf(t));

  Vec4 map_color[] = {{{1, 0, 0, 1}}, {{0, 1, 0, 1}}, {{0, 0, 1, 1}}};
  for (S32 map_index = 0; map_index < (S32)Array_Count(transient_state->env_maps); ++map_index)
  {
    Loaded_Bitmap* lod = &transient_state->env_maps[map_index].lod[0];
    B32 row_checker_on = false;
    S32 checker_width = 16;
    S32 checker_height = 16;
    for (S32 y = 0; y < lod->height; y += checker_height)
    {
      B32 checker_on = row_checker_on;
      for (S32 x = 0; x < lod->width; x += checker_width)
      {
        Vec4 color = checker_on ? map_color[map_index] : vec4(0, 0, 0, 1);
        Vec2 min_pos = vec2i(x, y);
        Vec2 max_pos = min_pos + vec2i(checker_width, checker_height);
        Draw_Rectf(lod, min_pos.x, min_pos.y, max_pos.x, max_pos.y, color);
        checker_on = !checker_on;
      }
      row_checker_on = !row_checker_on;
    }
  }
  transient_state->env_maps[0].pos_z = -1.5f;
  transient_state->env_maps[1].pos_z = 0.0f;
  transient_state->env_maps[2].pos_z = 1.5f;

  Vec2 origin = screen_center;

  Vec2 x_axis = 100.f * vec2(Cosf(2.f * t), Sinf(2.f * t));
  // Vec2 x_axis = vec2(100, 0);
  Vec2 y_axis = Vec_Perp(x_axis);
  // Vec4 color = vec4(0.5f + 0.5f * Cosf(5.3f * t), 0.5f + 0.5f * Sinf(7.3f * t), 0.5f + 0.5f * Cosf(-6.3f * t),
  //                   0.5f + 0.5f * Sinf(9.9f * t));
  Vec4 color = vec4(1);
  Render_Entry_Coordinate_System* entry = Render_Coordinate_System(
      render_group, origin + displacement - 0.5f * x_axis - 0.5f * y_axis, x_axis, y_axis, color,
      &game_state->test_diffuse, &game_state->test_normal, transient_state->env_maps + 2, transient_state->env_maps + 1,
      transient_state->env_maps + 0);
  U32 point_index = 0;
  for (F32 y = 0.f; y < 1.f; y += 0.25f)
  {
    for (F32 x = 0.f; x < 1.f; x += 0.25f)
    {
      entry->points[point_index++] = vec2(x, y);
    }
  }

  Vec2 map_pos = vec2(0);
  for (S32 map_index = 0; map_index < (S32)Array_Count(transient_state->env_maps); ++map_index)
  {
    Loaded_Bitmap* lod = &transient_state->env_maps[map_index].lod[0];
    x_axis = 0.5f * vec2i(lod->width, 0);
    y_axis = 0.5f * vec2i(0, lod->height);
    Render_Coordinate_System(render_group, map_pos, x_axis, y_axis, vec4(1), lod, 0, 0, 0, 0);
    map_pos += y_axis + vec2(0, 5.f);
  }
  // Push_Render_Saturation(render_group, 0.5f + 0.5f * Sinf(t));
#endif
  Render_Group_To_Output(render_group, draw_buffer);

  // Draw_BMP(draw_buffer, &game_state->test_bmp, 10, 10, 2);
  // Draw_BMP(draw_buffer, &game_state->test_bmp, 20, 20, 2);
  End_Sim(sim_region, game_state);
  End_Temp_Memory(&transient_state->transient_arena, sim_memory);
  End_Temp_Memory(&transient_state->transient_arena, render_memory);

  Check_Arena(&game_state->world_arena);
  Check_Arena(&transient_state->transient_arena);
  END_TIMED_BLOCK(Game_Update_And_Render);

  return;
}

extern "C" GAME_GET_SOUND_SAMPLES(Game_Get_Sound_Samples)
{
  Game_State* game_state = (Game_State*)memory->permanent_storage;
  // TODO: Allow sample offests for more robust platform options
  Game_Output_Sound(game_state, sound_buffer);
}
