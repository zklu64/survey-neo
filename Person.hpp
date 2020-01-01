#pragma once

#include "SceneNode.hpp"
#include <glm/glm.hpp>
using namespace glm;

#include <irrKlang.h>
using namespace irrklang;

#include "Material.hpp"

struct Pair;

class Person {
public:
    Person(const float posx, const float posz, const float rotate, const Material &hair, const Material &shirt, 
	const Material &pants, const Material &skin, ISound *sound = NULL, ISound *panic = NULL, Pair *pairing = NULL);
    virtual ~Person();
    void move(const float posx, const float posz);
    void rotate(const float rot);
    void trigger();
    void run(glm::vec3 light);
    ISound *audio;
    ISound *panicAudio;
    Pair *pairing;
    bool react, paranoid;
    SceneNode *node;
    vec3 pos;
    float rotated;
    double secondsPassed;
    clock_t startTime;
private:
    SceneNode *create(const Material &hair, const Material &shirt,
        const Material &pants, const Material &skin);
};

struct Pair {
	Person *leader;
	Person *follower;
};
