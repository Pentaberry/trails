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
int alive;
int frames;
long long lasttime;


static float mdevt; //randomizes particle specifications for each emitter, including devp
static int maxemits;

static int flamet;
static float devt; //randomizes base aspects of the emitter: life, physics, and flame
static int lifet;
static float speedt;
static float accxt;
static float accyt;
static float acczt;
static float jrkxt;
static float jrkyt;
static float jrkzt;

static float devp; //aka emitter.dev; randomizes values of individual particles
static int lifep;
static int sizep;
static float srgbap[4];
static float ergbap[4];
static float speedp;
static float accxp;
static float accyp;
static float acczp;
static float jrkxp;
static float jrkyp;
static float jrkzp;

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
struct Emitter {
  int flame;
  float dev;
  int life;
  int size;
  float sr, sg, sb, sa;
  float er, eg, eb, ea;
  float speed;
  float accx;
  float accy;
  float accz;
  float jrkx;
  float jrky;
  float jrkz;
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
float dev(float n, float d) {
  return n * randf(1 - d, 1 + d);
}
int dev(int n, float d) {
  return n * randf(1 - d, 1 + d);
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
  ImGuiIO& io = ImGui::GetIO();
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (io.WantCaptureMouse && !pressed) {
      ImGui_ImplSDL2_ProcessEvent(&event);
    } else {
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
    alive--;
  }
}

void metaemit() {
  auto vel = spin(dev(speedt, devt));
  auto trail = particles.create();
  alive++;
  particles.emplace<Life>(trail, 0, dev(lifet, devt));
  particles.emplace<Position>(trail, randf(500, 1500), randf(500, 1500), randf(500, 1500));
  particles.emplace<Physics>(trail, vel[0], vel[1], vel[2], dev(accxt, devt), dev(accyt, devt), dev(acczt, devt), dev(jrkxt, devt), dev(jrkyt, devt), dev(jrkzt, devt));
  particles.emplace<Emitter>(trail, dev(flamet, devt), dev(devp, mdevt), dev(lifep, mdevt), dev(sizep, mdevt), dev(srgbap[0], mdevt), dev(srgbap[1], mdevt), dev(srgbap[2], mdevt), dev(srgbap[3], mdevt), dev(ergbap[0], mdevt), dev(ergbap[1], mdevt), dev(ergbap[2], mdevt), dev(ergbap[3], mdevt), dev(speedp, mdevt), dev(accxp, mdevt), dev(accyp, mdevt), dev(acczp, mdevt), dev(jrkxp, mdevt), dev(jrkyp, mdevt), dev(jrkzp, mdevt));
}
    

void emit() {
  int emitters = 0;
  auto view = particles.view<Position, Emitter>();
  for (auto emitting : view) {
    emitters++;
    auto& emitter = view.get<Emitter>(emitting);
    auto& point = view.get<Position>(emitting);
    for (int i = 0; i < emitter.flame; i++) {
      auto vel = spin(dev(emitter.speed, emitter.dev));
      auto particle = particles.create();
      float d = randf(1 - emitter.dev, 1 + emitter.dev);
      alive++;
      particles.emplace<Life>(particle, 0, dev(emitter.life, emitter.dev));
      particles.emplace<Position>(particle, point.x, point.y, point.z);
      particles.emplace<Size>(particle, dev(emitter.size, emitter.dev));
      particles.emplace<Color>(particle, emitter.sr * d, emitter.sg * d, emitter.sb * d, emitter.sa * d, emitter.er * d, emitter.eg * d, emitter.eb * d, emitter.ea * d);
      particles.emplace<Physics>(particle, vel[0], vel[1], vel[2], dev(emitter.accx, emitter.dev), dev(emitter.accy, emitter.dev), dev(emitter.accz, emitter.dev), dev(emitter.jrkx, emitter.dev), dev(emitter.jrky, emitter.dev), dev(emitter.jrkz, emitter.dev));
    }
  }
  if (emitters < maxemits) {
    metaemit();
  }
}

void process() {
  update();
  emit();
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
  
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  
  ImGui::SetNextWindowSize(ImVec2(350, 350), ImGuiCond_Once);
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
  ImGui::Begin("Emitter Controls");
  ImGui::SliderInt("Max Emitters", &maxemits, 1, 100);
  ImGui::SliderFloat("Meta-Deviation", &mdevt, 0.f, 1.f);
  ImGui::SliderFloat("Deviation", &devt, 0.f, 1.f);
  ImGui::SliderInt("Lifespan", &lifet, 30, 120);
  ImGui::SliderInt("Flame", &flamet, 0, 50);
  ImGui::SliderFloat("Speed", &speedt, 0.f, 2.f);
  ImGui::SliderFloat("Acc X", &accxt, -0.2f, 0.2f);
  ImGui::SliderFloat("Acc Y", &accyt, -0.2f, 0.2f);
  ImGui::SliderFloat("Acc Z", &acczt, -0.2f, 0.2f);
  ImGui::SliderFloat("Jerk X", &jrkxt, -0.1f, 0.1f);
  ImGui::SliderFloat("Jerk Y", &jrkyt, -0.1f, 0.1f);
  ImGui::SliderFloat("Jerk Z", &jrkzt, -0.1f, 0.1f);
  ImGui::End();

  ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_Once);
  ImGui::SetNextWindowPos(ImVec2(700, 0), ImGuiCond_Once);
  ImGui::Begin("Particle Controls");
  ImGui::SliderFloat("Deviation", &devp, 0.f, 1.f);
  ImGui::SliderInt("Lifespan", &lifep, 30, 120);
  ImGui::SliderInt("Size", &sizep, 1, 10);
  ImGui::SliderFloat("Speed", &speedp, 0.f, 2.f);
  ImGui::SliderFloat("Acc X", &accxp, -0.2f, 0.2f);
  ImGui::SliderFloat("Acc Y", &accyp, -0.2f, 0.2f);
  ImGui::SliderFloat("Acc Z", &acczp, -0.2f, 0.2f);
  ImGui::SliderFloat("Jerk X", &jrkxp, -0.1f, 0.1f);
  ImGui::SliderFloat("Jerk Y", &jrkyp, -0.1f, 0.1f);
  ImGui::SliderFloat("Jerk Z", &jrkzp, -0.1f, 0.1f);
  ImGui::ColorPicker4("Start Color", srgbap);
  ImGui::ColorPicker4("End Color", ergbap);
  ImGui::End();
  
  ImGui::Render();
  
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  SDL_GL_SwapWindow(window);
} 

void stars() {
  for (int i = 0; i < pow(2, 10); i++) {
    auto star = particles.create();
    alive++;
    particles.emplace<Life>(star, 0, 1000000000);
    particles.emplace<Position>(star, rand01() * 2000, rand01() * 2000, rand01() * 2000);
    particles.emplace<Size>(star, randint(1, 5));
    particles.emplace<Color>(star, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
    particles.emplace<Physics>(star, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  }
}

void init() {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  window = SDL_CreateWindow("Trails", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 1000, SDL_WINDOW_OPENGL);
  context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, context);
  SDL_GL_SetSwapInterval(1);
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window, context);
  ImGui_ImplOpenGL3_Init("#version 130");
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  maxemits = 3;
  mdevt = 1;
  devt = 0.125;
  lifet = 60;
  speedt = 16;
  accxt = 0;
  accyt = 0;
  acczt = 0;
  jrkxt = 0;
  jrkyt = 0;
  jrkzt = 0;
  flamet = 10;

  devp = 0.125;
  lifep = 120;
  sizep = 6;
  srgbap[0] = 0; srgbap[1] = 0; srgbap[2] = 1; srgbap[3] = 1; //blue
  ergbap[0] = 1; ergbap[1] = 1; ergbap[2] = 0; ergbap[3] = 1; //yellow
  speedp = 0.5;
  accxp = 0;
  accyp = 0;
  acczp = 0;
  jrkxp = 0;
  jrkyp = -0.01;
  jrkzp = 0;

  running = true;
  pressed = false;
  cam[0] = 1000; cam[1] = 1000; cam[2] = -2000;
  frames = 0;
  lasttime = time();
  alive = 0;
  srand(time(0));
}

void loop() {
  handle();
  process();
  render();
  frames++;
  long long span = (time() - lasttime) / 1000;
  if (span > 1) {
    std::cout << "FPS: " << (frames / span) << std::endl;
    std::cout << "Particles: " << alive << std::endl;
    frames = 0;
    lasttime = time();
  }
  
  //std::this_thread::sleep_for(std::chrono::milliseconds(std::max(0, (int) (1000.f / 60 - timepassed))));
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
