//

#include "Project.hpp"
#include "scene_lua.hpp"

#include <cmath>
using namespace std;
#include "framework/GlErrorCheck.hpp"
#include "framework/MathUtils.hpp"
#include <imgui/imgui.h>
#include "stb_image.h"
#include "Material.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
using namespace glm;

#pragma comment(lib, "irrKlang.lib") // link with irrKlang.dll

static bool show_gui = true;
const float soundSpeed = 343.0f; //speed of sound in air m/s
ISoundEngine *SoundEngine = createIrrKlangDevice();

std::ostream &operator<< (std::ostream &out, const glm::vec3 &vec) {
    out << "{"
        << vec.x << " " << vec.y << " "<< vec.z
        << "}";

    return out;
}

//----------------------------------------------------------------------------------------
// Constructor
Project::Project(const std::string & luaSceneFile)
	: m_luaSceneFile(luaSceneFile),
	  m_positionAttribLocation(0),
	  m_normalAttribLocation(0),
	  m_texAttribLocation(0),
	  m_vao_meshData(0),
	  m_vbo_vertexPositions(0),
	  m_vbo_vertexNormals(0),
	  m_vbo_vertexUVs(0),
	  isPerson(false), infraredMode(false), freeMode(false), lookMode(false), textureMode(true), wPressed(false), aPressed(false), sPressed(false), dPressed(false), ePressed(false), qPressed(false), yaw(0.0), pitch(0.0)
{
	m_dir = vec3(0.0f, 0.0f, -1.0f);
	camPos = vec3(0.0f, 2.0f, 0.0f);
	camUp = vec3(0.0f, 1.0f, 0.0f);
    velocity = vec3(0.0f, 0.0f, 0.0f);

	//for audio calculations
	ground1 = vec3(-125, 0, 125);
	ground2 = vec3(125, 0, 125);
	ground3 = vec3(125, 0, -125);
}

//----------------------------------------------------------------------------------------
// Destructor
Project::~Project()
{
	if (background) {
		background->drop();
	}
	SoundEngine->drop();
}


//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void Project::init()
{
	// Set the background colour.
	glClearColor(0.0, 0.0, 0.0, 1.0);
	createShaderProgram();

	glGenVertexArrays(1, &m_vao_meshData);
	enableVertexShaderInputSlots();

	processLuaSceneFile(m_luaSceneFile);
	// Load and decode all .obj files at once here.  You may add additional .obj files to
	// this list in order to support rendering additional mesh types.  All vertex
	// positions, and normals will be extracted and stored within the MeshConsolidator
	// class.
	unique_ptr<MeshConsolidator> meshConsolidator (new MeshConsolidator{
			getAssetFilePath("cube.obj"),
			getAssetFilePath("scube.obj"),
			getAssetFilePath("sphere.obj"),
			getAssetFilePath("cone.obj"),
			getAssetFilePath("cylinder.obj")
	});


	// Acquire the BatchInfoMap from the MeshConsolidator.
	meshConsolidator->getBatchInfoMap(m_batchInfoMap);

	// Take all vertex data within the MeshConsolidator and upload it to VBOs on the GPU.
	uploadVertexDataToVbos(*meshConsolidator);

	mapVboDataToVertexShaderInputLocations();

	initPerspectiveMatrix();

	initViewMatrix();

	initLightSources();
	// Exiting the current scope calls delete automatically on meshConsolidator freeing
	// all vertex data resources.  This is fine since we already copied this data to
	// VBOs on the GPU.  We have no use for storing vertex data on the CPU side beyond
	// this point.
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	initAudio();
	initModels();
}

//----------------------------------------------------------------------------------------
void Project::processLuaSceneFile(const std::string & filename) {
	// This version of the code treats the Lua file as an Asset,
	// so that you'd launch the program with just the filename
	// of a puppet in the Assets/ directory.
	// std::string assetFilePath = getAssetFilePath(filename.c_str());
	// m_rootNode = std::shared_ptr<SceneNode>(import_lua(assetFilePath));

	// This version of the code treats the main program argument
	// as a straightforward pathname.
	m_rootNode = std::shared_ptr<SceneNode>(import_lua(filename));
	if (!m_rootNode) {
		std::cerr << "Could Not Open " << filename << std::endl;
	}
}

void Project::fillStreet(float startx, float startz, int leftoverSpace, const char axis, const char facing) {
	SceneNode *node;
	while (leftoverSpace != 0) {
		//we want space to be even, around 2-6 units, but also depend on how much leftover space we have on the street
		int space = clamp((rand() % leftoverSpace/2 + 1)*2, 2, 8);
		leftoverSpace -= space;
		if ( (rand() % 100) < 50) {
			(axis == 'x' ? startx+=space : startz-=space);
			continue;
		}

		//try to get some space between this building and the previous
		int width = clamp( space - 2, 2, 6);
		int delta = rand() % (space - width + 1);
		//we place our building at x = startx + delta

		int poorDoorI = rand() % 9 + 15;
		int poorWindowI = rand() % 10 + 24;
		int poorRoofI = rand() % 10 + 34;
		int height;
		bool apartment = false;
		if (rand() % 100 < 5) {
			apartment = true;
			height = rand() % 5 + 10;
		} else {
			height = rand() % 3 + 3;
		}
		int rotate = 0, perturb;
		perturb = rand() % 2; //shift on the axis that buildings aren't filling along

		if (facing == 'N') {
			rotate = 180;
			perturb *= -1;
		} else if (facing == 'E') {
			rotate = 90;
		} else if (facing == 'W') {
			rotate = 270;
			perturb *= -1;
		}

		Building *building;
		if (axis == 'x') {
			if (apartment) {
				building = new Apartment(width, height, height, vec3(startx + delta, 0.0, startz - perturb), poorDoorI, poorWindowI, poorRoofI, rotate);
			} else {
				building = new Store(width, height, height, vec3(startx + delta, 0.0, startz - perturb), poorDoorI, poorWindowI, poorRoofI, rotate);
				int achance = rand() % 100; //audio chance
				if (achance < 2) {
					int index = rand() % 6 + 5;
					building->audio = SoundEngine->play3D(("Assets/sounds/"+ soundPaths[index]).c_str(), vec3df( (startx + delta) + width/2, 0, startz - perturb - width/2), true, false, true);
					building->audio->setMinDistance(0.5);
				}
			}
			startx += space;
		} else {
			if (apartment) {
				building = new Apartment(width, height, height, vec3(startx - perturb, 0.0, startz - delta), poorDoorI, poorWindowI, poorRoofI, rotate);
			} else {
				building = new Store(width, height, height, vec3(startx - perturb, 0.0, startz - delta), poorDoorI, poorWindowI, poorRoofI, rotate);
				int achance = rand() % 100; //audio chance
				if (achance < 2) {
					int index = rand() % 6 + 5;
					building->audio = SoundEngine->play3D(("Assets/sounds/"+ soundPaths[index]).c_str(), vec3df( (startx - perturb) + width/2, 0, startz - delta - width/2), true, false, true);
					building->audio->setMinDistance(0.5);
				}
			}
			startz -= space;
		}
		building->grow();
		node = building->create();
		m_rootNode->add_child(node);
	}
}

void Project::placePeople(int startx, int endx, int startz, int endz) {
	for (int x = startx; x < endx; ++x) {
		for (int z = startz; z > endz; --z) {
			int chance = rand() % 200;
			if (chance < 1) {
				int rotated = rand() % 359;
				int hairI = rand() % 9;
				int pantsI = rand() % 9;
				int shirtI = rand() % 9;
				int skinI = rand() % 4 + 9;
				int achance = rand() % 100; // now see if we assign audio to person
				ISound* audio = NULL;
				ISound* panicAudio = NULL;
				if (achance < 5) {
					int index = rand() % 9 + 11;
					audio = SoundEngine->play3D(("Assets/sounds/"+ soundPaths[index]).c_str(), vec3df(x,0,z), true, false, true);
					audio->setMinDistance(0.3);
					int pindex = 29;
					if (index == 11) pindex = 25;
					if (index == 16) pindex = 26;
					if (index == 19) pindex = 27;
					if (index == 14) pindex = 28;
					panicAudio = SoundEngine->play3D(("Assets/sounds/"+ soundPaths[pindex]).c_str(), vec3df(x,0,z), true, false, true);
					panicAudio->setMinDistance(0.5);
				}
				Person *p = new Person(x, z, rotated, materials[hairI], materials[shirtI], materials[pantsI], materials[skinI], audio, panicAudio);
				m_rootNode->add_child(p->node);
				people.push_back(p);
			}
		}
	}
}

void Project::initModels() {
	Material gray = Material(vec4(0.2, 0.2, 0.2, 1.0), vec3(0.1, 0.1, 0.1), 10.0f);
	Material purple = Material(vec4(0.3, 0.21, 0.34, 1.0), vec3(0.1, 0.1, 0.1), 10.0f);
	Material wine = Material(vec4(0.33, 0.02, 0.15, 1.0), vec3(0.1, 0.1, 0.1), 10.0f);
	Material red = Material(vec4(0.584, 0.184, 0.157, 1.0), vec3(0.1, 0.1, 0.1), 10.0f);
	Material blue = Material(vec4(0.46, 0.67, 0.76, 1.0), vec3(0.1, 0.1, 0.1), 10.0f);
	Material green = Material(vec4(0.208, 0.32, 0.29, 1.0), vec3(0.1, 0.1, 0.1), 10.0f);
	Material yellow = Material(vec4(1.0, 1.0, 0.5, 1.0), vec3(0.1, 0.1, 0.1), 10.0f);
	Material white = Material(vec4(1.0, 1.0, 1.0, 1.0), vec3(0.1, 0.1, 0.1), 10.0f);
	Material black = Material(vec4(0.0, 0.0, 0.0, 1.0), vec3(0.1, 0.1, 0.1), 10.0f);
	Material pale = Material(vec4(1.0, 0.9, 0.78, 1.0), vec3(0.1, 0.1, 0.1), 10.0f);
	Material beige = Material(vec4(1.0, 0.86, 0.7, 1.0), vec3(0.1, 0.1, 0.1), 10.0f);
	Material tanned = Material(vec4(0.28, 0.23, 0.2, 1.0), vec3(0.1, 0.1, 0.1), 10.0f);
	Material dark = Material(vec4(0.145, 0.1137, 0.086, 1.0), vec3(0.1, 0.1, 0.1), 10.0f);

	materials = {gray, purple, wine, red, blue, green, yellow, white, black, pale, beige, tanned, dark};

	std::vector<std::string> texturePaths = {"road.jpg", "broken.jpg", "ground.jpg",
		"door1.jpg", "door2.jpg", "door3.jpg", "door4.jpg", //nice buildings' doors index 3-6
		"window1.jpg", "window2.jpg", "window3.jpg", "window4.jpg", //index 7-10
		"roof1.jpg","roof2.jpg","roof3.jpg","roof4.jpg", //11-14
		"door100.jpg", "door101.jpg", "door102.jpg", "door103.jpg", "door104.jpg", "door105.jpg", "door106.jpg", "door107.jpg", "door108.jpg", //15-23
		"window100.jpg", "window101.jpg", "window102.jpg", "window103.jpg", "window104.jpg", "window105.jpg", "window106.jpg", "window107.jpg", "window108.jpg", "window109.jpg", //24-33
		"roof100.jpg", "roof101.jpg", "roof102.jpg", "roof103.jpg", "roof104.jpg", "roof105.jpg", "roof106.jpg", "roof107.jpg", "roof108.jpg", "roof109.jpg", //34-43
		"ad1.jpg", "ad2.jpg", "ad3.jpg"}; //44-46
	soundPaths = {"b1.mp3", "b2.mp3", "b3.mp3", "b4.mp3", "b5.mp3", "b6.mp3", "b7.mp3", "b8.mp3", "b9.mp3", "b10.mp3", "b11.mp3", //0-4 for fancy buildings' audio snippets, 5-10 for poor buildings'
			"f1.mp3",
			"m1.mp3", "m2.mp3", "m3.mp3", "m4.mp3", "m5.mp3", "m6.mp3", "m7.mp3", "m8.mp3", //12-19
			"p1.mp3", "p2.mp3", "p3.mp3", "p4.mp3", //20-23
			"r1.mp3", "r2.mp3", "r3.mp3", "r4.mp3", "r5.mp3", "r6.mp3"}; //24-29
	for (auto it = texturePaths.begin(); it!=texturePaths.end(); ++it) {
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		//do stuff to avoid crashing when texture isn't a dimensions power of 2
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
		int width, height, nChannels;
		stbi_set_flip_vertically_on_load(true);
		unsigned char *data = stbi_load(("Assets/images/"+ *it).c_str(), &width, &height, &nChannels, 0);
		if (data) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		} else {
			std::cout << "invalid texture filepath" << *it << std::endl;
		}
		stbi_image_free(data);
		textures.push_back(texture);
	}
	texturePaths.clear(); //done with the path names, now we just work with textures vector

	//actually get reasonable random values after initiating srand
	srand(time(NULL));
	placePeople(-125, 125, 125, -125);
	int z = -53;
	while (z < 97) {
		int x = -97;
		while (x < 75) {
			//fill on street near road
			fillStreet(x, z, 44, 'x');
			fillStreet(x+38, z-7, 36, 'z', 'E');
			fillStreet(x, z-38, 36, 'x', 'N');
			fillStreet(x, z-7, 30, 'z', 'W');
			x+=50;
		}
		z+=50;
	}


	//fill just two more streets with buildings, the last two streets are for rich people
	int x = -97;
	fillStreet(x, z, 44, 'x');
	fillStreet(x+38, z-7, 36, 'z', 'E');
	fillStreet(x, z-38, 36, 'x', 'N');
	fillStreet(x, z-7, 30, 'z', 'W');
	x+=50;

	fillStreet(x, z, 44, 'x');
	fillStreet(x+38, z-7, 36, 'z', 'E');
	fillStreet(x, z-38, 36, 'x', 'N');
	fillStreet(x, z-7, 30, 'z', 'W');
	x+=50;

	//manually add in the rich people skyscrapers
	Skyscraper sky1 = Skyscraper(6, 25, 18, vec3(11 + x, 0.0, -16 + z), 3, 7, 11, 0);
	sky1.grow();
	m_rootNode->add_child(sky1.create());
	//pass sound clip and position of sound, +3 since we want sound to be at center of building, not min corner
	sky1.audio = SoundEngine->play3D(("Assets/sounds/"+ soundPaths[0]).c_str(), vec3df(11 + x + 3, 0, -16 + z - 3), true, false, true);
	sky1.audio->setMinDistance(0.5);

	Skyscraper sky2 = Skyscraper(4, 22, 15, vec3(32 + x, 0.0, -16 + z), 4, 8, 12, 90);
	sky2.grow();
        sky2.audio = SoundEngine->play3D(("Assets/sounds/"+ soundPaths[1]).c_str(), vec3df(32 + x + 2, 0, -16 + z - 2), true, false, true);
        sky2.audio->setMinDistance(0.5);
	m_rootNode->add_child(sky2.create());

	//put some billboards up
	GeometryNode *bb = new GeometryNode("cube", "bb1");
	bb->textureIndex = 44;
	bb->scale(vec3(10, 10, 0.5));
	bb->translate(vec3(x + 5, 5, z -43));
	m_rootNode->add_child(bb);

        bb = new GeometryNode("cube", "bb2");
        bb->textureIndex = 45;
        bb->scale(vec3(0.5, 12, 20));
        bb->translate(vec3(x + 43, 6, z -34));
        m_rootNode->add_child(bb);

	x+=50;

        bb = new GeometryNode("cube", "bb3");
        bb->textureIndex = 46;
        bb->scale(vec3(20, 15, 0.5));
        bb->translate(vec3(x + 22, 7.5, z - 0.25));
        m_rootNode->add_child(bb);

        Skyscraper sky3 = Skyscraper(8, 15, 10, vec3(11 + x, 0.0, -10 + z), 5, 9, 13, 180);
        sky3.grow();
        m_rootNode->add_child(sky3.create());
        sky3.audio = SoundEngine->play3D(("Assets/sounds/"+ soundPaths[2]).c_str(), vec3df(11 + x + 4, 0, -10 + z - 4), true, false, true);
        sky3.audio->setMinDistance(0.5);

        Skyscraper sky4 = Skyscraper(4, 22, 15, vec3(36 + x, 0.0, -26 + z), 6, 10, 14, 90);
        sky4.grow();
        sky4.audio = SoundEngine->play3D(("Assets/sounds/"+ soundPaths[3]).c_str(), vec3df(36 + x + 2, 0, -26 + z - 2), true, false, true);
        sky4.audio->setMinDistance(0.5);
        m_rootNode->add_child(sky4.create());
        Skyscraper sky5 = Skyscraper(4, 22, 15, vec3(36 + x, 0.0, -20 + z), 6, 10, 14, 90);
        sky5.grow();
        sky5.audio = SoundEngine->play3D(("Assets/sounds/"+ soundPaths[4]).c_str(), vec3df(36 + x + 2, 0, -20 + z - 2), true, false, true);
        sky5.audio->setMinDistance(0.5);
        m_rootNode->add_child(sky5.create());

	//manually place a few pairs of people to demo sound and their panic modes (move together)
	x = 0;
	z = 0;
	ISound *audio = SoundEngine->play3D(("Assets/sounds/"+ soundPaths[23]).c_str(), vec3df(x,0,z), true, false, true);
	audio->setMinDistance(0.5);
	ISound *panicAudio = SoundEngine->play3D(("Assets/sounds/"+ soundPaths[24]).c_str(), vec3df(x,0,z), true, false, true);
	panicAudio->setMinDistance(0.5);
	Person *p1 = new Person(x, z, 0, materials[7], materials[0], materials[5], materials[9], audio, panicAudio);
	m_rootNode->add_child(p1->node);
	Person *p2 = new Person(x + 1, z, 0, materials[7], materials[1], materials[7], materials[9]);
	m_rootNode->add_child(p2->node);
	Pair *pairing = new Pair{p1, p2};
	p1->pairing = pairing;
	p2->pairing = pairing;
	people.push_back(p1);
	people.push_back(p2);

	x = 52;
	z = 50;
	audio = SoundEngine->play3D(("Assets/sounds/"+ soundPaths[21]).c_str(), vec3df(x,0,z), true, false, true);
	audio->setMinDistance(0.5);
	panicAudio = SoundEngine->play3D(("Assets/sounds/"+ soundPaths[26]).c_str(), vec3df(x,0,z), true, false, true);
	panicAudio->setMinDistance(0.5);
	p1 = new Person(x, z, 180, materials[7], materials[0], materials[5], materials[9], audio, panicAudio);
	m_rootNode->add_child(p1->node);
	p2 = new Person(x + 1, z, 180, materials[7], materials[1], materials[7], materials[9]);
	m_rootNode->add_child(p2->node);
	pairing = new Pair{p1, p2};
	p1->pairing = pairing;
	p2->pairing = pairing;
	people.push_back(p1);
	people.push_back(p2);
}

//----------------------------------------------------------------------------------------
void Project::createShaderProgram()
{
	m_shader.generateProgramObject();
	m_shader.attachVertexShader( getAssetFilePath("VertexShader.vs").c_str() );
	m_shader.attachFragmentShader( getAssetFilePath("FragmentShader.fs").c_str() );
	m_shader.link();

}

//----------------------------------------------------------------------------------------
void Project::enableVertexShaderInputSlots()
{
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(m_vao_meshData);

		// Enable the vertex shader attribute location for "position" when rendering.
		m_positionAttribLocation = m_shader.getAttribLocation("position");
		glEnableVertexAttribArray(m_positionAttribLocation);

		// Enable the vertex shader attribute location for "normal" when rendering.
		m_normalAttribLocation = m_shader.getAttribLocation("normal");
		glEnableVertexAttribArray(m_normalAttribLocation);

                m_texAttribLocation = m_shader.getAttribLocation("tex");
                glEnableVertexAttribArray(m_texAttribLocation);
		CHECK_GL_ERRORS;
	}

	// Restore defaults
	glBindVertexArray(0);
}

//----------------------------------------------------------------------------------------
void Project::uploadVertexDataToVbos (
		const MeshConsolidator & meshConsolidator
) {
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &m_vbo_vertexPositions);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);

		glBufferData(GL_ARRAY_BUFFER, meshConsolidator.getNumVertexPositionBytes(),
				meshConsolidator.getVertexPositionDataPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	// Generate VBO to store all vertex normal data
	{
		glGenBuffers(1, &m_vbo_vertexNormals);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexNormals);

		glBufferData(GL_ARRAY_BUFFER, meshConsolidator.getNumVertexNormalBytes(),
				meshConsolidator.getVertexNormalDataPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

        // Generate VBO to store all vertex uv data
        {
                glGenBuffers(1, &m_vbo_vertexUVs);

                glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexUVs);
                glBufferData(GL_ARRAY_BUFFER, meshConsolidator.getNumVertexUVBytes(),
                                meshConsolidator.getVertexUVPtr(), GL_STATIC_DRAW);

                glBindBuffer(GL_ARRAY_BUFFER, 0);
                CHECK_GL_ERRORS;
        }
}

//----------------------------------------------------------------------------------------
void Project::mapVboDataToVertexShaderInputLocations()
{
	// Bind VAO in order to record the data mapping.
	glBindVertexArray(m_vao_meshData);

	// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
	// "position" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);
	glVertexAttribPointer(m_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Tell GL how to map data from the vertex buffer "m_vbo_vertexNormals" into the
	// "normal" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexNormals);
	glVertexAttribPointer(m_normalAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexUVs);
	glVertexAttribPointer(m_texAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
void Project::initPerspectiveMatrix()
{
	float aspect = ((float)m_windowWidth) / m_windowHeight;
	m_perpsective = glm::perspective(degreesToRadians(60.0f), aspect, 0.1f, 200.0f);
}


//----------------------------------------------------------------------------------------
void Project::initViewMatrix() {
	m_view = glm::lookAt(vec3(0.0f, 0.0f, 0.0f),
			vec3(0.0f, 0.0f, -1.0f),
			vec3(0.0f, 1.0f, 0.0f));
}

//----------------------------------------------------------------------------------------
void Project::initLightSources() {
	//m_light is attached to camera
	m_light.spotExponent = 20.0;
	m_light.pos = vec4(0.0, -1.0, 0.1, 1.0);
	m_light.cosCutOff = cos(radians(10.0f));
	m_light.rgbIntensity = vec3(1.0f); // light
	m_light.dir = vec4(0.0, 0.0, -1.0, 0);
}

void Project::initAudio() {
	background = SoundEngine->play2D("Assets/sounds/fan.mp3", true, false, true);
	background->setVolume(0.1);
}

//----------------------------------------------------------------------------------------
void Project::uploadCommonSceneUniforms() {
	m_shader.enable();
	{
		//-- Set Perpsective matrix uniform for the scene:
		GLint location = m_shader.getUniformLocation("Perspective");
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(m_perpsective));
		CHECK_GL_ERRORS;
			location = m_shader.getUniformLocation("light.dir");
			glUniform3fv(location, 1, value_ptr(m_light.dir));
			location = m_shader.getUniformLocation("light.cosCutOff");
			glUniform1f(location, m_light.cosCutOff);
			location = m_shader.getUniformLocation("light.position");
			glUniform3fv(location, 1, value_ptr(m_light.pos));
			location = m_shader.getUniformLocation("light.rgbIntensity");
			glUniform3fv(location, 1, value_ptr(m_light.rgbIntensity));
			CHECK_GL_ERRORS;

		//-- Set background light ambient intensity
			location = m_shader.getUniformLocation("ambientIntensity");
			vec3 ambientIntensity(0.1f);
			glUniform3fv(location, 1, value_ptr(ambientIntensity));
			CHECK_GL_ERRORS;
	}
	m_shader.disable();
}

bool intersectGround(const vec3 A, const vec3 B, vec3 &point, vec3 v1, vec3 v2, vec3 v3) {
        vec3 normal = cross(v1-v2, v3-v2);
        float d = dot(normal, v2);
        float t = (d-dot(normal, A)) / (dot(normal, B));
        if (t > 0) {
                point = A+t*B;
                return true;
        }
        return false;
}

/*
 * Called once per frame, before guiLogic().
 */
void Project::appLogic()
{
	if (lookMode) {
                if (wPressed) pitch+=0.05f;
                if (sPressed) pitch-=0.05f;
                if (aPressed) yaw-=0.05f;
                if (dPressed) yaw+=0.05f;
	} else {
		if (wPressed) velocity += 0.2f* m_dir;
		if (sPressed) velocity -= 0.2f* m_dir;
		if (aPressed) velocity -= 0.2f*normalize(cross(m_dir, camUp));
		if (dPressed) velocity += 0.2f*normalize(cross(m_dir, camUp));
		if (ePressed) velocity += 0.2f*camUp;
		if (qPressed) velocity -= 0.2f*camUp;
	}
	pitch = clamp(pitch, -1.57, 1.57);
	m_dir.x = sin(yaw) * cos(pitch);
	m_dir.y = sin(pitch);
	m_dir.z = -cos(pitch) * cos(yaw);
	camUp.x = sin(yaw) * cos(pitch+1.57);
	camUp.y = sin(pitch+1.57);
	camUp.z = -cos(pitch+1.57) * cos(yaw);
	m_dir = normalize(m_dir);
	camUp = normalize(camUp);
	velocity = clamp(velocity, -5.0f, 5.0f);
	camPos += velocity;
	if (!freeMode) camPos.y = clamp(camPos.y, 45.0f, 60.0f);
	camPos.x = clamp(camPos.x, -125.0f, 125.0f);
	camPos.z = clamp(camPos.z, -125.0f, 125.0f);

	velocity *= 0.95;
	//add some perturbations to helicopter
	velocity.z += (rand() % 3)*0.03 - 0.03f;
	velocity.x += (rand() % 3)*0.03 - 0.03f;
	velocity.y += (rand() % 3)*0.05 - 0.05f;


	m_view = glm::lookAt(camPos, camPos + m_dir, camUp);
	double posx, posy;
	glfwGetCursorPos(m_window, &posx, &posy);
	posx = ((posx/m_windowWidth) * 2.0 - 1.0); //convert to opengl coords
	posy = -((posy/m_windowHeight) * 2.0 - 1.0);
	m_light.dir = vec4(normalize(vec3(posx, posy, -1.0)), 0);

	light_dir_model = vec3(inverse(m_view) * m_light.dir);
	light_pos_model = vec3(inverse(m_view)*m_light.pos);
	if (!intersectGround(light_pos_model, light_dir_model, light_intersect, ground1, ground2, ground3)) {
		light_intersect = vec3(0, -10000, 0);
	}
	for (auto it = people.begin(); it!=people.end(); ++it) {
		float dx = (*it)->pos.x - light_intersect.x;
		float dz = (*it)->pos.z - light_intersect.z;
		float distance = sqrt(dx * dx + dz * dz);
		if (distance < 4) (*it)->trigger();
		(*it)->run(light_intersect);
	}

	//need position in model coordinates, since sound files were defined there
	SoundEngine->setListenerPosition(
		vec3df(light_intersect.x, light_intersect.y, light_intersect.z),
		vec3df(camUp.x, camUp.y, camUp.z));
	//relative frequency, velocity is us the observer
	float frequency = (soundSpeed + length(20.0f*velocity)) / soundSpeed;
	background->setPlaybackSpeed(frequency);
	uploadCommonSceneUniforms();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void Project::guiLogic()
{
	if( !show_gui ) {
		return;
	}

	static bool firstRun(true);
	if (firstRun) {
		ImGui::SetNextWindowPos(ImVec2(50, 50));
		firstRun = false;
	}

	static bool showDebugWindow(true);
	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);
	float opacity(0.5f);
	ImGui::Begin("Menu", &showDebugWindow, ImVec2(100,100), opacity, windowFlags);
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Info"))
		{
                        ImGui::Text( "key W: move forward");
                        ImGui::Text( "key S: move backward");
                        ImGui::Text( "key A: move left");
                        ImGui::Text( "key D: move right");
                        ImGui::Text( "key Q: descend");
                        ImGui::Text( "key E: ascend");
                        ImGui::Text( "hold Shift: Look mode - WASD keys become looking instead of moving");
			ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Toggles"))
		{
			if (ImGui::MenuItem("Toggle Textures")) {
                                textureMode = !textureMode;
                        }
			if (ImGui::MenuItem("Free Look Mode")) {
                                freeMode = !freeMode;
                        }
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	ImGui::End();
}

//----------------------------------------------------------------------------------------
// Update mesh specific shader uniforms:
void Project::updateShaderUniforms(
		const ShaderProgram & shader,
		const GeometryNode & node,
		const glm::mat4 & modelMatrix
) {
	shader.enable();
		//-- Set ModelView matrix:
		GLint location = shader.getUniformLocation("ModelView");
		mat4 modelView = mat4(m_view*modelMatrix);
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(modelView));
		CHECK_GL_ERRORS;
		//-- Set NormMatrix:
		location = shader.getUniformLocation("NormalMatrix");
		mat3 normalMatrix = glm::transpose(glm::inverse(mat3(modelView)));
		glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(normalMatrix));
		CHECK_GL_ERRORS;
		//-- Set Material values:
		location = shader.getUniformLocation("material.kd");
		vec4 kd = node.material.kd;

		//make people white translucent in infrared
		if (infraredMode && isPerson) {
			kd.x = 1.0;
			kd.y = 1.0;
			kd.z = 1.0;
		}
		glUniform4fv(location, 1, value_ptr(kd));
		CHECK_GL_ERRORS;
		location = shader.getUniformLocation("material.ks");
		vec3 ks = node.material.ks;
		glUniform3fv(location, 1, value_ptr(ks));
		CHECK_GL_ERRORS;
		location = shader.getUniformLocation("material.shininess");
		glUniform1f(location, node.material.shininess);
		CHECK_GL_ERRORS;
		location = shader.getUniformLocation("textured");
		glUniform1i(location, (node.textureIndex != -1 && textureMode) ? 1 : 0);
		location = shader.getUniformLocation("infrared");
		glUniform1i(location, infraredMode ? 1 : 0);
		if (node.textureIndex != -1) {
			unsigned int texture = textures[node.textureIndex];
			glBindTexture(GL_TEXTURE_2D, texture);
		}
	shader.disable();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void Project::draw() {

	glEnable( GL_DEPTH_TEST );
	glEnable(GL_CULL_FACE);
	renderSceneGraph(*m_rootNode);


	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE);
}

//----------------------------------------------------------------------------------------
void Project::renderSceneGraph(SceneNode & root) {
	glBindVertexArray(m_vao_meshData);
	m_model = mat4();
	traverse(root);
	glBindVertexArray(0);
	CHECK_GL_ERRORS;
}

void Project::traverse(SceneNode & curr) {
	matStack.push(m_model);
	m_model *= curr.get_transform();
	//person flag for infraredMode
	if (curr.m_name == "torso") isPerson = true;

	if (curr.m_nodeType == NodeType::GeometryNode) {
		GeometryNode & geometryNode = static_cast<GeometryNode &>(curr);
		updateShaderUniforms(m_shader, geometryNode, m_model);
                BatchInfo batchInfo = m_batchInfoMap[geometryNode.meshId];
                m_shader.enable();
                glDrawArrays(GL_TRIANGLES, batchInfo.startIndex, batchInfo.numIndices);
                m_shader.disable();
	}
	for (SceneNode * child : curr.children) {
		traverse(*child);
	}

	if (curr.m_name == "torso") isPerson = false;

	m_model = mat4(matStack.top());
	matStack.pop();
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void Project::cleanup()
{

}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool Project::cursorEnterWindowEvent (
		int entered
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse cursor movement events.
 */
bool Project::mouseMoveEvent (
		double xPos,
		double yPos
) {
	bool eventHandled(false);
	return true;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool Project::mouseButtonInputEvent (
		int button,
		int actions,
		int mods
) {
	bool eventHandled(false);

	// Fill in with event handling code...
	if (!ImGui::IsMouseHoveringAnyWindow()) {
		if (actions == GLFW_PRESS) {
		}
        }
	return true;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool Project::mouseScrollEvent (
		double xOffSet,
		double yOffSet
) {
	bool eventHandled(false);

	// Fill in with event handling code...
	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool Project::windowResizeEvent (
		int width,
		int height
) {
	bool eventHandled(false);
	initPerspectiveMatrix();
	return eventHandled;
}



/*
 * Event handler.  Handles key input events.
 */
bool Project::keyInputEvent (
		int key,
		int action,
		int mods
) {
	bool eventHandled(false);

	if( action == GLFW_PRESS ) {
		if( key == GLFW_KEY_M ) {
			show_gui = !show_gui;
			eventHandled = true;
		}
                if( key == GLFW_KEY_W ) {
			wPressed = true;
                        eventHandled = true;
                }
                if( key == GLFW_KEY_A ) {
			aPressed = true;
                        eventHandled = true;
                }
                if( key == GLFW_KEY_S ) {
                        sPressed = true;
			eventHandled = true;
                }
                if( key == GLFW_KEY_D ) {
			dPressed = true;
                        eventHandled = true;
                }
                if( key == GLFW_KEY_Q ) {
			qPressed = true;
                        eventHandled = true;
                }
		if (key == GLFW_KEY_E ) {
			ePressed = true;
			eventHandled = true;
		}
		if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
                        lookMode = true;
                        eventHandled = true;
                }
		if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) {
			infraredMode = true;
			eventHandled = true;
		}
        } else if (action == GLFW_RELEASE ) {
                if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) {
                        infraredMode = false;
                        eventHandled = true;
                }
                if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
                        lookMode = false;
                        eventHandled = true;
                }
		if (key == GLFW_KEY_W ) {
			wPressed = false;
			eventHandled = true;
		}
                if (key == GLFW_KEY_A ) {
                        aPressed = false;
                        eventHandled = true;
                }
                if (key == GLFW_KEY_S ) {
                        sPressed = false;
                        eventHandled = true;
                }
                if (key == GLFW_KEY_D ) {
                        dPressed = false;
                        eventHandled = true;
                }
                if( key == GLFW_KEY_Q ) {
			qPressed = false;
                        eventHandled = true;
                }
		if (key == GLFW_KEY_E ) {
			ePressed = false;
			eventHandled = true;
		}
        }
	// Fill in with event handling code...

	return eventHandled;
}
