#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_opengl3.h"
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
SDL_GLContext context;
bool running;
bool pressed;
int cam[3];
int mousepos[2];
entt::registry particles;
int frame;

struct Life {
  int age, lifespan;
};
struct Position {
  float x, y, z;
};
struct Size {
  int size;
};
struct Color {
  float sr, sg, sb, sa;
  float er, eg, eb, ea;
};
struct Physics {
  float velx, vely, velz;
  float accx, accy, accz;
  float jrkx, jrky, jrkz;
};
struct Trail {
  int min_lifespan, max_lifespan;
  float x, y, z;
  int min_size, max_size;
  float sr, sg, sb, sa;
  float er, eg, eb, ea;
  float speed;
  float accx, accy, accz;
  float jrkx, jrky, jrkz;
};

/* It's gonna be a */ long long time() {
  auto now = std::chrono::system_clock::now();
  auto dst = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(dst).count();
}
  
float rand01() {
  return (float) (rand() % 65536) / 65536;
}
float randn(float n) {
  return rand01() * n;
}
int randn(int n) {
  return (int) (rand01() * n);
}
float randf(float a, float b) {
  return a + randn(b - a);
}
int randint(int a, int b) {
  return a + randn(b - a);
}

int ip(int a, int b, Life life) {
  return a + (b - a) * ((float) life.age / life.lifespan);
}
float ip(float a, float b, Life life) {
  return a + (b - a) * life.age / life.lifespan;
}

std::array<float, 3> spin(float speed) {
  float x = randf(-1, 1), y = randf(-1, 1), z = randf(-1, 1);
  float dist = sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
  x *= speed / dist; y *= speed / dist; z *= speed / dist;
  return {x, y, z};
}

void handle() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;
      case SDL_MOUSEWHEEL:
        cam[2] += event.wheel.y * 80;
        break;
      case SDL_MOUSEBUTTONDOWN:
        pressed = true;
        break;
      case SDL_MOUSEBUTTONUP:
        pressed = false;
        break;
      case SDL_MOUSEMOTION:
        if (pressed) {
          cam[0] -= event.button.x - mousepos[0];
          cam[1] -= event.button.y - mousepos[1];
        }
        mousepos[0] = event.button.x;
        mousepos[1] = event.button.y;
        break;
    }
  }
}

void update() {
  std::vector<entt::entity> dying;
  auto view = particles.view<Life, Position, Physics>();
  for (auto particle : view) {
    auto& life = view.get<Life>(particle);
    auto& point = view.get<Position>(particle);
    auto& physics = view.get<Physics>(particle);
    point.x += physics.velx;
    point.y += physics.vely;
    point.z += physics.velz;
    physics.velx += physics.accx;
    physics.vely += physics.accy;
    physics.velz += physics.accz;
    physics.accx += physics.jrkx;
    physics.accy += physics.jrky;
    physics.accz += physics.jrkz;
    life.age++;
    if (life.age == life.lifespan) {
      dying.push_back(particle);
    }
  };
  for (auto particle : dying) {
    particles.destroy(particle);
  }
}

void fireworks() {
  auto vel = spin(randint(10, 20));
  auto trail = particles.create();
  particles.emplace<Life>(trail, 0, 60);
  particles.emplace<Position>(trail, 1000, 1000, 1000);
  particles.emplace<Physics>(trail, vel[0], vel[1], vel[2], 0, 0, 0, 0, 0, 0);
  particles.emplace<Trail>(trail,
    100, 125,
    0, 0, 0,
    3, 8,
    0.f, 0.f, 1.f, 1.f,
    0.f, 1.f, 1.f, 1.f,
    0.5,
    0, 0, 0,
    0, 0.01, 0);
}

void dotrails() {
  bool trailsexist = false;
  std::vector<Trail> trails;
  auto view = particles.view<Position, Trail>();
  for (auto particle : view) {
    auto& point = view.get<Position>(particle);
    auto& trail = view.get<Trail>(particle);
    trail.x = point.x;
    trail.y = point.y;
    trail.z = point.z;
    trails.push_back(trail);
    trailsexist = true;
  };
  for (Trail trail : trails) {
    for (int i = 0; i < 32; i++) {
      auto vel = spin(trail.speed);
      auto particle = particles.create();
      particles.emplace<Life>(particle, 0, randint(trail.min_lifespan, trail.max_lifespan));
      particles.emplace<Position>(particle, trail.x, trail.y, trail.z);
      particles.emplace<Size>(particle, randint(trail.min_size, trail.max_size));
      particles.emplace<Color>(particle, trail.sr, trail.sg, trail.sb, trail.sa, trail.er, trail.eg, trail.eb, trail.ea);
      particles.emplace<Physics>(particle, vel[0], vel[1], vel[2], trail.accx, trail.accy, trail.accz, trail.jrkx, trail.jrky, trail.jrkz);
    }
  }
  if (!trailsexist) {
    fireworks();
  }
}

void process() {
  update();
  dotrails();
}

void render() {
  glViewport(0, 0, 1000, 1000);
  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, 1000, 1000, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  auto view = particles.view<Life, Position, Size, Color>();
  for (auto particle : view) {
    auto& life = view.get<Life>(particle);
    auto& point = view.get<Position>(particle);
    auto& size = view.get<Size>(particle);
    auto& color = view.get<Color>(particle);
    if (point.z > cam[2]) {
      glColor4f(ip(color.sr, color.er, life), ip(color.sg, color.eg, life), ip(color.sb, color.eb, life), ip(color.sa, color.ea, life));
      float radius = ip(size.size, 0, life) / 2.f;
      float x = point.x, y = point.y;
      //x -= cam[0];
      //y -= cam[1];
      x = (x - cam[0]) / (point.z - cam[2]) * 1000 + 500;
      y = (y - cam[1]) / (point.z - cam[2]) * 1000 + 500;
      glBegin(GL_QUADS);
      glVertex2f(x - radius, y - radius);
      glVertex2f(x - radius, y + radius);
      glVertex2f(x + radius, y + radius);
      glVertex2f(x + radius, y - radius);
      glEnd();
    }
  }
  
  SDL_GL_SwapWindow(window);
} 

void stars() {
  for (int i = 0; i < pow(2, 11); i++) {
    auto star = particles.create();
    particles.emplace<Life>(star, 0, 1000000000);
    particles.emplace<Position>(star, rand01() * 2000, rand01() * 2000, rand01() * 2000);
    particles.emplace<Size>(star, randint(1, 5));
    particles.emplace<Color>(star, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
    particles.emplace<Physics>(star, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  }
}

void init() {
  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow("Trails", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 1000, SDL_WINDOW_OPENGL);
  context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, context);
  SDL_GL_SetSwapInterval(1);
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto io = ImGui::GetIO();
  (void) io;
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window, context);
  ImGui_ImplOpenGL3_Init("#version 130");
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  running = true;
  pressed = false;
  cam[0] = 1000; cam[1] = 1000; cam[2] = 0;
  frame = 0;
  srand(time(0));
}

void loop() {
  long long start = time();
  handle();
  process();
  render();
  int timepassed = time() - start;
  if (frame % 60 == 0) {
    std::cout << "FPS: " << (timepassed == 0 ? "∞" : std::to_string(1000 / timepassed)) << std::endl;
  }
  
  std::this_thread::sleep_for(std::chrono::milliseconds(std::max(0, (int) (1000.f / 60 - timepassed))));
  frame++;
}

void close() {
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
