#pragma once

#include <fstream>
#include "simple_image.hpp"
#include "cgra_geometry.hpp"
#include "cgra_math.hpp"
#include "opengl.hpp"
#include "skeleton.hpp"

using namespace cgra;
using namespace std;

class ModelSkinned {
	struct vertex {
		int p = 0; // index for point in m_points
		int t = 0; // index for uv in m_uvs
		int n = 0; // index for normal in m_normals
		bool checked = false;
		float weights[3];
		bone* closeJoints[3];
		mat4 trans = mat4().identity();
		//vec3 diff = vec3(0, 0, 0);
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

	bool bonesCalcd=false;

	// Constructors
	ModelSkinned();
	ModelSkinned(string filename, vec3 origin, vec3 rotation, vec3 scale, int texReapet);

	// Methods
	//void applyPose(Pose);
	void readOBJ(string filename);
	void createDisplayList(vec3 origin, vec3 rotation, vec3 scale, int texRepeat); // creates the initial transormation matrix in order of translate, rotate, scale
	void loadShaderComponents(GLuint shader);
	void prepare(GLuint g_shader);
	void setTextureID(GLuint texID, bool is2D, bool isBumpMap);
	void setMaterial(vec3 amb, vec3 diff, vec3 spec, float shine);
};