//

#pragma once

#include "framework/Window.hpp"
#include "framework/OpenGLImport.hpp"
#include "framework/ShaderProgram.hpp"
#include "framework/MeshConsolidator.hpp"

#include "SceneNode.hpp"
#include "GeometryNode.hpp"
#include "Building.hpp"
#include "Person.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <stack>
#include <vector>
#include <irrKlang.h>
using namespace irrklang;

#pragma comment(lib, "irrKlang.lib")

#define  SENS_PANX     100.0
#define  SENS_PANY     100.0
#define  SENS_ZOOM     35.0

struct LightSource {
	glm::vec3 position;
	glm::vec3 rgbIntensity;
};

struct SpotLight {
	glm::vec3 rgbIntensity;
	glm::vec4 pos;
	float cosCutOff;
	float spotExponent;
	glm::vec4 dir;
};


class Project : public Window {
public:
	Project(const std::string & luaSceneFile);
	virtual ~Project();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	//-- Virtual callback methods
	virtual bool cursorEnterWindowEvent(int entered) override;
	virtual bool mouseMoveEvent(double xPos, double yPos) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool windowResizeEvent(int width, int height) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;

	//-- One time initialization methods:
	void processLuaSceneFile(const std::string & filename);
	void createShaderProgram();
	void enableVertexShaderInputSlots();
	void uploadVertexDataToVbos(const MeshConsolidator & meshConsolidator);
	void mapVboDataToVertexShaderInputLocations();
	void initViewMatrix();
	void initLightSources();
	void updateShaderUniforms(const ShaderProgram & shader, const GeometryNode & node, const glm::mat4 & viewMatrix);
	void initModels();
	void initAudio();
	void initPerspectiveMatrix();
	void uploadCommonSceneUniforms();
	void renderSceneGraph(SceneNode &node);
	void traverse(SceneNode &curr);
	void fillStreet(float startx, const float startz, int leftoverSpace, const char axis = 'x', const char facing = 'S');
	void placePeople(int startx, int endx, int startz, int endz);

	glm::mat4 m_perpsective;
	glm::mat4 m_model;
	glm::mat4 m_view;

	SpotLight m_light;

	//-- GL resources for mesh geometry data:
	GLuint m_vao_meshData;
	GLuint m_vbo_vertexPositions;
	GLuint m_vbo_vertexNormals;
	GLuint m_vbo_vertexUVs;
	GLint m_positionAttribLocation;
	GLint m_normalAttribLocation;
	GLint m_texAttribLocation;
	ShaderProgram m_shader;

	// BatchInfoMap is an associative container that maps a unique MeshId to a BatchInfo
	// object. Each BatchInfo object contains an index offset and the number of indices
	// required to render the mesh with identifier MeshId.
	BatchInfoMap m_batchInfoMap;

	std::string m_luaSceneFile;

	std::shared_ptr<SceneNode> m_rootNode;

	std::stack<glm::mat4> matStack;
	bool isPerson, infraredMode, lookMode, freeMode, textureMode, wPressed, aPressed, sPressed, dPressed, ePressed, qPressed;
	double yaw, pitch;
	glm::vec3 velocity, camUp, camPos, m_dir, light_intersect, ground1, ground2, ground3, light_dir_model, light_pos_model;
	std::vector<unsigned int> textures;
	std::vector<Person *> people;
	std::vector<Material> materials;
	std::vector<std::string> soundPaths;
	ISound *background;
};
