#include "Building.hpp"
#include <stdlib.h> 
//#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
using namespace std;
#include "GeometryNode.hpp"
#include "Material.hpp"
const int brokenWindowIndex = 1;

Building::Building(const float width, const float height, int levels, glm::vec3 corner, int doorI, int windowI, int roofI, const float rotate, ISound *sound) {
	this->doorI = doorI;
	this->windowI = windowI;
	this->roofI = roofI;
	this->audio = sound;
	this->rotate = rotate;
	block.width = width;
	block.height = height;
	block.depth = 0.5;
	block.levels = levels;
	block.corner = glm::vec3(corner);
	//w is window, i is intermediate floor, d is door, t is overwritten depending on inherited building class
	encoding = "B";
	rewriteRule['B'] = "GIT";
	rewriteRule['I'] = "FI";
	rewriteRule['F'] = "W|W|W|W|/";
	rewriteRule['W'] = "[2 r]w";
	rewriteRule['G'] = "D|W|W|W|/";
	rewriteRule['D'] = "[2 r]d";
	//2 r means repeat width of 2, may implement [int x] later, meaning create terminal symbol mesh width int, x number of times
} 

Building::~Building() {
}

Store::~Store(){}

Apartment::~Apartment(){}

Skyscraper::~Skyscraper(){}

Store::Store(const float width, const float height, int levels, glm::vec3 corner, int doorI, int windowI, int roofI, const float rotate, ISound *sound)
	:Building(width, height, levels, corner, doorI, windowI, roofI, rotate, sound)
{	
	rewriteRule['T'] = "c/";
	m_type = BuildType::Store;
}

Apartment::Apartment(const float width, const float height, int levels, glm::vec3 corner, int doorI, int windowI, int roofI, const float rotate, ISound *sound) 
        :Building(width, height, levels, corner, doorI, windowI, roofI, rotate, sound)
{
	rewriteRule['T'] = "f/"; //flat roof
	m_type = BuildType::Apartment;	
}

Skyscraper::Skyscraper(const float width, const float height, int levels, glm::vec3 corner, int doorI, int windowI, int roofI, const float rotate, ISound *sound)
        :Building(width, height, levels, corner, doorI, windowI, roofI, rotate, sound)
{
        int chance = rand() % 100;
        if (chance < 30) {
		rewriteRule['T'] = "s/"; //sphere roof
	} else {
                rewriteRule['T'] = "f/"; //flat roof
        }
	//rewriteRule['W'] = "[1 r]w";
        m_type = BuildType::Skyscraper;
}

void Building::grow()
{
	int e = 0;
	int terminal = 0;
	while (terminal != encoding.size())
	{
		terminal = 0;
		string rewrite = "";
		for (size_t i = 0; i < encoding.size(); i++)
		{
			char ch = encoding[i];
			if (ch == 'I' || ch == 'T' || ch == 'G') {
				if (e < block.levels) {
					e++;
				} else {
					continue;
				}
			}
			if (rewriteRule.find(ch) != rewriteRule.end())
			{
				rewrite += rewriteRule[ch];
			}
			else
			{
				terminal++;
				rewrite += ch;
			}
			
		}
		// Assign expansion to next iteration
		encoding = rewrite;
	}
}

void Apartment::grow(){
	//overloaded method since we want chance of broken windows
	int e = 0;
        int terminal = 0;
        while (terminal != encoding.size())
        {
                terminal = 0;
                string rewrite = "";
                for (size_t i = 0; i < encoding.size(); i++)
                {
                        char ch = encoding[i];
                        if (ch == 'I' || ch == 'T' || ch == 'G') {
                                if (e < block.levels) {
                                        e++;
                                } else {
                                        continue;
                                }
                        }
                        if (rewriteRule.find(ch) != rewriteRule.end())
                        {
				if (ch == 'W') {
					int chance = rand() % 100;
					if (chance < 10) {
						//broken window
						rewrite += "[2 r]b";
					} else {
						rewrite += rewriteRule[ch];
					}
				} else {
                        	        rewrite += rewriteRule[ch];
				}
                        }
                        else
                        {
                                terminal++;
                                rewrite += ch;
                        }
                }
                // Assign expansion to next iteration
                encoding = rewrite;
        }
}

SceneNode *Building::create() {
	SceneNode *root = new SceneNode("building");
	root->rotate('y', rotate);
	root->translate(block.corner + glm::vec3(block.width/2, 0.0, -block.width/2));
	size_t c = count(encoding.begin(), encoding.end(), '/') - 1;
	if (c < 1) return root;
	float incY = block.height / c;
	int x = 0;
	//faces offset for cube
	glm::vec3 faces[4]={glm::vec3(0.0, 0.0, block.width/2 - block.depth/2), glm::vec3(-block.width/2 + block.depth/2, 0.0, 0.0), 
			glm::vec3(0.0, 0.0, -block.width/2 + block.depth/2),
			glm::vec3(block.width/2 - block.depth/2, 0.0, 0.0)};
	string temp = encoding;
	//corner is always lowerLeft
	while (temp.length() != 0) {
		string delimiter = "/";
		string token = temp.substr(0, temp.find(delimiter));
		temp = temp.substr(temp.find(delimiter)+1, string::npos);

		if (token[0] == '[') {
			delimiter = "|";
			int i = 0;
			while (token.length() != 0) {
				//each floor
				string token2 = token.substr(0, token.find(delimiter));
				token = token.substr(token.find(delimiter)+1, string::npos);
				bool door = (token2.back() == 'd');
				double center; //want door to be further back than windows
				float x = 0.0;
				float incX = (token2[1] - '0');
				int blocks = floor(block.width / incX);
				int c = 0;
				if (door) {
					center = (float)(blocks-1)/2.0f;
				}
				while (c < blocks) {
					Material mat = Material();
					mat.kd = glm::vec4(0.3, 0.3, 0.3, 0.2);
					GeometryNode *window = new GeometryNode("cube", "window");
					window->textureIndex = windowI;
					if (door && m_type == BuildType::Skyscraper) {
						window->m_name = "door";
						window->textureIndex = doorI;
					}
					if (token2.back() == 'b') {
						window->m_name = "broken";
						window->textureIndex = brokenWindowIndex;
					}
					if (i % 2 == 0) {
						//face along x axis
						window->scale(glm::vec3(incX, incY, block.depth));
						window->translate(faces[i] + glm::vec3(x - block.width/2 + incX/2, incY/2, 0.0));
						if (door && (c == floor(center) || c == ceil(center)) && m_type != BuildType::Skyscraper) {
							window->translate(glm::vec3(0.0, 0.0, -block.depth/2.0f));
							window->m_name = "door";
							window->textureIndex = doorI;
						}
					} else {
						window->scale(glm::vec3(block.depth+0.01, incY, incX-0.01));
						//the -0.01 is to prevent flickering since these walls perfectly overlaps x axis walls without it
						window->translate(faces[i] + glm::vec3(0.0, incY/2, -x + block.width/2 - incX/2));
						if (door && (c == floor(center) || c == ceil(center)) && m_type != BuildType::Skyscraper) {
							window->translate(glm::vec3(block.depth/2.0f, 0.0, 0.0));
							window->m_name = "door";
							window->textureIndex = doorI;
						}
					}
					window->material = mat;
					root->add_child(window);
					x += incX;
					++c;
				}
				++i;
			}
			faces[0] = faces[0] + glm::vec3(0.0, incY, 0.0);
			faces[1] = faces[1] + glm::vec3(0.0, incY, 0.0);
			faces[2] = faces[2] + glm::vec3(0.0, incY, 0.0);
			faces[3] = faces[3] + glm::vec3(0.0, incY, 0.0);
		} else {
			Material mat = Material();
			mat.kd = glm::vec4(0.3, 0.3, 0.3, 0.2);
			if (token[0] == 'c') {
				GeometryNode *roof = new GeometryNode("cylinder", "roof");
				roof->material = mat;
				roof->rotate('x', 90.0);
				roof->scale(glm::vec3(block.width/2 - 0.05, block.height/3, block.width/2 - 0.05));
				roof->translate(glm::vec3(0, faces[0].y, 0));
				roof->textureIndex = roofI;
				root->add_child(roof);
			} else if (token[0] == 's') {
                                GeometryNode *roof = new GeometryNode("sphere", "roof");
				roof->material = mat;
                                roof->scale(glm::vec3(block.width/2 - 0.05, 5.0, block.width/2 - 0.05));
                                roof->translate(glm::vec3(0, faces[0].y-2.5, 0));
                                roof->textureIndex = roofI;
                                root->add_child(roof);
				roof = new GeometryNode("cube", "roof");
				roof->material = mat;
                                roof->scale(glm::vec3(block.width - 0.05, 0.5, block.width - 0.05));
                                roof->translate(glm::vec3(0, faces[0].y, 0));
                                roof->textureIndex = roofI;
                                root->add_child(roof);
			} else if (token[0] == 'f') {
                                GeometryNode *roof = new GeometryNode("cube", "roof");
				roof->material = mat;
                                roof->scale(glm::vec3(block.width - 0.05, 0.5, block.width - 0.05));
                                roof->translate(glm::vec3(0, faces[0].y, 0));
                                roof->textureIndex = roofI;
                                root->add_child(roof);
                        }
		}
	}
	return root;
}
