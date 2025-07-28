#pragma once

class Scene {
public:
    Scene();
    virtual ~Scene();

    virtual void enter();
    virtual void leave();

    virtual void update();
    virtual void render();

    Scene* next();
    bool finished();

protected:
    void finish(Scene* next_scene);

private:
    Scene* _next = nullptr;
    bool _finished = false;
};
