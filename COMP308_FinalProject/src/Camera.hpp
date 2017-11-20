#pragma once

#include "cgra_geometry.hpp"
#include "cgra_math.hpp"
#include "opengl.hpp"


using namespace cgra;
using namespace std;

class Camera {

	private:
	
	public:
		// Fields
		vec3 position; // where the camera is located
		float pitch;   // camera rotation on x axis
		float yaw;     // camera rotation on y axis
		float roll;    // camera rotation on z axis
		float zoom;

		// Projection values
		float g_fovy = 20.0;
		float g_znear = 0.1;
		float g_zfar = 1000.0;

		// Constructors
		Camera();
		Camera(vec3 position, vec3 rotation);

		// Methods
		void prepare(GLuint g_shader, int width, int height);
		void moveCamera(vec3 translation);
		mat4 getViewMatrix();
		mat4 getProjectionMatrix(int width, int height);
		void debug();
};