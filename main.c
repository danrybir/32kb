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
static inline float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}
float lerp2(float v00, float v10, float v01, float v11, float x, float y) {
  // aka bilinear interpolation
  float i0 = lerp(v00, v10, x);
  float i1 = lerp(v01, v11, x);
  return lerp(i0, i1, y);
}
static float u64_to_unit_float(uint64_t x) {
  uint32_t y = (uint32_t)(x >> 40);
  return (float)y / 16777216.0f;
}
float noise_(float x, float y) {
  float v00 = u64_to_unit_float(splitmix64(((uint64_t)x >> 32) | (uint32_t)y));
  float v10 = u64_to_unit_float(splitmix64(((uint64_t)(x + 1) >> 32) | (uint32_t)y));
  float v01 = u64_to_unit_float(splitmix64(((uint64_t)x >> 32) | (uint32_t)(y + 1)));
  float v11 = u64_to_unit_float(splitmix64(((uint64_t)(x + 1) >> 32) | (uint32_t)(y + 1)));
  return lerp2(v00, v10, v01, v11, x - (long)x, y - (long)y);
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
  return lerp2(v00, v10, v01, v11, fx, fy);
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
static void hashmap_resize(HashMap* map);
HashMap* hashmap_create(size_t initial_capacity) {
  HashMap* map = malloc(sizeof(HashMap));
  map->capacity = initial_capacity;
  map->size = 0;
  map->entries = calloc(map->capacity, sizeof(Entry));
  return map;
}
void hashmap_put(HashMap* map, uint64_t key, uint64_t value) {
  if((double)map->size / map->capacity > 0.7) hashmap_resize(map);
  uint64_t hash = splitmix64(key);
  size_t idx = hash % map->capacity;
  while(map->entries[idx].state == USED) {
    if(map->entries[idx].key == key) {
      map->entries[idx].value = value;
      return;
    }
    idx = (idx + 1) % map->capacity;
  }
  map->entries[idx].key = key;
  map->entries[idx].value = value;
  map->entries[idx].state = USED;
  map->size++;
}
uint64_t* hashmap_get(HashMap* map, uint64_t key) {
  uint64_t hash = splitmix64(key);
  size_t idx = hash % map->capacity;
  while(map->entries[idx].state != EMPTY) {
    if(map->entries[idx].state == USED && map->entries[idx].key == key)
      return &map->entries[idx].value;
    idx = (idx + 1) % map->capacity;
  }
  return NULL;
}
void hashmap_remove(HashMap* map, uint64_t key) {
  uint64_t hash = splitmix64(key);
  size_t idx = hash % map->capacity;
  while(map->entries[idx].state != EMPTY) {
    if(map->entries[idx].state == USED && map->entries[idx].key == key) {
      map->entries[idx].state = DELETED;
      map->size--;
      return;
    }
    idx = (idx + 1) % map->capacity;
  }
}
void hashmap_free(HashMap* map) {
  free(map->entries);
  free(map);
}
static void hashmap_resize(HashMap* map) {
  size_t new_capacity = map->capacity * 2;
  Entry* new_entries = calloc(new_capacity, sizeof(Entry));
  for(size_t i = 0; i < map->capacity; i++) {
    if(map->entries[i].state == USED) {
      uint64_t hash = splitmix64(map->entries[i].key);
      size_t idx = hash % new_capacity;
      while(new_entries[idx].state == USED) idx = (idx + 1) % new_capacity;
      new_entries[idx] = map->entries[i];
    }
  }
  free(map->entries);
  map->entries = new_entries;
  map->capacity = new_capacity;
}

/*
  vscode color picker thing
  #e2a625ff
*/
#define TPRT 0xE7
const unsigned char tiles[][64] = {
  {TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, 0xFF, 0xFF, TPRT, TPRT, TPRT,
   TPRT, TPRT, 0xFF, 0xFF, 0xFF, 0xFF, TPRT, TPRT, TPRT, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, TPRT,
   TPRT, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, TPRT, TPRT, TPRT, 0xFF, 0xFF, 0xFF, 0xFF, TPRT, TPRT,
   TPRT, TPRT, TPRT, 0xFF, 0xFF, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT},
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  {0x18, 0x10, 0x18, 0x18, 0x10, 0x10, 0x18, 0x10, 0x10, 0x18, 0x10, 0x10, 0x18, 0x18, 0x10, 0x18,
   0x10, 0x10, 0x18, 0x18, 0x10, 0x10, 0x18, 0x18, 0x10, 0x18, 0x10, 0x10, 0x18, 0x18, 0x10, 0x18,
   0x18, 0x18, 0x10, 0x18, 0x10, 0x10, 0x18, 0x10, 0x10, 0x18, 0x10, 0x18, 0x10, 0x18, 0x10, 0x10,
   0x18, 0x10, 0x10, 0x18, 0x10, 0x18, 0x10, 0x18, 0x10, 0x18, 0x10, 0x10, 0x18, 0x10, 0x18, 0x10},
  {0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
   0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
   0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
   0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F},
  {0xF8, 0xF4, 0xF8, 0xF8, 0xF4, 0xF4, 0xF8, 0xF4, 0xF4, 0xF8, 0xF4, 0xF4, 0xF8, 0xF8, 0xF4, 0xF8,
   0xF4, 0xF4, 0xF8, 0xF8, 0xF4, 0xF4, 0xF8, 0xF8, 0xF4, 0xF8, 0xF4, 0xF4, 0xF8, 0xF8, 0xF4, 0xF8,
   0xF8, 0xF8, 0xF4, 0xF8, 0xF4, 0xF4, 0xF8, 0xF4, 0xF4, 0xF8, 0xF4, 0xF8, 0xF4, 0xF8, 0xF4, 0xF4,
   0xF8, 0xF4, 0xF4, 0xF8, 0xF4, 0xF8, 0xF4, 0xF8, 0xF4, 0xF8, 0xF4, 0xF4, 0xF8, 0xF4, 0xF8, 0xF4},
  {0xE0, 0xE0, TPRT, 0xE0, 0xE0, TPRT, 0xE0, 0xE0, 0xE0, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, 0xE0,
   TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, 0xE0, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, 0xE0,
   0xE0, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, 0xE0, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT,
   0xE0, TPRT, TPRT, TPRT, TPRT, TPRT, TPRT, 0xE0, 0xE0, 0xE0, TPRT, 0xE0, 0xE0, TPRT, 0xE0, 0xE0}};

void draw_tile(int x, int y, int i, unsigned char mask, SDL_Renderer* renderer) {
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
      case 2: // 180deg
        sx = 7 - ox;
        sy = 7 - oy;
        break;
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

int get_tile(int32_t x, int32_t y, HashMap* map, uint64_t seed) {
  if(hashmap_get(map, ((uint64_t)x << 32) | (uint32_t)y) != NULL) return 1;
  float value = noise((float)x / 8, (float)y / 8, seed);
  return value > 0.5 ? 2 : value > 0.4 ? 4 : 3;
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
  while(get_tile(px, py, map, seed) == 3) {
    px += splitmix64(px + splitmix64(seed)) % 256 - 128;
    py += splitmix64(splitmix64(px) + py + splitmix64(seed)) % 256 - 128;
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
          hashmap_put(map, i, 1);
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
    // idk what im doing wrong but im doing it wrong
    if(state[SDL_SCANCODE_UP] && get_tile(px, py - 1, map, seed) != 3) {
      py -= 1;
      cy = min(py - 4, cy);
    }
    if(state[SDL_SCANCODE_DOWN] && get_tile(px, py + 1, map, seed) != 3) {
      py += 1;
      cy = max(py - 11, cy);
    }
    if(state[SDL_SCANCODE_LEFT] && get_tile(px - 1, py, map, seed) != 3) {
      px -= 1;
      cx = min(px - 4, cx);
    }
    if(state[SDL_SCANCODE_RIGHT] && get_tile(px + 1, py, map, seed) != 3) {
      px += 1;
      cx = max(px - 11, cx);
    }
    SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
    SDL_RenderClear(ren);
    for(int i = 0; i < 16 * 16; ++i)
      draw_tile((i % 16) * 32,
                (i / 16) * 32,
                get_tile((i % 16) + cx, (i / 16) + cy, map, seed),
                (splitmix64(i + cy * 16 + cx)) & 0xf,
                ren);
    draw_tile((px - cx) * 32, (py - cy) * 32, 0, 0, ren);
    draw_tile((px - cx + ox) * 32, (py - cy + oy) * 32, 5, 0, ren);
    SDL_RenderPresent(ren);
    SDL_Delay(100);
  }
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}