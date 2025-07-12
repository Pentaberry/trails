#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>
#include <iostream>
#include <entt/entt.hpp>
//Will separate between files later if needed

struct Position {
  float x, y, z;
};
struct Size {
  float size;
};

sf::Color rgba(int r, int g, int b, int a) {
  return sf::Color(r, g, b, a);
}
sf::Vector2f xy(float x, float y) {
  return sf::Vector2f(x, y);
}

float rand01() {
  return (float) (rand() % 65536) / 65536;
}
int randn(int n) {
  return (int) (rand01() * n);
}

void handle(sf::RenderWindow* window, bool* pressed, sf::Vector2i* mousepos, float* camx, float* camy) {
  sf::Event event;
  while (window->pollEvent(event)) {
    switch (event.type) {
      case sf::Event::EventType::Closed:
        window->close();
        break;
      default:
        break;
      }
    }
  if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
    sf::Vector2i newmouse = sf::Mouse::getPosition(*window);
    if (*pressed) {
      *camx += mousepos->x - newmouse.x;
      *camy += mousepos->y - newmouse.y;
    }
    *mousepos = newmouse;
    *pressed = true;
  } else {
    *pressed = false;
  }
}

void render(sf::RenderWindow* window, entt::registry* particles, float camx, float camy) {
  window->clear(rgba(0, 0, 0, 255));
  for (auto particle : particles->view<Position, Size>()) {
    auto& point = particles->get<Position>(particle);
    auto& size = particles->get<Size>(particle);
    float x = point.x, y = point.y;
    x = (x - camx - 500) / (point.z / 500) + camx + 500;
    y = (y - camy - 500) / (point.z / 500) + camy + 500;
    sf::RectangleShape visual(xy(size.size, size.size));
    visual.setFillColor(rgba(255, 255, 255, 255));
    visual.setPosition(xy(x - size.size / 2, y - size.size / 2));
    window->draw(visual);
  }
  window->display();
}
  

void stars(entt::registry* particles) {
  for (int i = 0; i < 10000; i++) {
    auto star = particles->create();
    particles->emplace<Position>(star, rand01() * 2000, rand01() * 2000, rand01() * 200);
    particles->emplace<Size>(star, rand01() * 4 + 1);
  }
}

int main() {
  sf::RenderWindow window(sf::VideoMode(1000, 1000), "Trails", sf::Style::Titlebar | sf::Style::Close);
  window.setFramerateLimit(60);
  srand(time(0));
  
  float camx = 500, camy = 500;
  bool pressed = false;
  sf::Vector2i mousepos;
  entt::registry particles;
  stars(&particles);
 
  while (window.isOpen()) {
    handle(&window, &pressed, &mousepos, &camx, &camy);
    render(&window, &particles, camx, camy);
  }
  return 0;
}

