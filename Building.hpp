#pragma once

#include "SceneNode.hpp"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
using namespace std;

#include <irrKlang.h>
using namespace irrklang;

enum class BuildType {
	Skyscraper,
	Apartment,
	Store
};

struct Block {
	float width, height, depth;
	unsigned int levels;
	glm::vec3 corner;
};

class Building {
public:
    Building(const float width, const float height, int levels, glm::vec3 corner, int doorI = -1, int windowI = -1, int roofI = -1, const float rotate = 0.0f, ISound *sound = NULL);

    virtual ~Building();  
    virtual void grow();
    virtual SceneNode *create();
    BuildType m_type;
    Block block;
    string encoding;
    int windowI, doorI, roofI, soundI;
    float rotate;
    ISound *audio;
    unordered_map<char, string> rewriteRule;
};


class Skyscraper : public Building {
public:
	Skyscraper(const float width, const float height, int levels, glm::vec3 corner, int doorI, int windowI, int roofI, const float rotate = 0.0f, ISound *sound = NULL);
	virtual ~Skyscraper();
};

class Store : public Building {
public:
	Store(const float width, const float height, int levels, glm::vec3 corner, int doorI, int windowI, int roofI, const float rotate = 0.0f, ISound *sound = NULL);
	virtual ~Store();
};

class Apartment : public Building {
public:
	Apartment(const float width, const float height, int levels, glm::vec3 corner, int doorI, int windowI, int roofI, const float rotate = 0.0f, ISound *sound = NULL);
	virtual ~Apartment();
	void grow();
};
