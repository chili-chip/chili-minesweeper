#include "scene.hpp"

Scene::Scene() { }
Scene::~Scene() { }

void Scene::enter() { }
void Scene::leave() { }

void Scene::update() { }
void Scene::render() { }

Scene* Scene::next() {
    return _next;
}

bool Scene::finished() {
    return _finished;
}

void Scene::finish(Scene* next_scene) {
    _next = next_scene;
    _finished = true;
}
