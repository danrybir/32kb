#include <SDL2/SDL.h>
#include <stdio.h>
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
  SDL_RenderPresent(ren);
  SDL_Delay(3000);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}