#include <SDL2/SDL.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>
#include <entt/entt.hpp>
//Will separate between files later if needed

SDL_Window* window;
SDL_Renderer* renderer;
bool running;
bool pressed;
int cam[2];
int mousepos[2];
entt::registry particles;
int frame;

struct Position {
  float x, y, z;
};
struct Size {
  int size;
};

/* It's gonna be a */ long long time() {
  auto now = std::chrono::system_clock::now();
  auto dst = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(dst).count();
}
  
float rand01() {
  return (float) (rand() % 65536) / 65536;
}
int randn(int n) {
  return (int) (rand01() * n);
}
int randint(int a, int b) {
  return a + randn(b - a);
}

void handle() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;
      case SDL_MOUSEBUTTONDOWN:
        pressed = true;
        break;
      case SDL_MOUSEBUTTONUP:
        pressed = false;
        break;
      case SDL_MOUSEMOTION:
        if (pressed) {
          cam[0] += mousepos[0] - event.button.x;
          cam[1] += mousepos[1] - event.button.y;
        }
        mousepos[0] = event.button.x;
        mousepos[1] = event.button.y;
        break;
    }
  }
}

void render() {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

  auto view = particles.view<Position, Size>();
  auto& size_s = particles.storage<Size>();
  auto& point_s = particles.storage<Position>();

  for (auto particle : view) {
    auto& point = point_s.get(particle);
    auto& size = size_s.get(particle);
    float x = point.x, y = point.y;
    x = (x - cam[0] - 500) / (point.z / 500) + cam[0] + 500;
    y = (y - cam[1] - 500) / (point.z / 500) + cam[1] + 500;
    SDL_Rect star = {(int) x, (int) y, size.size, size.size};
    SDL_RenderFillRect(renderer, &star);
  }
  SDL_RenderPresent(renderer);
}
  

void stars() {
  for (int i = 0; i < pow(2, 14); i++) {
    auto star = particles.create();
    particles.emplace<Position>(star, rand01() * 2000, rand01() * 2000, rand01() * 200);
    particles.emplace<Size>(star, randint(1, 5));
  }
}

void init() {
  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow("Trails", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 1000, 0);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  running = true;
  pressed = false;
  cam[0] = 500; cam[1] = 500;
  frame = 0;
  srand(time(0));
}

void loop() {
  long long start = time();
  handle();
  render();
  int timepassed = time() - start;
  if (frame % 60 == 0) {
    std::cout << "FPS: " << (1000 / timepassed) << std::endl;
  }
  
  //No need to sleep; nothing is computing per tick yet
  //std::this_thread::sleep_for(std::chrono::milliseconds(std::max(0, (int) (1000.f / 60 - timepassed))));
  frame++;
}

void close() {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

int main() {
  init();
  stars();
 
  while (running) {
    loop();
  }

  close();
  return 0;
}
