#pragma once

#include <fstream>
#include "simple_image.hpp"
#include "cgra_geometry.hpp"
#include "cgra_math.hpp"
#include "opengl.hpp"

using namespace cgra;
using namespace std;

class Model {
	struct vertex {
		int p = 0; // index for point in m_points
		int t = 0; // index for uv in m_uvs
		int n = 0; // index for normal in m_normals
	};

	struct triangle {
		vertex v[3]; //requires 3 verticies
	};

	struct RigidBody {
		std::string filepath;
		std::vector<cgra::vec3> m_points;
		std::vector<cgra::vec2> m_uvs;
		std::vector<cgra::vec3> m_normals;
		std::vector<triangle> m_triangles;
	};

	struct Material {
		vec3 ambient;
		vec3 diffuse;
		vec3 specular;
		float shininess;
	};

public:
	// Fields
	RigidBody body;
	GLuint listID;
	GLuint textureID;
	Material material;

	// Original Transformations
	vec3 origin;
	vec3 rotation;
	vec3 scale;
	mat4 modelMatrix;
	float texRepeat;
	bool is2DTexture = false;
	bool isBumpMapTexture = false;

	// Constructors
	Model(string filename, vec3 origin, vec3 rotation, vec3 scale, int texReapet);

	// Methods
	void readOBJ(string filename);
	mat4 getMatrixOffStack();
	void createDisplayList(vec3 origin, vec3 rotation, vec3 scale, int texRepeat); // creates the initial transormation matrix in order of translate, rotate, scale
	void loadShaderComponents(GLuint shader);
	void setTextureID(GLuint texID, bool is2D, bool isBumpMap);
	void setMaterial(vec3 amb, vec3 diff, vec3 spec, float shine);
};