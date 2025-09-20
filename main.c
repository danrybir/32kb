#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <stdio.h>

/*
  vscode color picker thing
  #4b2e0dff
*/
const unsigned long colors[] = {0x000000ff, 0x1adb14ff, 0x4b2e0dff, 0xffffffff, 0x00000000};
const unsigned char tiles[][64] = {
  {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 3, 3, 3, 3, 4, 4, 4, 3, 3, 3, 3, 3, 3, 4,
   4, 3, 3, 3, 3, 3, 3, 4, 4, 4, 3, 3, 3, 3, 4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

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
  while(running) {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
      if(e.type == SDL_QUIT)
        running = 0;
      else if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        running = 0;
    }
    SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
    SDL_RenderClear(ren);
    for(int i = 0; i < 16 * 16; ++i)
      draw_tile((i % 16) * 32, (i / 16) * 32, (i + i / 16) % 2, 0, ren);
    SDL_RenderPresent(ren);
    SDL_Delay(16);
  }
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}