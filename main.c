#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define min(x, y) x > y ? y : x
#define max(x, y) x > y ? x : y

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
float noise(float x, float y) {
  uint32_t xi = (uint32_t)floorf(x);
  uint32_t yi = (uint32_t)floorf(y);
  uint64_t k00 = ((uint64_t)xi << 32) | (uint64_t)yi;
  uint64_t k10 = ((uint64_t)(xi + 1) << 32) | (uint64_t)yi;
  uint64_t k01 = ((uint64_t)xi << 32) | (uint64_t)(yi + 1);
  uint64_t k11 = ((uint64_t)(xi + 1) << 32) | (uint64_t)(yi + 1);
  float v00 = u64_to_unit_float(splitmix64(k00));
  float v10 = u64_to_unit_float(splitmix64(k10));
  float v01 = u64_to_unit_float(splitmix64(k01));
  float v11 = u64_to_unit_float(splitmix64(k11));
  float fx = x - (float)xi;
  float fy = y - (float)yi;
  return lerp2(v00, v10, v01, v11, fx, fy);
}


/*
  vscode color picker thing
  #31e4c6ff
*/
const unsigned long colors[] = {
  0x000000ff, 0x1adb14ff, 0x31e4c6ff, 0xffffffff, 0x00000000, 0x168812ff};
const unsigned char tiles[][64] = {
  {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 3, 3, 3, 3, 4, 4, 4, 3, 3, 3, 3, 3, 3, 4,
   4, 3, 3, 3, 3, 3, 3, 4, 4, 4, 3, 3, 3, 3, 4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {1, 5, 1, 1, 5, 5, 1, 5, 5, 1, 5, 5, 1, 1, 5, 1, 5, 5, 1, 1, 5, 5, 1, 1, 5, 1, 5, 5, 1, 1, 5, 1,
   1, 1, 5, 1, 5, 5, 1, 5, 5, 1, 5, 1, 5, 1, 5, 5, 1, 5, 5, 1, 5, 1, 5, 1, 5, 1, 5, 5, 1, 5, 1, 5},
  {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}};

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
      unsigned long color = colors[tile[sy * 8 + sx]];
      unsigned char r = (color >> 24) & 0xFF;
      unsigned char g = (color >> 16) & 0xFF;
      unsigned char b = (color >> 8) & 0xFF;
      unsigned char a = (color)&0xFF;
      if(a == 0) continue;
      SDL_SetRenderDrawColor(renderer, r, g, b, a);
      SDL_Rect rect = {x + ox * 4, y + oy * 4, 4, 4};
      SDL_RenderFillRect(renderer, &rect);
    }
  }
}


int main() {
  if(SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
  // world gen
  unsigned char* world;
  int ww = 512, wh = 512;
  world = malloc(ww * wh);
  if(!world) return 1;
  for(int y = 0; y < wh; ++y) {
    printf("generating row %d/%d...", y + 1, wh);
    for(int x = 0; x < ww; ++x) {
      int i = y * ww + x;
      float value = noise((float)x / 8, (float)y / 8);
      world[i] = value > 0.5 ? 2 : 3;
    }
    printf(" done\n");
  }
  SDL_Window* win = SDL_CreateWindow(
    "nightmare", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 512, 512, SDL_WINDOW_SHOWN);
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
  int cx = 0, cy = 0;
  while(running) {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
      if(e.type == SDL_QUIT)
        running = 0;
      else if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        running = 0;
    }
    const Uint8* state = SDL_GetKeyboardState(NULL);
    if(state[SDL_SCANCODE_UP]) {
      if(py > 0) py -= 1;
      cy = max(min(py - 4, cy), 0);
    }
    if(state[SDL_SCANCODE_DOWN]) {
      if(py < wh - 1) py += 1;
      cy = max(py - 11, cy);
      cy = min(cy, wh - 16);
    }
    if(state[SDL_SCANCODE_LEFT]) {
      if(px > 0) px -= 1;
      cx = max(min(px - 4, cx), 0);
    }
    if(state[SDL_SCANCODE_RIGHT]) {
      if(px < ww - 1) px += 1;
      cx = max(px - 11, cx);
      cx = min(cx, ww - 16);
    }
    SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
    SDL_RenderClear(ren);
    for(int i = 0; i < 16 * 16; ++i)
      draw_tile((i % 16) * 32, (i / 16) * 32, world[((i / 16) + cy) * ww + (i % 16) + cx], 0, ren);
    draw_tile((px - cx) * 32, (py - cy) * 32, 0, 0, ren);
    SDL_RenderPresent(ren);
    SDL_Delay(100);
  }
  free(world);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}