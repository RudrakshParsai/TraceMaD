#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cmath>
#include <vector>
#include <algorithm>

// Global settings
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 600;
const int NUM_RAYS = 720;
const double PI = 3.14159265358979323846;

struct Vec2 {
    double x, y;
};

struct Circle {
    Vec2 pos;
    double radius;
    Vec2 velocity;
};

struct Ray {
    Vec2 start;
    Vec2 dir; 
};

auto clamp = [](int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
};

// SDL3 handles pixel formats differently, so we need to map our RGB to the surface's format
Uint32 map_color(SDL_PixelFormat format, Uint8 r, Uint8 g, Uint8 b) {
    const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(format);
    if (!details) return 0;
    return SDL_MapRGB(details, nullptr, r, g, b);
}

// Draw pixel directly to the buffer. 
//Surface MUST be locked before calling this.
void put_pixel_fast(Uint32* pixels, int pitch, int x, int y, Uint32 color) {
    pixels[y * pitch + x] = color;
}

void draw_filled_circle(SDL_Surface* surface, Circle circle, Uint32 color) {
    int cx = (int)circle.pos.x;
    int cy = (int)circle.pos.y;
    int r = (int)circle.radius;
    
    // divide by 4 because pixels are 32-bit
    int pitch = surface->pitch / 4;
    Uint32* pixels = (Uint32*)surface->pixels;

// Only check pixels inside the circle's bounding box
    int x_start = std::max(0, cx - r);
    int x_end = std::min(SCREEN_WIDTH - 1, cx + r);
    int y_start = std::max(0, cy - r);
    int y_end = std::min(SCREEN_HEIGHT - 1, cy + r);

    for (int y = y_start; y <= y_end; y++) {
        for (int x = x_start; x <= x_end; x++) {
            double dx = x - cx;
            double dy = y - cy;
            // Standard circle equation: x^2 + y^2 <= r^2
            if ((dx * dx) + (dy * dy) <= (r * r)) {
                put_pixel_fast(pixels, pitch, x, y, color);
            }
        }
    }
}

// Bresenham's line algorithm for drawing rays
void draw_line(SDL_Surface* surface, int x0, int y0, int x1, int y1, Uint32 color) {
    Uint32* pixels = (Uint32*)surface->pixels;
    int pitch = surface->pitch / 4;

    // Keep coordinates within screen bounds to prevent crashes
    x0 = clamp(x0, 0, SCREEN_WIDTH - 1);
    y0 = clamp(y0, 0, SCREEN_HEIGHT - 1);
    x1 = clamp(x1, 0, SCREEN_WIDTH - 1);
    y1 = clamp(y1, 0, SCREEN_HEIGHT - 1);

    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        put_pixel_fast(pixels, pitch, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Calculate the distance a ray travels before hitting a wall or circle
double calculate_intersection(Ray ray, const std::vector<Circle>& obstacles) {
    double min_t = 1e30; // Start with a very large distance

    // Check intersections with the four screen borders
    if (ray.dir.x != 0) {
        double t1 = (0 - ray.start.x) / ray.dir.x;
        double t2 = (SCREEN_WIDTH - ray.start.x) / ray.dir.x;
        if (t1 > 0) min_t = std::min(min_t, t1);
        if (t2 > 0) min_t = std::min(min_t, t2);
    }
    if (ray.dir.y != 0) {
        double t1 = (0 - ray.start.y) / ray.dir.y;
        double t2 = (SCREEN_HEIGHT - ray.start.y) / ray.dir.y;
        if (t1 > 0) min_t = std::min(min_t, t1);
        if (t2 > 0) min_t = std::min(min_t, t2);
    }

    // Mathematical Ray-Circle intersection
    for (const auto& circle : obstacles) {
        Vec2 L = {circle.pos.x - ray.start.x, circle.pos.y - ray.start.y};
        double tca = L.x * ray.dir.x + L.y * ray.dir.y;

        if (tca < 0) continue; // Circle is behind the ray

        double d2 = (L.x * L.x + L.y * L.y) - (tca * tca);
        double r2 = circle.radius * circle.radius;

        if (d2 > r2) continue; // Ray misses the circle entirely

        double thc = std::sqrt(r2 - d2);
        double t0 = tca - thc;

        if (t0 > 0 && t0 < min_t) {
            min_t = t0;
        }
    }
    return min_t;
}

int main(int argc, char* argv[]) {
    // SDL initialization
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Could not init SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Raytracer", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) return 1;

    // We draw to the surface and update the window once per frame
    SDL_Surface* screen = SDL_GetWindowSurface(window);
    if (!screen) return 1;

    // Pre-calculate colors based on the window's pixel format
    Uint32 black = map_color(screen->format, 0, 0, 0);
    Uint32 white = map_color(screen->format, 255, 255, 255);
    Uint32 yellow = map_color(screen->format, 255, 212, 59);
    Uint32 blue = map_color(screen->format, 50, 50, 255);

    // Initial scene setup
    Vec2 lightPos = {200, 200};
    std::vector<Circle> obstacles = {
        {{550, 300}, 80, {0, 150}}, // The moving one
        {{200, 100}, 30, {0, 0}},
        {{900, 450}, 60, {0, 0}}
    };

    // Pre-calculate ray directions (360 degrees worth)
    std::vector<Ray> rays(NUM_RAYS);
    for (int i = 0; i < NUM_RAYS; i++) {
        double angle = ((double)i / NUM_RAYS) * 2.0 * PI;
        rays[i].dir = {std::cos(angle), std::sin(angle)};
    }

    bool is_running = true;
    SDL_Event event;
    Uint64 last_tick = SDL_GetTicks();

    // Main Game Loop
    while (is_running) {
        // Calculate delta time for smooth movement
        Uint64 current_tick = SDL_GetTicks();
        double deltaTime = (current_tick - last_tick) / 1000.0;
        last_tick = current_tick;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                is_running = false;
            }
            // Update light source position to follow the mouse
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                lightPos.x = event.motion.x;
                lightPos.y = event.motion.y;
            }
        }

        // Logic: Update the bouncing circle position
        obstacles[0].pos.y += obstacles[0].velocity.y * deltaTime;
        if (obstacles[0].pos.y - obstacles[0].radius < 0 || 
            obstacles[0].pos.y + obstacles[0].radius > SCREEN_HEIGHT) {
            obstacles[0].velocity.y *= -1; // Reverse direction on hit
        }

        // --- RENDER START ---
        
        // Clear background
        SDL_FillSurfaceRect(screen, nullptr, black);

        // Lock surface to get direct access to pixels
        if (SDL_LockSurface(screen)) {
            
            // Loop through every ray and find where it hits
            for (int i = 0; i < NUM_RAYS; i++) {
                rays[i].start = lightPos;
                double dist = calculate_intersection(rays[i], obstacles);
                
                int targetX = (int)(rays[i].start.x + rays[i].dir.x * dist);
                int targetY = (int)(rays[i].start.y + rays[i].dir.y * dist);

                draw_line(screen, (int)rays[i].start.x, (int)rays[i].start.y, targetX, targetY, yellow);
            }

            // Draw the circles on top of the rays
            for (auto& obs : obstacles) {
                draw_filled_circle(screen, obs, blue);
            }
            
            // Draw a white dot at the light source/mouse position
            Circle playerDot = {lightPos, 10, {0,0}};
            draw_filled_circle(screen, playerDot, white);

            SDL_UnlockSurface(screen);
        }

        // Push the finished frame to the window
        SDL_UpdateWindowSurface(window);
    }

    // Cleanup
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}