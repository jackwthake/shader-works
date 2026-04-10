#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL3/SDL.h>

#include <shader-works/renderer.h>
#include <shader-works/primitives.h>

#define WIN_WIDTH 320
#define WIN_HEIGHT 240
#define WIN_SCALE 4
#define WIN_TITLE "zombies"
#define MAX_DEPTH 15

#define UNUSED(X) (void)(X)

typedef enum { DIR_NORTH, DIR_SOUTH, DIR_EAST, DIR_WEST } dir_t;

typedef struct {
  int x, y, w, h;
} rect_t;

typedef struct {
  int x, y;
  dir_t dir;
} walker_t;

#define MAX_ROOMS 128
#define MAX_CORRIDORS 128

rect_t rooms[MAX_ROOMS];
rect_t corridors[MAX_CORRIDORS];
int num_rooms = 0;
int num_corridors = 0;

float rand_float_range(float min, float max) {
  float scale = rand() / (float) RAND_MAX;
  return min + scale * (max - min);
}

// Simple function to remove the last corridor if it matches any previous one
void remove_duplicate_corridor() {
  for (int i = 0; i < num_corridors; i++) {
    for (int j = i + 1; j < num_corridors; j++) {
      // If corridor J is the same as corridor I
      if (corridors[i].x == corridors[j].x &&
        corridors[i].y == corridors[j].y &&
        corridors[i].w == corridors[j].w &&
        corridors[i].h == corridors[j].h) {

          // Shift everything after J one slot to the left
          for (int k = j; k < num_corridors - 1; k++) {
            corridors[k] = corridors[k + 1];
          }
          num_corridors--;
          j--; // Re-check this index since a new element was shifted into it
      }
    }
  }
}

void sdl_setup(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **framebuffer_tex) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(WIN_TITLE, WIN_WIDTH * WIN_SCALE, WIN_HEIGHT * WIN_SCALE, 0, window, renderer);
  *framebuffer_tex = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  SDL_SetTextureScaleMode(*framebuffer_tex, SDL_SCALEMODE_NEAREST);
}

void walker_step(walker_t *walker) {
  if (num_corridors >= MAX_CORRIDORS) return;

  int seg_w = 6;
  int seg_len = 24;

  // 1. PLACE CORRIDOR (No questions asked)
  rect_t seg;
  if (walker->dir == DIR_EAST || walker->dir == DIR_WEST) {
    seg.w = seg_len; seg.h = seg_w;
    seg.x = (walker->dir == DIR_WEST) ? walker->x - seg_len : walker->x;
    seg.y = walker->y - seg_w / 2;
  } else {
    seg.w = seg_w; seg.h = seg_len;
    seg.x = walker->x - seg_w / 2;
    seg.y = (walker->dir == DIR_NORTH) ? walker->y - seg_len : walker->y;
  }

  corridors[num_corridors++] = seg;

  // 3. ALWAYS ADVANCE HEAD (Even if we deleted the corridor, we keep the snake moving)
  if (walker->dir == DIR_EAST)  walker->x += seg_len;
  else if (walker->dir == DIR_WEST)  walker->x -= seg_len;
  else if (walker->dir == DIR_SOUTH) walker->y += seg_len;
  else if (walker->dir == DIR_NORTH) walker->y -= seg_len;

  // 4. ROOM NODES AND BRANCHING
  float roll = rand_float_range(0, 1);
  if (roll < 0.2f && num_rooms < MAX_ROOMS) {
    rooms[num_rooms++] = (rect_t){walker->x - 2, walker->y - 2, 4, 4};
  }

  if (roll < 0.4f) {
    walker->dir = (dir_t)(rand() % 4);
  } else if (roll > 0.8f && num_rooms > 0) {
    int r_idx = rand() % num_rooms;
    walker->x = rooms[r_idx].x + 2;
    walker->y = rooms[r_idx].y + 2;
    walker->dir = (dir_t)(rand() % 4);
  }
}

int main(int argc, char const *argv[]) {
  UNUSED(argc); UNUSED(argv);
  SDL_Window *window; SDL_Renderer *renderer; SDL_Texture *fb_tex;
  srand(time(NULL));
  sdl_setup(&window, &renderer, &fb_tex);

  walker_t walker = { WIN_WIDTH / 2, WIN_HEIGHT / 2, DIR_EAST };
  rooms[num_rooms++] = (rect_t){walker.x - 2, walker.y - 2, 4, 4};

  for (int i = 0; i < 35; i++) {
    walker_step(&walker);
  }

  remove_duplicate_corridor();

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) running = false;
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_SPACE) {
        walker_step(&walker);
        remove_duplicate_corridor();
      }
    }

    SDL_SetRenderDrawColor(renderer, 10, 10, 15, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    for (int i = 0; i < num_corridors; i++) {
      SDL_FRect r = { corridors[i].x * WIN_SCALE, corridors[i].y * WIN_SCALE, corridors[i].w * WIN_SCALE, corridors[i].h * WIN_SCALE };
      SDL_RenderFillRect(renderer, &r);
    }

    SDL_SetRenderDrawColor(renderer, 0, 255, 100, 255);
    for (int i = 0; i < num_rooms; i++) {
      SDL_FRect r = { rooms[i].x * WIN_SCALE, rooms[i].y * WIN_SCALE, rooms[i].w * WIN_SCALE, rooms[i].h * WIN_SCALE };
      SDL_RenderFillRect(renderer, &r);
    }

    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
    SDL_FRect wr = { (walker.x - 3) * WIN_SCALE, (walker.y - 3) * WIN_SCALE, 6 * WIN_SCALE, 6 * WIN_SCALE };
    SDL_RenderFillRect(renderer, &wr);

    SDL_RenderPresent(renderer);
  }

  SDL_Quit();
  return 0;
}
