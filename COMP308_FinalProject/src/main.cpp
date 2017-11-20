//---------------------------------------------------------------------------
//
// Copyright (c) 2016 Taehyun Rhee, Joshua Scott, Ben Allen
//
// This software is provided 'as-is' for assignment of COMP308 in ECS,
// Victoria University of Wellington, without any express or implied warranty. 
// In no event will the authors be held liable for any damages arising from
// the use of this software.
//
// The contents of this file may not be copied or duplicated in any form
// without the prior permission of its owner.
//
//----------------------------------------------------------------------------

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <stdexcept>

#include "cgra_geometry.hpp"
#include "cgra_math.hpp"
#include "simple_image.hpp"
#include "simple_shader.hpp"
#include "opengl.hpp"
#include "Camera.hpp"
#include "Model.hpp"
#include "skeleton.hpp"
#include "ModelSkinned.hpp"

using namespace std;
using namespace cgra;

// Structures
struct Light {
	vec3 ambient;
	vec3 position;
	vec3 diffuse;
	vec3 specular;

	vec3 spotDirection;
	float spotCutoff;
	float spotExponent;
};

// FrameBuffer texture dimensions
//
int bufferWidth = 1024 * 2;
int bufferHeight = 1024 * 2;

// Window
//
GLFWwindow* g_window;

// Camera
//
Camera camera;

// Entities
//
vector<Model> entities;
ModelSkinned cylinder;
Skeleton *boneRig = nullptr;

// Mouse controlled Camera values
//
bool g_leftMouseDown = false;
bool g_rightMouseDown = false;
vec2 g_mousePosition;
bool control = false;                         // is control held down

// Shader and texture GLuint's
//
GLuint g_texture = 0;
GLuint mappingShader = 0;                     // UV -> World Space mapping shader
GLuint modelRenderer = 0;                     // untextured model shader
GLuint texturedModelRenderer = 0;             // textured model shader
GLuint textureToQuad = 0;                     // render teture onto quad shader
GLuint pingPongShader = 0;                    // ping ponging shader
GLuint testShader = 0;                        // debugging shader
GLuint g_shadow_shader = 0;                   // renders shadows for the tanning model (in UV coords)
GLuint g_simple_shader = 0;                   // depth map rendering
GLuint g_shadow_scene_shader = 0;             // renders shadows for whole scene
GLuint skinModelRenderer = 0;                 // renderer for skinned model

// Framebuffer ID's
//
GLuint pingPongA;                             // Our PingPongA Framebuffer
GLuint pingPongATexture;                      // Our PingPongA texture for the color buffer
GLuint pingPongB;                             // Our PingPongB Framebuffer
GLuint pingPongBTexture;                      // Our PingPongB texture for the color buffer
GLuint mappingBuffer;                         // Our Framebuffer for mapping UV -> World Space
GLuint mappingTexture;                        // Our color buffer attachment texture for our mappingBuffer
bool inputIsA = true;                         // True if input for Ping Pong is A, false if input is B

// Shadow Mapping variables
// Hold id of the framebuffer for light POV rendering
vec3 cameraOldPos;
mat4 shadowMatrix;
GLuint FBO, shadowFBO, shadowSceneFBO; // fbo's
GLuint renderTexture, depthTexture, shadowMap, finalShadowTexture, shadowSceneMap;
int shadowMapWidth = 1024;
int shadowMapHeight = 1024;
vec3 lightPosition;

vec3 lightInvDir;                              // Normalized Light Direction
mat4 depthProjectionMatrix;                    // Projection Matrix from Light's POV (Orthographic)
mat4 depthViewMatrix;                          // Light's POV View Matrix
mat4 depthModelMatrix;                         // Model Matrix for Light's POV
mat4 depthMVP;                                 // Combined total matrix for Light's POV


// Start and End Skin colors
//
//vec3 startSkinColor = vec3(255, 203, 182);
//vec3 endSkinColor = vec3(132, 57, 17);
vec3 startSkinColor = vec3(255, 203, 181);
vec3 endSkinColor = vec3(192, 128, 96);
vec3 skinIncrement;

// Tanning process variables
//
float targetTanTime = 10.0f;                  // How long it should take a fragment to tan from start to end color with constant full fource lighting
float targetFPS = 60.0f;                      // Target FPS for the program

// Program state variables
//
bool paused = false;                          // Is our tanning process paused?
bool tannersDeleted = false;                  // Have our tanners been deleted?
bool rotateTaurus = false;                    // Should we incrementally rotate the taurus?
int rotCount = 0;

// Model Movement variables
// 
float xShift = 0.5f;                          // x pos change of monkey
float yShift = 0.5f;                          // y pos change of monkey
int COLORPICK_HEIGHT;                         // height of window for color picking

// Forward declarations of methods
//
float mapRanges(float oldStart, float oldEnd, float newStart, float newEnd, float value);
void uploadMatricesToShader(GLuint shader, Model model, int width, int height);
void updateShadowMatrix(Model model, mat4 projectionMatrix, mat4 viewMatrix);
void updateShadowMatrixSkinned(ModelSkinned model, mat4 projectionMatrix, mat4 viewMatrix);
void createPingPongBuffers();
void initPingPongBuffersColor();
void cleanUpFrameBuffers();
void texturePass(int width, int height);
void scenePass(int width, int height);
void testPass(int width, int height);
void quad(int loc);
void test();
GLuint createTexture(int width, int height, bool isDepthTexture, string textureName);
void initLight(GLuint g_shader);
void initShader();
void initCamera();
void initScene();
void calculateTanningProcesses();
void updateModel();
void getNearestBones(ModelSkinned* man, Skeleton* boneRig);

/*
 * Return the model_view matrix on the stack
 */
mat4 getMatrixOffStack() {
	float matrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
	vec4 one = vec4(matrix[0], matrix[1], matrix[2], matrix[3]);
	vec4 two = vec4(matrix[4], matrix[5], matrix[6], matrix[7]);
	vec4 three = vec4(matrix[8], matrix[9], matrix[10], matrix[11]);
	vec4 four = vec4(matrix[12], matrix[13], matrix[14], matrix[15]);
	mat4 _m = mat4(one, two, three, four);
	return _m;
}

// Mouse Button callback
// Called for mouse movement event on since the last glfwPollEvents
//
void cursorPosCallback(GLFWwindow* win, double xpos, double ypos) {
	// cout << "Mouse Movement Callback :: xpos=" << xpos << "ypos=" << ypos << endl;
	if (g_leftMouseDown) {
		camera.yaw -= g_mousePosition.x - xpos;
		camera.pitch -= g_mousePosition.y - ypos;
	}

	//If right mouse button is held down, rotate the currently selected joint by currently selected axis
	if (g_rightMouseDown) {
		boneRig->rotateAxis(g_mousePosition.x - xpos, g_mousePosition.y - ypos);
	}

	g_mousePosition = vec2(xpos, ypos);
}


// Mouse Button callback
// Called for mouse button event on since the last glfwPollEvents
//
void mouseButtonCallback(GLFWwindow *win, int button, int action, int mods) {
	// cout << "Mouse Button Callback :: button=" << button << "action=" << action << "mods=" << mods << endl;
	if (button == GLFW_MOUSE_BUTTON_LEFT)
		g_leftMouseDown = (action == GLFW_PRESS);

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
		g_rightMouseDown = (action == GLFW_PRESS);
	
	// User let go of right click
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == 0) {
		boneRig->prepRecursive();
		boneRig->calcPropTransforms();
		updateModel();
	}

	//Find which joint of the skeleton we're clicking on
	if (g_leftMouseDown && control) {
		unsigned char pixel[4];
		glReadPixels(g_mousePosition.x, COLORPICK_HEIGHT - g_mousePosition.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
		boneRig->selectJoint((int)pixel[0], (int)pixel[1], (int)pixel[2]);
	}


}


// Scroll callback
// Called for scroll event on since the last glfwPollEvents
//
void scrollCallback(GLFWwindow *win, double xoffset, double yoffset) {
	// cout << "Scroll Callback :: xoffset=" << xoffset << "yoffset=" << yoffset << endl;
	camera.position -= vec3(0, 0, yoffset * 5);
}


// Keyboard callback
// Called for every key event on since the last glfwPollEvents
//
void keyCallback(GLFWwindow *win, int key, int scancode, int action, int mods) {
	//cout << "Key Callback :: key=" << key << "scancode=" << scancode
	// 	<< "action=" << action << "mods=" << mods << endl;

	// pausing
	if (key == 80 && action == 1) {
		paused = !paused;
		cout << "Tanning Paused = " << paused << endl;
	}
	
	// Removing Tanner Model from Scene
	if (key == 77 && action == 1) {
		rotateTaurus = !rotateTaurus;
	}

	// Moving Tanner Model
	//
	if (key == 262 && action == 1) {          // right arrow 
		Model *monkey = &entities[2];
		monkey->modelMatrix *= monkey->modelMatrix.translate(xShift, 0.0f, 0.0f);
	}
	else if (key == 263 && action == 1) {     // left arrow
		Model *monkey = &entities[2];
		monkey->modelMatrix *= monkey->modelMatrix.translate(-xShift, 0.0f, 0.0f);
	}
	else if (key == 264 && action == 1) {     // down arrow
		Model *monkey = &entities[2];
		monkey->modelMatrix *= monkey->modelMatrix.translate(0.0f, -yShift, 0.0f);
	}
	else if (key == 265 && action == 1) {     // up arrow
		Model *monkey = &entities[2];
		monkey->modelMatrix *= monkey->modelMatrix.translate(0.0f, yShift, 0.0f);
	}

	// Modifying tanning processes
	//
	if (key == 45 && action == 1) {          // - key
		targetTanTime -= 5.0f;
		if (targetTanTime < 5.0f) targetTanTime = 5.0f;
		calculateTanningProcesses();
		cout << "Tan Time is: " << targetTanTime << endl;
	}
	else if (key == 61 && action == 1) {     // = key
		targetTanTime += 5.0f;
		calculateTanningProcesses();
		cout << "Tan Time is: " << targetTanTime << endl;
	}
	else if (key == 82 && action == 1) {     // r key
		initPingPongBuffersColor();
		cout << "Reset Ping Pong Buffers" << endl;
	}

	// x key
	if (key == 88 && action == 1) {
		boneRig->updateAxis();
	}

	//Used to check if control key is held down for selecting joints
	if (key == 341 && action == 1) {
		control = true;
	}
	if (key == 341 && action == 0) {
		control = false;
	}
}


// Character callback
// Called for every character input event on since the last glfwPollEvents
//
void charCallback(GLFWwindow *win, unsigned int c) {
	// cout << "Char Callback :: c=" << char(c) << endl;
	// Not needed for this assignment, but useful to have later on
}

/*
Rendering scene from light positions pass
*/
void shadowLightPass() {
	// Save the old camera's position, and move to the Light's position
	cameraOldPos = camera.position;
	camera.position = lightPosition;

	// Bind our light's POV buffer and set appropriate flags
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glEnable(GL_DEPTH_TEST); // must be enabled for shadow mapping

	// Render the scene to the FrameBuffer
	for (Model model : entities) {
		// Calculate matrices to view from Light Position, for the current model
		lightInvDir = normalize(lightPosition);
		depthProjectionMatrix = depthProjectionMatrix.orthographicProjection(-10, 10, -10, 10, -10, 20);
		depthViewMatrix = depthViewMatrix.lookAt(lightInvDir.x, lightInvDir.y, lightInvDir.z, 0, 0, 0, 0, 1, 0);
		depthModelMatrix = model.modelMatrix;
		depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

		// Upload calculated matrices to shader and render the current model
		glUseProgram(g_simple_shader);
		glUniformMatrix4fv(glGetUniformLocation(g_simple_shader, "modelViewProjectionMatrix"), 1, false, depthMVP.dataPointer());
		glCallList(model.listID);
	}

	// Render the cylinder
	glUseProgram(g_simple_shader);
	mat4 modelMatrix = mat4().identity();
	mat4 viewMatrix = depthViewMatrix;
	mat4 projection = depthProjectionMatrix;
	mat4 totalMatrix = projection * viewMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(g_simple_shader, "modelViewProjectionMatrix"), 1, false, totalMatrix.dataPointer());
	glCallList(cylinder.listID);


	// Clean up
	glUseProgram(0);
	glDisable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, 640, 480);
	camera.position = cameraOldPos;
}

/*
Makes the ping ponging pass for the tanning model's texture
*/
void texturePass(int width, int height){
	// Use our ping ponging shader
	glUseProgram(pingPongShader);

	// Upload skin increment / end color values to shader
	glUniform3f(glGetUniformLocation(pingPongShader, "endSkinColor"), endSkinColor.x / 255, endSkinColor.y / 255, endSkinColor.z / 255);
	glUniform3f(glGetUniformLocation(pingPongShader, "skinIncrement"), skinIncrement.x, skinIncrement.y, skinIncrement.z);

	// Send light to shader
	initLight(pingPongShader);

	// Set flags
	glEnable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE3);

	// Bind Shadow Map
	glBindTexture(GL_TEXTURE_2D, shadowMap);

	// Get the model we want to tan
	Model model = entities[1];

	// Upload various matrices to shader
	uploadMatricesToShader(modelRenderer, model, width, height);
	updateShadowMatrix(model, depthProjectionMatrix, depthViewMatrix);
	glUniformMatrix4fv(glGetUniformLocation(pingPongShader, "lightModelViewProjectionMatrix"), 1, false, shadowMatrix.dataPointer());
	glUniform1i(glGetUniformLocation(pingPongShader, "shadowMap"), 3); //current bound texture

	// Set texture flag
	glActiveTexture(GL_TEXTURE0);

	// Do the actual ping ponging
	if (inputIsA) {
		// Input is pingPongA, so upload it's texture to the shader
		glBindTexture(GL_TEXTURE_2D, pingPongATexture);
		glUniform1i(glGetUniformLocation(pingPongShader, "texture0"), 0);

		// Bind our output pingPongBuffer and render the man's texture
		glBindFramebuffer(GL_FRAMEBUFFER, pingPongB);
		uploadMatricesToShader(pingPongShader, entities[1], bufferWidth, bufferHeight);
		entities[1].loadShaderComponents(pingPongShader);
		glCallList(entities[1].listID);
	}

	else if (!inputIsA) {
		// Input is pingPongB, so upload it's texture to the shader
		glBindTexture(GL_TEXTURE_2D, pingPongBTexture);
		glUniform1i(glGetUniformLocation(pingPongShader, "texture0"), 0);

		// Bind our output pingPongBuffer and render the man's texture
		glBindFramebuffer(GL_FRAMEBUFFER, pingPongA);
		uploadMatricesToShader(pingPongShader, entities[1], bufferWidth, bufferHeight);
		entities[1].loadShaderComponents(pingPongShader);
		glCallList(entities[1].listID);
	}

	// Clean up
	glUseProgram(0);
}

/*
Makes the pass which renders the scene to the screen
*/
void scenePass(int width, int height) {
	// Use our regular model shader for drawing models
	glUseProgram(texturedModelRenderer);

	// Set texture flag
	glActiveTexture(GL_TEXTURE0);

	// Bind appropriate texture based on pingPong output
	if (inputIsA) {
		// Input is pingPongA, so we just textured pingPongB in the previous pass
		// Bind pingPongB and upload it to our shader as a texture
		glBindTexture(GL_TEXTURE_2D, pingPongBTexture);
		glUniform1i(glGetUniformLocation(texturedModelRenderer, "texture0"), 0);
	}
	else if (!inputIsA) {
		// Input is pingPongB, so we just textured pingPongA in the previous pass
		// Bind pingPongA and upload it to our shader as a texture
		glBindTexture(GL_TEXTURE_2D, pingPongATexture);
		glUniform1i(glGetUniformLocation(texturedModelRenderer, "texture0"), 0);
	}

	//// Set flags
	glEnable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE3);

	// Bind the shadow map
	glBindTexture(GL_TEXTURE_2D, shadowMap);

	// Get the model being tanned and upload its specific matrices to shader
	Model model = entities[1];
	uploadMatricesToShader(texturedModelRenderer, model, width, height);
	updateShadowMatrix(model, depthProjectionMatrix, depthViewMatrix);
	glUniformMatrix4fv(glGetUniformLocation(texturedModelRenderer, "lightModelViewProjectionMatrix"), 1, false, shadowMatrix.dataPointer());
	glUniform1i(glGetUniformLocation(texturedModelRenderer, "shadowMap"), 3); //current bound texture

	// Upload light to shader
	initLight(texturedModelRenderer);
	
	// Bind our default buffer so we can render the scene
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	// Clear the screen frame buffer then render the model we want to tan
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	model.loadShaderComponents(texturedModelRenderer);
	glCallList(model.listID);

	// Render the table
	glUseProgram(modelRenderer);

	// Load lights to our shader and render table
	initLight(modelRenderer);

	// Set flags
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glActiveTexture(GL_TEXTURE3); //activate texture layer
	
	// Bind Shadow Map
	glBindTexture(GL_TEXTURE_2D, shadowMap);

	// Render the models
	for (Model model : entities) {
		// Load matrices to shader
		if (model.body.filepath != "COMP308_FinalProject/res/assets/torus.obj") {
			uploadMatricesToShader(modelRenderer, model, width, height);
			updateShadowMatrix(model, depthProjectionMatrix, depthViewMatrix);
			glUniformMatrix4fv(glGetUniformLocation(modelRenderer, "lightModelViewProjectionMatrix"), 1, false, shadowMatrix.dataPointer());
			glUniform1i(glGetUniformLocation(modelRenderer, "shadowMap"), 3); //current bound texture

			// Render current model
			model.loadShaderComponents(modelRenderer);
			glCallList(model.listID);
		}
	}

	// Render cylinder
	glUseProgram(modelRenderer);
	mat4 modelMatrix = mat4().identity();
	mat4 viewMatrix = camera.getViewMatrix();
	mat4 projection = camera.getProjectionMatrix(width, height);
	mat4 totalMatrix = projection * viewMatrix * modelMatrix;
	updateShadowMatrixSkinned(cylinder, depthProjectionMatrix, depthViewMatrix);
	glUniformMatrix4fv(glGetUniformLocation(modelRenderer, "modelViewProjectionMatrix"), 1, false, totalMatrix.dataPointer());
	glUniformMatrix4fv(glGetUniformLocation(modelRenderer, "lightModelViewProjectionMatrix"), 1, false, shadowMatrix.dataPointer());
	glUniform1i(glGetUniformLocation(modelRenderer, "shadowMap"), 3); //current bound texture
	glCallList(cylinder.listID);

	// Clean up
	glUseProgram(0);
}

/*
Makes an (optional) pass of our rendering process, will render two quads onto the screen, each with either
of our ping pong buffers textures applied to them.  This is used for debugging / visualisation purposes
*/
void testPass(int width, int height) {
	// Use our textureQuad special shader
	glUseProgram(textureToQuad);

	// Bind our pingPongA Buffer and it's texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pingPongATexture);
	glUniform1i(glGetUniformLocation(textureToQuad, "texture0"), 0);

	// Render the left quad
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	quad(0);

	// Bind our pingPongB Buffer and it's texture
	glBindTexture(GL_TEXTURE_2D, pingPongBTexture);
	glUniform1i(glGetUniformLocation(textureToQuad, "texture0"), 0);

	// Render the quad
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	quad(1);

	glUseProgram(0);
}

/*
Ugly method for rendering a quad to the screen. The int given will tell where to render the quad on the current buffer.
0 = bottom left, 1 = bottom right, 2 = full screen
*/
void quad(int loc) {
	if (loc == 0) {
		glBegin(GL_POLYGON);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-0.75, -0.25, 0.5);  // top left

		glTexCoord2f(1.0, 1.0);
		glVertex3f(-0.25, -0.25, 0.5);   // top right

		glTexCoord2f(1.0, 0.0);
		glVertex3f(-0.25, -0.75, 0.5);  // bottom right

		glTexCoord2f(0.0, 0.0);
		glVertex3f(-0.75, -0.75, 0.5); // bottom left

		glEnd();
	}

	else if (loc == 1) {
		glBegin(GL_POLYGON);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(0.25, -0.25, 0.5);  // top left

		glTexCoord2f(1.0, 1.0);
		glVertex3f(0.75, -0.25, 0.5);   // top right

		glTexCoord2f(1.0, 0.0);
		glVertex3f(0.75, -0.75, 0.5);  // bottom right

		glTexCoord2f(0.0, 0.0);
		glVertex3f(0.25, -0.75, 0.5); // bottom left

		glEnd();
	}

	else if (loc == 2) {
		glBegin(GL_POLYGON);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1, 1, 0.5);  // top left

		glTexCoord2f(1.0, 1.0);
		glVertex3f(1, 1, 0.5);   // top right

		glTexCoord2f(1.0, 0.0);
		glVertex3f(1, -1, 0.5);  // bottom right

		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1, -1, 0.5); // bottom left

		glEnd();
	}
}

/*
Helper method for mapping a value from one range to another.
For example, mapRanges(0, 100, 0, 1, 50) should return 0.5
*/
float mapRanges(float oldStart, float oldEnd, float newStart, float newEnd, float value) {
	float input_range = oldEnd - oldStart;
	float output_range = newEnd - newStart;

	return (value - oldStart) * output_range / input_range + newStart;
}

/*
Ugly helper method for checking if our UV mapping idea was ok
*/
void test() {
	glViewport(0, 0, bufferWidth, bufferHeight);
	// Use our textureQuad special shader
	glUseProgram(testShader);
	// Bind our pingPongA Buffer and it's texture
	glBindTexture(GL_TEXTURE_2D, g_texture);
	glUniform1i(glGetUniformLocation(testShader, "texture0"), 0);
	glBindFramebuffer(GL_FRAMEBUFFER, pingPongA);
	quad(0);

	glUseProgram(textureToQuad);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, pingPongATexture);
	glUniform1i(glGetUniformLocation(textureToQuad, "texture0"), 0);
	glViewport(0, 0, 640, 480);
	quad(1);

	glUseProgram(0); //done using testShader
}


// Draw function
//
void render(int width, int height) {
	// Update height for color picking
	COLORPICK_HEIGHT = height;
  
	// Enable flags
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);

	// Render scene from light POV pass
	glViewport(0, 0, shadowMapWidth, shadowMapHeight);
	shadowLightPass();
	
	// Make our ping ponging pass if we are not paused
	if (!paused) {
		glViewport(0, 0, bufferWidth, bufferHeight);
		texturePass(width, height);
	}

	// Make our regular scene rendering pass
	glViewport(0, 0, width, height);
	scenePass(width, height);
	
	// Make an optional testPass to show the textures being ping ponged
	glViewport(0, 0, width, height);
	testPass(width, height);

	//Switch our input buffer if we are not paused
	if (!paused) {
		inputIsA = !inputIsA;
	}

	// Are we rotating the taurus?
	if (rotateTaurus) {
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(1, 0, 1, 0);
		mat4 rot = getMatrixOffStack();
		entities[1].modelMatrix *= rot;

		rotCount++;

		if (rotCount == 180) {
			rotateTaurus = false;
			rotCount = 0;
		}
	}
	
	// Unbind our shader
	glUseProgram(0);

	// Disable flags for cleanup
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_NORMALIZE);

	// Render bone rig
	boneRig->prepare(camera, width, height);
	boneRig->renderSkeleton();
}

/*
Updates the shadow matrix
*/
void updateShadowMatrix(Model model, mat4 projectionMatrix, mat4 viewMatrix) {
	mat4 modelMatrix = model.modelMatrix;

	//bias matrix
	vec4 row1 = vec4(0.5, 0.0, 0.0, 0.0);
	vec4 row2 = vec4(0.0, 0.5, 0.0, 0.0);
	vec4 row3 = vec4(0.0, 0.0, 0.5, 0.0);
	vec4 row4 = vec4(0.5, 0.5, 0.5, 1.0);
	mat4 biasMatrix = mat4(row1, row2, row3, row4);

	shadowMatrix = biasMatrix * projectionMatrix * viewMatrix * modelMatrix;
}

/*
Updates the shadow matrix
*/
void updateShadowMatrixSkinned(ModelSkinned model, mat4 projectionMatrix, mat4 viewMatrix) {
	mat4 modelMatrix = mat4().identity();

	//bias matrix
	vec4 row1 = vec4(0.5, 0.0, 0.0, 0.0);
	vec4 row2 = vec4(0.0, 0.5, 0.0, 0.0);
	vec4 row3 = vec4(0.0, 0.0, 0.5, 0.0);
	vec4 row4 = vec4(0.5, 0.5, 0.5, 1.0);
	mat4 biasMatrix = mat4(row1, row2, row3, row4);

	shadowMatrix = biasMatrix * projectionMatrix * viewMatrix * modelMatrix;
}

/*
Uploads the transformation / view / projection matrices to the given shader, for the given model
*/
void uploadMatricesToShader(GLuint shader, Model model, int width, int height) {
	// Get table specific data
	mat4 modelMatrix = model.modelMatrix;
	mat4 viewMatrix = camera.getViewMatrix();
	mat4 projectionMatrix = camera.getProjectionMatrix(width, height);
	mat4 totalMatrix = projectionMatrix * viewMatrix * modelMatrix;

	// calculate normal matrix
	vec3 columnOne = vec3(modelMatrix[0][0], modelMatrix[0][1], modelMatrix[0][2]);
	vec3 columnTwo = vec3(modelMatrix[1][0], modelMatrix[1][1], modelMatrix[1][2]);
	vec3 columnThree = vec3(modelMatrix[2][0], modelMatrix[2][1], modelMatrix[2][2]);
	mat3 normalMatrix = mat3(columnOne, columnTwo, columnThree);

	// Upload vertex/normal matrices to our shader
	glUniformMatrix4fv(glGetUniformLocation(shader, "modelMatrix"), 1, false, modelMatrix.dataPointer());
	glUniformMatrix4fv(glGetUniformLocation(shader, "viewMatrix"), 1, false, viewMatrix.dataPointer());
	glUniformMatrix4fv(glGetUniformLocation(shader, "projectionMatrix"), 1, false, projectionMatrix.dataPointer());
	glUniformMatrix4fv(glGetUniformLocation(shader, "modelViewProjectionMatrix"), 1, false, totalMatrix.dataPointer());
	glUniformMatrix3fv(glGetUniformLocation(shader, "normalMatrix"), 1, false, normalMatrix.dataPointer());
}

// Sets up where and what the light is
// Called once on start up
// 
void initLight(GLuint g_shader) {
	// cout << "Init Light to Shader: " << g_shader << endl;

	// Ambient Light
	Light aLight = Light();
	aLight.position = vec3(0, 0, 0);
	aLight.ambient = vec3(0.3, 0.3, 0.3);
	aLight.diffuse = vec3(0, 0, 0);
	aLight.specular = vec3(0, 0, 0);
	glUniform3f(glGetUniformLocation(g_shader, "aLight.ambient"), aLight.ambient.x, aLight.ambient.y, aLight.ambient.z);
	glUniform3f(glGetUniformLocation(g_shader, "aLight.diffuse"), aLight.diffuse.x, aLight.diffuse.y, aLight.diffuse.z);
	glUniform3f(glGetUniformLocation(g_shader, "aLight.specular"), aLight.specular.x, aLight.specular.y, aLight.specular.z);
	glUniform3f(glGetUniformLocation(g_shader, "aLight.position"), aLight.position.x, aLight.position.y, aLight.position.z);

	// Tanning Light (Tans the model, also acts as a normal directional light in lighting calculations)
	Light tanLight = Light();
	tanLight.position = vec3(0, 35, 70);
	lightPosition = tanLight.position;
	tanLight.ambient = vec3(0, 0, 0);
	tanLight.diffuse = vec3(0.3, 0.3, 0.3);
	tanLight.specular = vec3(0, 0, 0);
	glUniform3f(glGetUniformLocation(g_shader, "tanLight.ambient"), tanLight.ambient.x, tanLight.ambient.y, tanLight.ambient.z);
	glUniform3f(glGetUniformLocation(g_shader, "tanLight.diffuse"), tanLight.diffuse.x, tanLight.diffuse.y, tanLight.diffuse.z);
	glUniform3f(glGetUniformLocation(g_shader, "tanLight.specular"), tanLight.specular.x, tanLight.specular.y, tanLight.specular.z);
	glUniform3f(glGetUniformLocation(g_shader, "tanLight.position"), tanLight.position.x, tanLight.position.y, tanLight.position.z);
}

// An example of how to load a texure from a hardcoded location
//
void initTexture() {
	Image tex("./COMP308_FinalProject/res/textures/skin.jpg");

	glActiveTexture(GL_TEXTURE0); // Use slot 0, need to use GL_TEXTURE1 ... etc if using more than one texture PER OBJECT
	glGenTextures(1, &g_texture); // Generate texture ID
	glBindTexture(GL_TEXTURE_2D, g_texture); // Bind it as a 2D texture

											 // Setup sampling strategies
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Finnaly, actually fill the data into our texture
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, tex.w, tex.h, tex.glFormat(), GL_UNSIGNED_BYTE, tex.dataPointer());
}

// An example of how to load a shader from a hardcoded location
//
void initShader() {
	// To create a shader program we use a helper function
	// We pass it an array of the types of shaders we want to compile
	// and the corrosponding locations for the files of each stage
	
	// Mapper shader for UV -> Vertex World Space
	mappingShader = makeShaderProgramFromFile({ GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, { "./COMP308_FinalProject/res/shaders/mapper.vert", "./COMP308_FinalProject/res/shaders/mapper.frag" });

	// Regular shader for rendering models with texture
	texturedModelRenderer = makeShaderProgramFromFile({ GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, { "./COMP308_FinalProject/res/shaders/texturedModelRenderer.vert", "./COMP308_FinalProject/res/shaders/texturedModelRenderer.frag" });

	// Regular shader for rendering models without texture
	modelRenderer = makeShaderProgramFromFile({ GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, { "./COMP308_FinalProject/res/shaders/modelRenderer.vert", "./COMP308_FinalProject/res/shaders/modelRenderer.frag" });

	// Shader for testing frame buffer textures by rendering them onto a quad
	textureToQuad = makeShaderProgramFromFile({ GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, { "./COMP308_FinalProject/res/shaders/textureOnQuad.vert", "./COMP308_FinalProject/res/shaders/textureOnQuad.frag" });

	// The ping ponging shader
	pingPongShader = makeShaderProgramFromFile({ GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, { "./COMP308_FinalProject/res/shaders/pingPong.vert", "./COMP308_FinalProject/res/shaders/pingPong.frag" });

	// Shader for testing stuff
	testShader = makeShaderProgramFromFile({ GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, { "./COMP308_FinalProject/res/shaders/test.vert", "./COMP308_FinalProject/res/shaders/test.frag" });

	// Renders the scene with shadows
	g_shadow_shader = makeShaderProgramFromFile({ GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, { "./COMP308_FinalProject/res/shaders/shadowShader.vert", "./COMP308_FinalProject/res/shaders/shadowShader.frag" });

	// Depth map rendering
	g_simple_shader = makeShaderProgramFromFile({ GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, { "./COMP308_FinalProject/res/shaders/simpleShader.vert", "./COMP308_FinalProject/res/shaders/simpleShader.frag" });

	// renderer for a skinned model
	skinModelRenderer = makeShaderProgramFromFile({ GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, { "./COMP308_FinalProject/res/shaders/skinnedModelRenderer.vert", "./COMP308_FinalProject/res/shaders/skinnedModelRenderer.frag" });
}


// Sets up where the camera is in the scene
// 
void initCamera() {
	// Create our camera
	camera = Camera(vec3(0, 0, 50), vec3(0, 0, 0));
}

// Inits the object in our scene
//
void initScene() {
	// Inititialize geometry
	Model table = Model("COMP308_FinalProject/res/assets/table.obj", vec3(0, 0, 0.5), vec3(0, 0, 0), vec3(1, 1, 1), 2);
	Model torus = Model("COMP308_FinalProject/res/assets/torus.obj", vec3(0, 3, 0), vec3(0, 180, 0), vec3(4, 4, 4), 1);
	Model monkey = Model("COMP308_FinalProject/res/assets/monkey.obj", vec3(0, 5, 10), vec3(0, 0, 0), vec3(1, 1, 1), 1);
	cylinder = ModelSkinned("COMP308_FinalProject/res/assets/cylinderuv.obj", vec3(0, 2.2, 0), vec3(0, 0, 0), vec3(1, 1, 1), 1);


	// Set object materials
	table.setMaterial(vec3(0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec3(0.1f, 0.1f, 0.1f), 0.1 * 128);
	monkey.setMaterial(vec3(0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec3(0.1f, 0.1f, 0.1f), 0.1 * 128);
	torus.setMaterial(vec3(0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec3(0.1f, 0.1f, 0.1f), 0.1 * 128);

	// Store all of our RigidBodies
	entities.push_back(table);
	entities.push_back(torus);
	entities.push_back(monkey);

	// Load priman bone rig
	boneRig = new Skeleton("COMP308_FinalProject/res/assets/test.asf", vec3(0, 0, 0), vec3(0, 0, 0), vec3(2.8, 2.8, 2.8) );

	// Initialise the joint positions for the skeleton
	boneRig->prepRecursive();

	//Get the nearest 2 joints for every vertex in the human model
	getNearestBones(&cylinder, boneRig);
	cylinder.bonesCalcd = true;
}

/*
Creates a texture and returns the texture id
isDepthTexture - used for shadow map
textureName - only used for debugging
*/
GLuint createTexture(int width, int height, bool isDepthTexture, string textureName) {
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, (!isDepthTexture ? GL_RGBA8 : GL_DEPTH_COMPONENT), width, height, 0, isDepthTexture ? GL_DEPTH_COMPONENT : GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return textureId;
}

/*
Creates FBO's and textures for shadow mapping
*/
void initShadowTextures() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	shadowMap = createTexture(shadowMapWidth, shadowMapHeight, true, "shadowMap");
	finalShadowTexture = createTexture(1024, 1024, true, "finalShadowTexture");
	shadowSceneMap = createTexture(shadowMapWidth, shadowMapHeight, true, "shadowSceneMap");

	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0); //use shadow map
	// cout << "shadowMap FBO id: " << FBO << endl;

	glGenFramebuffers(1, &shadowSceneFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowSceneFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowSceneMap, 0); //use shadow map
	// cout << "shadowSceneMap FBO id: " << FBO << endl;

	int i = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (i != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Framebuffer is not OK, status = " << i << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*
Creates our two FrameBuffers that we will use to ping pong our tanning texture
*/
void createPingPongBuffers() {
	// Start by initialising pingPongA
	glGenFramebuffers(1, &pingPongA);
	glBindFramebuffer(GL_FRAMEBUFFER, pingPongA);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glGenTextures(1, &pingPongATexture);
	glBindTexture(GL_TEXTURE_2D, pingPongATexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingPongATexture, 0);

	// cout << "PingPongATexture is: " << pingPongATexture << endl;

	// Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		abort();

	// Then initialise pingPongB
	glGenFramebuffers(1, &pingPongB);
	glBindFramebuffer(GL_FRAMEBUFFER, pingPongB);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glGenTextures(1, &pingPongBTexture);
	glBindTexture(GL_TEXTURE_2D, pingPongBTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingPongBTexture, 0);

	// cout << "PingPongBTexture is: " << pingPongBTexture << endl;

	// Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		abort();

	// Init the colors
	initPingPongBuffersColor();

	// Unbind the current frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*
Initialises the entire color attachments for pingPongA and pingPongB
to be the startSkinColor.
*/
void initPingPongBuffersColor() {
	// Set our clear color to our beginning skin color
	glClearColor((float)startSkinColor.x / 255.0f, (float)startSkinColor.y / 255.0f, (float)startSkinColor.z / 255.0f, 1.0f);

	// Clear each ping pong buffer using the starting color
	glBindFramebuffer(GL_FRAMEBUFFER, pingPongA);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindFramebuffer(GL_FRAMEBUFFER, pingPongB);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Retrieve the color of a pixel in our buffer and double check it has been initialized correctly
	unsigned char pixel[4];
	glReadPixels(1, 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
	vec3 color = vec3(pixel[0], pixel[1], pixel[2]);

	if (color != startSkinColor) {
		cout << "Problem initialising colors" << endl;
	}

	// Set clear color back to black
	glClearColor(0, 0, 0, 1);
}

/*
Deletes the FrameBuffers we made 
*/
void cleanUpFrameBuffers() {
	glDeleteFramebuffers(1, &pingPongA);
	glDeleteTextures(1, &pingPongATexture);

	glDeleteFramebuffers(1, &pingPongB);
	glDeleteTextures(1, &pingPongBTexture);

	glDeleteFramebuffers(1, &mappingBuffer);
	glDeleteTextures(1, &mappingTexture);

	glDeleteFramebuffers(1, &FBO);
	glDeleteTextures(1, &renderTexture);
	glDeleteTextures(1, &depthTexture);
	glDeleteTextures(1, &shadowMap);
}

/*
Calculate various timings and increments to do with our taning process
*/
void calculateTanningProcesses() {
	// First, we need to calculate the texture increment in our ping pong shader
	// The target increment is the color change for one frame if the fragment is being hit by the light at full force

	// Formula for our perFrame increment then becomes ( (endSkinColor - startSkinColor) / tanTime) / targetFPS
	// Also we need to make sure our deltaSkinColor is normalized, as our start and end colors are defined in 0 - 255 space
	vec3 deltaSkinColor = (endSkinColor / 255.0f) - (startSkinColor / 255.0f);
	vec3 increment = (deltaSkinColor / targetTanTime) / targetFPS;

	skinIncrement = increment;
}

/**
 * Called after user lets go of right click when rotating the skinned model
 * Updates the mesh
 */
void updateModel() {
	vector<int> seen; // Used to check that we're not moving the same vert more than once since the vertex structs can point to the same m_point pos

	bone* selectedBone;
	for (int i = 0; i < boneRig->m_bones.size(); i++) {
		if (boneRig->m_bones[i].selected) {
			//cout << "LIL" << endl;
			selectedBone = &boneRig->m_bones[i];
		}
	}
	// For every triangle
	for (int tri = 0; tri < cylinder.body.m_triangles.size(); tri++) {
		// For every vertex in the triangle
		for (int v = 0; v < 3; v++) {
			// Get the current vertex we're working on
			//  if (std::find(seen.begin(), seen.end(), entities[0].body.m_triangles[tri].v[v].p) == seen.end()) {
			seen.push_back(cylinder.body.m_triangles[tri].v[v].p); // The (probably terrible) way to check if we're going over the same vert
			cylinder.body.m_triangles[tri].v[v].checked = true; // Now we've seen it

			//Make copies of the current vertex to adjust
			vec4 og = vec4(cylinder.body.m_points[cylinder.body.m_triangles[tri].v[v].p], 1);
			
			// Get the current bones that affect this vertex
			bone* b1 = cylinder.body.m_triangles[tri].v[v].closeJoints[0];
			bone* b2 = cylinder.body.m_triangles[tri].v[v].closeJoints[1];
			bone* b3 = cylinder.body.m_triangles[tri].v[v].closeJoints[2];

			// Create matrix with transforms of bone 1
			mat4 matrix1 = mat4().identity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glTranslatef(b1->jointPosition.x, b1->jointPosition.y, b1->jointPosition.z);
			glRotatef(b1->propTransform.x, 1, 0, 0);
			glRotatef(b1->propTransform.y, 0, 1, 0);
			glRotatef(b1->propTransform.z, 0, 0, 1);
			matrix1 = getMatrixOffStack();

			// Create matrix with transforms of bone 2
			mat4 matrix2 = mat4().identity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glTranslatef(b2->jointPosition.x, b2->jointPosition.y, b2->jointPosition.z);
			glRotatef(b2->propTransform.x, 1, 0, 0);
			glRotatef(b2->propTransform.y, 0, 1, 0);
			glRotatef(b2->propTransform.z, 0, 0, 1);
			matrix2 = getMatrixOffStack();
			 
			// Create matrix with transforms of bone 3
			mat4 matrix3 = mat4().identity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glTranslatef(b3->jointPosition.x, b3->jointPosition.y, b3->jointPosition.z);
			glRotatef(b3->propTransform.x, 1, 0, 0);
			glRotatef(b3->propTransform.y, 0, 1, 0);
			glRotatef(b3->propTransform.z, 0, 0, 1);
			matrix3 = getMatrixOffStack();
			
			// Perform matrix averaging
			matrix1 *= cylinder.body.m_triangles[tri].v[v].weights[0];
			matrix2 *= cylinder.body.m_triangles[tri].v[v].weights[1];
			matrix3 *= cylinder.body.m_triangles[tri].v[v].weights[2];
			mat4 finalmat = matrix1 + matrix2 + matrix3;

			// Set vertex's transform
			cylinder.body.m_triangles[tri].v[v].trans = finalmat;
		}
	}
	cylinder.createDisplayList(cylinder.origin, cylinder.rotation, cylinder.scale, 1);
	for (int tri = 0; tri < cylinder.body.m_triangles.size(); tri++) {
		// For every vertex in the triangle
		for (int v = 0; v < 3; v++) {
			cylinder.body.m_triangles[tri].v[v].checked = false;//Reset our seen check
		}
	}
}

/*
 * For every vertex, calculate the nearest bone and weights for the vertex
 */ 
void getNearestBones(ModelSkinned* man, Skeleton* boneRig) {
	// For every triangle
	for (int tri = 0; tri < man->body.m_triangles.size(); tri++) {
		// For every vertex in the triangle
		for (int v = 0; v < 3; v++) {
			bone *bones[3];
			float nearest[3] = { 99999, 99999, 999999 };
			// For every bone in man
			vec4 worldSpace = man->modelMatrix * vec4(man->body.m_points[man->body.m_triangles[tri].v[v].p], 1);
			for (int i = 1; i < boneRig->m_bones.size(); i++) {
				bone* b = &boneRig->m_bones[i];
				// Get the distance between current bone and current vertex
				float distance = sqrt(pow((b->jointPosition.x - worldSpace.x), 2.0f) +
					pow((b->jointPosition.y - worldSpace.y), 2.0f) +
					pow((b->jointPosition.z - worldSpace.z), 2.0f));
				// Set the current smallest distances if needed
				if (distance < nearest[0] | distance < nearest[1] | distance < nearest[2]) {
					if (distance < nearest[0]) {
						nearest[2] = nearest[1];
						nearest[1] = nearest[0];
						nearest[0] = distance;
						bones[2] = bones[1];
						bones[1] = bones[0];
						bones[0] = b;
					}
					else if (distance < nearest[1]) {
						nearest[2] = nearest[1];
						nearest[1] = distance;
						bones[2] = bones[1];
						bones[1] = b;
					}
					else if (distance < nearest[2]) {
						nearest[2] = distance;
						bones[2] = b;
					}
				}
			}
			
			// Normalize and set as weights
			float weight1 = 1 - (nearest[0] / (nearest[0] + nearest[1] + nearest[2]));
			float weight2 = 1 - (nearest[1] / (nearest[0] + nearest[1] + nearest[2]));
			float weight3 = 1 - (nearest[2] / (nearest[0] + nearest[1] + nearest[2]));
			float finWeight1 = weight1 / (weight1 + weight2 + weight3);
			float finWeight2 = weight2 / (weight1 + weight2 + weight3);
			float finWeight3 = weight3 / (weight1 + weight2 + weight3);
			
			man->body.m_triangles[tri].v[v].weights[0] = 1.0f;
			man->body.m_triangles[tri].v[v].weights[1] = 0.0f;
			man->body.m_triangles[tri].v[v].weights[2] = 0.0f;

			// Set the bones for vertex
			man->body.m_triangles[tri].v[v].closeJoints[0] = bones[0];
			man->body.m_triangles[tri].v[v].closeJoints[1] = bones[1];
			man->body.m_triangles[tri].v[v].closeJoints[2] = bones[2];

		}
	}
}

// Forward decleration for cleanliness (Ignore)
void APIENTRY debugCallbackARB(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, GLvoid*);


//Main program
// 
int main(int argc, char **argv) {

	// Initialize the GLFW library
	if (!glfwInit()) {
		cerr << "Error: Could not initialize GLFW" << endl;
		abort(); // Unrecoverable error
	}

	// Get the version for GLFW for later
	int glfwMajor, glfwMinor, glfwRevision;
	glfwGetVersion(&glfwMajor, &glfwMinor, &glfwRevision);

	// Create a windowed mode window and its OpenGL context
	g_window = glfwCreateWindow(640, 480, "COMP308 Final Project", nullptr, nullptr);
	if (!g_window) {
		cerr << "Error: Could not create GLFW window" << endl;
		abort(); // Unrecoverable error
	}

	// Make the g_window's context is current.
	// If we have multiple windows we will need to switch contexts
	glfwMakeContextCurrent(g_window);



	// Initialize GLEW
	// must be done after making a GL context current (glfwMakeContextCurrent in this case)
	glewExperimental = GL_TRUE; // required for full GLEW functionality for OpenGL 3.0+
	GLenum err = glewInit();
	if (GLEW_OK != err) { // Problem: glewInit failed, something is seriously wrong.
		cerr << "Error: " << glewGetErrorString(err) << endl;
		abort(); // Unrecoverable error
	}



	// Print out our OpenGL verisions
	cout << "Using OpenGL " << glGetString(GL_VERSION) << endl;
	cout << "Using GLEW " << glewGetString(GLEW_VERSION) << endl;
	cout << "Using GLFW " << glfwMajor << "." << glfwMinor << "." << glfwRevision << endl;



	// Attach input callbacks to g_window
	glfwSetCursorPosCallback(g_window, cursorPosCallback);
	glfwSetMouseButtonCallback(g_window, mouseButtonCallback);
	glfwSetScrollCallback(g_window, scrollCallback);
	glfwSetKeyCallback(g_window, keyCallback);
	glfwSetCharCallback(g_window, charCallback);



	// Enable GL_ARB_debug_output if available. Not nessesary, just helpful
	if (glfwExtensionSupported("GL_ARB_debug_output")) {
		// This allows the error location to be determined from a stacktrace
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		// Set the up callback
		glDebugMessageCallbackARB(debugCallbackARB, nullptr);
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
		cout << "GL_ARB_debug_output callback installed" << endl;
	}
	else {
		cout << "GL_ARB_debug_output not available. No worries." << endl;
	}


	// Initialize Geometry/Material/Lights
	initShader();
	initCamera();
	initScene();
	initLight(-1);
	initTexture();
	initShadowTextures();
	createPingPongBuffers();
	calculateTanningProcesses();

	// Start our timer
	int frameCount = 0;
	float timeElapsed = 0.0f;
	clock_t startTime = clock();

	// Loop until the user closes the window
	while (!glfwWindowShouldClose(g_window)) {

		// Make sure we draw to the WHOLE window
		int width, height;
		glfwGetFramebufferSize(g_window, &width, &height);

		// Main Render
		render(width, height);

		// Swap front and back buffers
		glfwSwapBuffers(g_window);

		// Poll for and process events
		glfwPollEvents();

		// Calculate timings and fps
		frameCount++;
		clock_t endTime = clock();
		clock_t clockTicksTaken = endTime - startTime;
		timeElapsed = clockTicksTaken / (double)CLOCKS_PER_SEC;

		//// Print FPS if we need to
		//if (timeElapsed >= 1.0) {
		//	cout << "FPS: " << frameCount << endl;
		//	frameCount = 0;
		//	startTime = clock();
		//}
	}

	cleanUpFrameBuffers();
	glfwTerminate();
}






//-------------------------------------------------------------
// Fancy debug stuff
//-------------------------------------------------------------

// function to translate source to string
string getStringForSource(GLenum source) {

	switch (source) {
	case GL_DEBUG_SOURCE_API:
		return("API");
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		return("Window System");
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		return("Shader Compiler");
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		return("Third Party");
	case GL_DEBUG_SOURCE_APPLICATION:
		return("Application");
	case GL_DEBUG_SOURCE_OTHER:
		return("Other");
	default:
		return("n/a");
	}
}

// function to translate severity to string
string getStringForSeverity(GLenum severity) {

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		return("HIGH!");
	case GL_DEBUG_SEVERITY_MEDIUM:
		return("Medium");
	case GL_DEBUG_SEVERITY_LOW:
		return("Low");
	default:
		return("n/a");
	}
}

// function to translate type to string
string getStringForType(GLenum type) {
	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		return("Error");
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		return("Deprecated Behaviour");
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		return("Undefined Behaviour");
	case GL_DEBUG_TYPE_PORTABILITY:
		return("Portability Issue");
	case GL_DEBUG_TYPE_PERFORMANCE:
		return("Performance Issue");
	case GL_DEBUG_TYPE_OTHER:
		return("Other");
	default:
		return("n/a");
	}
}

// actually define the function
void APIENTRY debugCallbackARB(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei, const GLchar* message, GLvoid*) {
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) return;

	cerr << endl; // extra space

	cerr << "Type: " <<
		getStringForType(type) << "; Source: " <<
		getStringForSource(source) << "; ID: " << id << "; Severity: " <<
		getStringForSeverity(severity) << endl;

	cerr << message << endl;

	if (type == GL_DEBUG_TYPE_ERROR_ARB) throw runtime_error("");
}