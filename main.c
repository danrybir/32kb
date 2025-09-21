// the code is optimized for size, not speed or stability
// expect errors, lag etc

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define min(x, y) ((x) > (y) ? (y) : (x))
#define max(x, y) ((x) > (y) ? (x) : (y))

// world gen shtuff
static inline uint64_t splitmix64(uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  x = x ^ (x >> 31);
  return x;
}
static inline float smoothstep(float a, float b, float t) {
  return a + (b - a) * (3 * t * t - 2 * t * t * t);
}
float smoothstep2(float v00, float v10, float v01, float v11, float x, float y) {
  // still bilinear interpolation
  // now with smoothstep!
  float i0 = smoothstep(v00, v10, x);
  float i1 = smoothstep(v01, v11, x);
  return smoothstep(i0, i1, y);
}
static float u64_to_unit_float(uint64_t x) {
  uint32_t y = (uint32_t)(x >> 40);
  return (float)y / 16777216.0f;
}
float noise(float x, float y, uint64_t seed) {
  int32_t xi = (int32_t)floorf(x);
  int32_t yi = (int32_t)floorf(y);
  uint32_t uxi = (uint32_t)xi;
  uint32_t uyi = (uint32_t)yi;
  uint64_t k00 = ((uint64_t)uxi << 32) | (uint64_t)uyi;
  uint64_t k10 = ((uint64_t)((uint32_t)(xi + 1)) << 32) | (uint64_t)uyi;
  uint64_t k01 = ((uint64_t)uxi << 32) | (uint64_t)((uint32_t)(yi + 1));
  uint64_t k11 = ((uint64_t)((uint32_t)(xi + 1)) << 32) | (uint64_t)((uint32_t)(yi + 1));
  float v00 = u64_to_unit_float(splitmix64(k00 + splitmix64(seed)));
  float v10 = u64_to_unit_float(splitmix64(k10 + splitmix64(seed)));
  float v01 = u64_to_unit_float(splitmix64(k01 + splitmix64(seed)));
  float v11 = u64_to_unit_float(splitmix64(k11 + splitmix64(seed)));
  float fx = x - (float)xi;
  float fy = y - (float)yi;
  // return lerp2(v00, v10, v01, v11, fx, fy);
  return smoothstep2(v00, v10, v01, v11, fx, fy);
}

// hashmap implementation because every good game needs one
typedef enum {
  EMPTY,
  USED,
  DELETED
} EntryState;
typedef struct {
  uint64_t key;
  uint64_t value;
  EntryState state;
} Entry;
typedef struct {
  Entry* entries;
  size_t capacity;
  size_t size;
} HashMap;
void hashmap_put(HashMap* map, uint64_t key, uint64_t value);
HashMap* hashmap_create(size_t initial_capacity) {
  HashMap* map = malloc(sizeof(HashMap));
  map->capacity = initial_capacity;
  map->size = 0;
  map->entries = calloc(initial_capacity, sizeof(Entry));
  return map;
}
void hashmap_free(HashMap* map) {
  free(map->entries);
  free(map);
}
static void hashmap_resize(HashMap* map, size_t new_capacity) {
  Entry* old_entries = map->entries;
  size_t old_capacity = map->capacity;
  map->entries = calloc(new_capacity, sizeof(Entry));
  map->capacity = new_capacity;
  map->size = 0;
  for(size_t i = 0; i < old_capacity; i++) {
    if(old_entries[i].state == USED) hashmap_put(map, old_entries[i].key, old_entries[i].value);
  }
  free(old_entries);
}
uint64_t* hashmap_get(HashMap* map, uint64_t key) {
  if(map->capacity == 0) return NULL;
  uint64_t hash = splitmix64(key);
  size_t index = hash % map->capacity;
  for(size_t i = 0; i < map->capacity; i++) {
    size_t idx = (index + i) % map->capacity;
    if(map->entries[idx].state == EMPTY) { return NULL; }
    if(map->entries[idx].state == USED && map->entries[idx].key == key) {
      return &map->entries[idx].value;
    }
  }
  return NULL;
}
void hashmap_put(HashMap* map, uint64_t key, uint64_t value) {
  if(map->capacity == 0) return;
  if(map->size * 10 >= map->capacity * 7) hashmap_resize(map, map->capacity * 2);
  uint64_t hash = splitmix64(key);
  size_t index = hash % map->capacity;
  size_t first_deleted = map->capacity;
  for(size_t i = 0; i < map->capacity; i++) {
    size_t idx = (index + i) % map->capacity;
    if(map->entries[idx].state == EMPTY) { break; }
    if(map->entries[idx].state == DELETED && first_deleted == map->capacity) {
      first_deleted = idx;
    }
    if(map->entries[idx].state == USED && map->entries[idx].key == key) {
      map->entries[idx].value = value;
      return;
    }
  }
  size_t target_idx = first_deleted;
  if(first_deleted == map->capacity) {
    target_idx = index;
    for(size_t i = 0; i < map->capacity; i++) {
      size_t idx = (index + i) % map->capacity;
      if(map->entries[idx].state != USED) {
        target_idx = idx;
        break;
      }
    }
  }
  map->entries[target_idx].key = key;
  map->entries[target_idx].value = value;
  map->entries[target_idx].state = USED;
  map->size++;
}
void hashmap_remove(HashMap* map, uint64_t key) {
  if(map->capacity == 0) return;
  uint64_t hash = splitmix64(key);
  size_t index = hash % map->capacity;
  for(size_t i = 0; i < map->capacity; i++) {
    size_t idx = (index + i) % map->capacity;
    if(map->entries[idx].state == EMPTY) { return; }
    if(map->entries[idx].state == USED && map->entries[idx].key == key) {
      map->entries[idx].state = DELETED;
      map->size--;
      break;
    }
  }
  if(map->capacity > 1 && map->size * 10 <= map->capacity) {
    hashmap_resize(map, map->capacity / 2);
  }
}

// tiles
#define TPRT 0xE7
const unsigned char tiles[][64] = {
#define TILE_PLAYER 0
  {TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, 0xFF, 0xFF, TPRT, TPRT, TPRT,
   TPRT, TPRT, 0xFF, 0xFF, 0xFF, 0xFF, TPRT, TPRT, TPRT, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, TPRT,
   TPRT, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, TPRT, TPRT, TPRT, 0xFF, 0xFF, 0xFF, 0xFF, TPRT, TPRT,
   TPRT, TPRT, TPRT, 0xFF, 0xFF, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT},
#define TILE_BLACK 1
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
#define TILE_CURSOR 2
  {0xE0, 0xE0, TPRT, 0xE0, 0xE0, TPRT, 0xE0, 0xE0, 0xE0, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, 0xE0,
   TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, 0xE0, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, 0xE0,
   0xE0, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, 0xE0, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT,
   0xE0, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, 0xE0, 0xE0, 0xE0, TPRT, 0xE0, 0xE0, TPRT, 0xE0, 0xE0},
#define TILE_GRASS 3
  {0x10, 0x10, 0x14, 0x14, 0x10, 0x14, 0x10, 0x10, 0x14, 0x10, 0x14, 0x10, 0x10, 0x10, 0x14, 0x14,
   0x10, 0x14, 0x10, 0x14, 0x14, 0x14, 0x14, 0x14, 0x10, 0x10, 0x14, 0x10, 0x10, 0x14, 0x10, 0x10,
   0x10, 0x14, 0x10, 0x14, 0x14, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x10, 0x10, 0x14, 0x14, 0x10,
   0x10, 0x14, 0x14, 0x14, 0x10, 0x14, 0x10, 0x14, 0x14, 0x10, 0x10, 0x10, 0x10, 0x14, 0x14, 0x14},
#define TILE_WATER 4
  {0x0F, 0x17, 0x0F, 0x0F, 0x0F, 0x0F, 0x0B, 0x0F, 0x17, 0x17, 0x0B, 0x0F, 0x0F, 0x0F, 0x0B, 0x0B,
   0x0F, 0x0B, 0x0F, 0x0F, 0x0F, 0x17, 0x0F, 0x0F, 0x0F, 0x0F, 0x17, 0x17, 0x0B, 0x17, 0x17, 0x0F,
   0x0F, 0x0F, 0x17, 0x0F, 0x0F, 0x0B, 0x0F, 0x0F, 0x0F, 0x0B, 0x0B, 0x0F, 0x17, 0x17, 0x0F, 0x0F,
   0x17, 0x17, 0x0F, 0x0F, 0x17, 0x0F, 0x0B, 0x0F, 0x0F, 0x17, 0x0F, 0x0F, 0x0F, 0x0B, 0x0B, 0x0F},
#define TILE_SAND 5
  {0xF4, 0xF0, 0xF4, 0xF4, 0xF4, 0xF4, 0xFC, 0xF4, 0xF0, 0xF0, 0xFC, 0xF4, 0xF4, 0xF4, 0xFC, 0xFC,
   0xF4, 0xFC, 0xF4, 0xF4, 0xF4, 0xF0, 0xF4, 0xF4, 0xF4, 0xF4, 0xF0, 0xF0, 0xFC, 0xF0, 0xF0, 0xF4,
   0xF4, 0xF4, 0xF0, 0xF4, 0xF4, 0xFC, 0xF4, 0xF4, 0xF4, 0xFC, 0xFC, 0xF4, 0xF0, 0xF0, 0xF4, 0xF4,
   0xF0, 0xF0, 0xF4, 0xF4, 0xF0, 0xF4, 0xFC, 0xF4, 0xF4, 0xF0, 0xF4, 0xF4, 0xF4, 0xFC, 0xFC, 0xF4},
#define TILE_DIRT 6
  {0x64, 0x44, 0x68, 0x88, 0x44, 0x88, 0x44, 0x44, 0x88, 0x44, 0x88, 0x44, 0x44, 0x44, 0x88, 0x68,
   0x44, 0x88, 0x44, 0x68, 0x88, 0x68, 0x88, 0x68, 0x44, 0x64, 0x88, 0x44, 0x44, 0x88, 0x44, 0x44,
   0x44, 0x88, 0x64, 0x88, 0x68, 0x68, 0x44, 0x88, 0x64, 0x68, 0x44, 0x44, 0x64, 0x88, 0x88, 0x64,
   0x44, 0x88, 0x68, 0x88, 0x44, 0x88, 0x44, 0x88, 0x88, 0x44, 0x44, 0x44, 0x44, 0x68, 0x88, 0x68},
#define TILE_STONE 7
  {0x6E, 0x92, 0x92, 0x6E, 0x92, 0x6E, 0x6E, 0x6E, 0x92, 0x6E, 0x92, 0x6E, 0x92, 0x92, 0x92, 0x92,
   0x92, 0x6E, 0x92, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x92, 0x6E, 0x92, 0x92, 0x92, 0x92, 0x6E, 0x92,
   0x92, 0x92, 0x92, 0x92, 0x6E, 0x92, 0x6E, 0x92, 0x92, 0x92, 0x6E, 0x92, 0x6E, 0x92, 0x92, 0x92,
   0x6E, 0x6E, 0x6E, 0x92, 0x92, 0x6E, 0x92, 0x6E, 0x92, 0x92, 0x6E, 0x92, 0x92, 0x92, 0x92, 0x6E},
#define TILE_WOOD 8
  {0x84, 0x64, 0x84, 0x84, 0x64, 0x84, 0x64, 0x84, 0x84, 0x64, 0x84, 0x84, 0x64, 0x84, 0x64, 0x84,
   0x84, 0x84, 0x64, 0x84, 0x64, 0x84, 0x84, 0x64, 0x64, 0x84, 0x64, 0x84, 0x64, 0x84, 0x64, 0x84,
   0x64, 0x84, 0x64, 0x84, 0x64, 0x84, 0x64, 0x84, 0x64, 0x84, 0x64, 0x84, 0x64, 0x84, 0x64, 0x84,
   0x84, 0x84, 0x64, 0x84, 0x84, 0x84, 0x64, 0x84, 0x64, 0x84, 0x64, 0x84, 0x84, 0x64, 0x84, 0x64}};
const unsigned char tile_masks[] = {0, 0, 0, 15, 15, 15, 15, 15, 15};
void draw_tile(int x, int y, int i, unsigned char mask, SDL_Renderer* renderer) {
  // 0000abcd
  // a - rotate 90deg ccw
  // b - rotate 90deg cw
  // c - flip vertical
  // d - flip horizontal
  unsigned char* tile = (unsigned char*)tiles[i];
  int rot = 0;
  if(mask & 0x04)
    rot = 1;
  else if(mask & 0x08)
    rot = 3;
  int flip_h = (mask & 0x01) != 0;
  int flip_v = (mask & 0x02) != 0;
  for(int oy = 0; oy < 8; ++oy) {
    for(int ox = 0; ox < 8; ++ox) {
      int sx, sy;
      switch(rot) {
      case 0: // 0deg
        sx = ox;
        sy = oy;
        break;
      case 1: // 90deg CW
        sx = oy;
        sy = 7 - ox;
        break;
      // 180deg is flipH+V
      case 3: // 90deg CCW
        sx = 7 - oy;
        sy = ox;
        break;
      default:
        sx = ox;
        sy = oy;
      }
      if(flip_h) sx = 7 - sx;
      if(flip_v) sy = 7 - sy;
      unsigned char color332 = tile[sy * 8 + sx];
      if(color332 == TPRT) continue;
      unsigned char r = (color332 >> 5) << 5;
      unsigned char g = (color332 & 0b00011100) << 3;
      unsigned char b = (color332 & 0b11) << 6;
      SDL_SetRenderDrawColor(renderer, r, g, b, 255);
      SDL_Rect rect = {x + ox * 4, y + oy * 4, 4, 4};
      SDL_RenderFillRect(renderer, &rect);
    }
  }
}

// actual world gen
int get_tile(int32_t x, int32_t y, HashMap* map, uint64_t seed, char check_changed) {
  // placed blocks
  if(check_changed) {
    uint64_t* placed = hashmap_get(map, ((uint64_t)x << 32) | (uint32_t)y);
    if(placed != NULL) return *placed;
  }
  // terrain
  float elevation = noise((float)x / 8, (float)y / 8, splitmix64(seed));
  float stonemap = powf(noise((float)x / 64, (float)y / 64, splitmix64(seed) + 1), 3);
  float woodmap = (splitmix64(seed + 1 + splitmix64(x) * splitmix64(y)) % 9) == 0
                    ? noise((float)x / 32, (float)y / 32, splitmix64(seed) + 3)
                    : 0;
  return elevation > 0.5
           ? (stonemap > 0.3 && elevation > 0.6
                ? TILE_STONE
                : (woodmap > 0.6 && stonemap < 0.05 && elevation > 0.6 ? TILE_WOOD : TILE_GRASS))
         : elevation > 0.4 ? TILE_SAND
                           : TILE_WATER;
}

int main(int argc, char** argv) {
  uint64_t seed = 0;
  if(argc > 1) seed = strtoull(argv[1], NULL, 10);
  HashMap* map = hashmap_create(256);
  if(!map) return 1;
  if(SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
  SDL_Window* win = SDL_CreateWindow(
    "32kb", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 512, 512, SDL_WINDOW_SHOWN);
  if(!win) {
    SDL_Quit();
    return 1;
  }
  SDL_Renderer* ren =
    SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if(!ren) {
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 1;
  }
  char running = 1;
  int px = 0, py = 0;
  int cx = -7, cy = -7;
  int ox = 1, oy = 0;
  int try = 0;
  while(get_tile(px, py, map, seed, 0) == TILE_BLACK) {
    ++try;
    px += splitmix64((uint64_t)(int32_t)(px + splitmix64(seed) + try)) % 256 - 128;
    py += splitmix64((uint64_t)(int32_t)(splitmix64(px) + py + splitmix64(seed) + try)) % 256 - 128;
  }
  cx = px - 7;
  cy = py - 7;
  printf("spawned at %d, %d\n", px, py);
  while(running) {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
      if(e.type == SDL_QUIT)
        running = 0;
      else if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        running = 0;
      else if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
        int32_t cursx = px + ox, cursy = py + oy;
        uint64_t i = ((uint64_t)cursx << 32) | (uint32_t)cursy;
        if(hashmap_get(map, i) == 0) {
          hashmap_put(map, i, TILE_BLACK);
        } else {
          hashmap_remove(map, i);
        }
      }
    }
    const Uint8* state = SDL_GetKeyboardState(NULL);
    if(state[SDL_SCANCODE_U]) {
      ox = 0;
      oy = -1;
    }
    if(state[SDL_SCANCODE_J]) {
      ox = 0;
      oy = 1;
    }
    if(state[SDL_SCANCODE_H]) {
      ox = -1;
      oy = 0;
    }
    if(state[SDL_SCANCODE_K]) {
      ox = 1;
      oy = 0;
    }
    if(state[SDL_SCANCODE_UP]) {
      py -= 1;
      cy = min(py - 4, cy);
    }
    if(state[SDL_SCANCODE_DOWN]) {
      py += 1;
      cy = max(py - 11, cy);
    }
    if(state[SDL_SCANCODE_LEFT]) {
      px -= 1;
      cx = min(px - 4, cx);
    }
    if(state[SDL_SCANCODE_RIGHT]) {
      px += 1;
      cx = max(px - 11, cx);
    }
    SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
    SDL_RenderClear(ren);
    for(int i = 0; i < 16 * 16; ++i) {
      int tile = get_tile((i % 16) + cx, (i / 16) + cy, map, seed, 1);
      draw_tile(
        (i % 16) * 32, (i / 16) * 32, tile, (splitmix64(i + cy * 16 + cx)) & tile_masks[tile], ren);
    }
    draw_tile((px - cx) * 32, (py - cy) * 32, TILE_PLAYER, 0, ren);
    draw_tile((px - cx + ox) * 32, (py - cy + oy) * 32, TILE_CURSOR, 0, ren);
    SDL_RenderPresent(ren);
    SDL_Delay(100);
  }
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}