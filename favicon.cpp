#include "favicon.h"

void load_favicon(SDL_Window* window){
  SDL_Surface* surface = SDL_LoadBMP("./ui/favicon.bmp");

  // The icon is attached to the window pointer
  SDL_SetWindowIcon(window, surface);

  // ...and the surface containing the icon pixel data is no longer required.
  SDL_FreeSurface(surface);
}
