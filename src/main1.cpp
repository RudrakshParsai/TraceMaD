#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // SDL3 needs this for the main function to work right
#include <math.h>
#include <iostream>

#define WIDTH 900
#define HEIGHT 600
#define WHITE 0xffffffff // Hex for white: Red, Green, Blue, and Alpha (opacity)

// My custom object to keep track of a circle's data
struct Circle {
  double x;
  double y;
  double r;
};

// Logic: Draw a square area around the center, then only color pixels inside the radius
void FillCircle(SDL_Surface* surface, struct Circle circle, Uint32 color) {
  double radius_squared = pow(circle.r, 2); // Squared radius for the distance formula

  // Loop through a grid (bounding box) around the circle
  for (double x = (circle.x - circle.r); x <= (circle.x + circle.r); x++) {
    for (double y = (circle.y - circle.r); y <= (circle.y + circle.r); y++) {
      
      // Distance formula: a^2 + b^2 = c^2
      double distance_squared = pow(x - circle.x, 2) + pow(y - circle.y, 2);
      
      if (distance_squared < radius_squared) {
        // Safety: Don't draw outside the window or the program might crash!
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
          // In SDL, even a single pixel is technically a tiny 1x1 rectangle
          SDL_Rect pixel = {(int)x, (int)y, 1, 1};
          SDL_FillSurfaceRect(surface, &pixel, color);
        }
      }
    }
  }
}

int main(int argc, char* argv[]) {
  // 1. Fire up SDL's video system
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  // 2. Create the actual window (Title, Width, Height, Flags)
  SDL_Window* window = SDL_CreateWindow("RayTracing", WIDTH, HEIGHT, 0);

  if (!window) {
    SDL_Log("Window creation failed: %s", SDL_GetError());
    return 1;
  }

  // 3. Get the "canvas" (surface) of the window so we can draw on it
  SDL_Surface* surface = SDL_GetWindowSurface(window);

//   // 4. Draw a test square: {x, y, width, height}
//   SDL_Rect rect = {200, 200, 200, 200};
//   SDL_FillSurfaceRect(surface, &rect, WHITE);

  // 5. Draw my circle
  struct Circle circle = {400, 300, 80};
  FillCircle(surface, circle, WHITE);

  SDL_UpdateWindowSurface(window);

  SDL_Delay(5000);

  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}