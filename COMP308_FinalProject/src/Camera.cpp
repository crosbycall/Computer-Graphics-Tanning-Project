#pragma once

#include "Camera.hpp"

Camera::Camera() {

}

Camera::Camera(vec3 position, vec3 rotation) {
	this->position = position;
	this->pitch = rotation.x;
	this->yaw = rotation.y;
	this->roll = rotation.z;
}

/*
Loads our camera matrices into our shader
*/
void Camera::prepare(GLuint g_shader, int width, int height) {
	// Load the view matrix
	mat4 viewMatrix = getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(g_shader, "viewMatrix"), 1, false, viewMatrix.dataPointer());

	// Load the projection matrix
	mat4 projectionMatrix = getProjectionMatrix(width, height);
	glUniformMatrix4fv(glGetUniformLocation(g_shader, "projectionMatrix"), 1, false, projectionMatrix.dataPointer());
}

/*
Translates the camera by the given vector
*/
void Camera::moveCamera(vec3 translation) {
	position += translation;
}

/*
Returns a mat4 representing the view matrix for this camera
*/
mat4 Camera::getViewMatrix() {
	// Make a fresh matrix
	mat4 viewMatrix = mat4().identity();

	// Now translate by the inverse of the camera (move the world, not the camera)
	viewMatrix *= viewMatrix.translate(-position.x, -position.y, -position.z);

	// Apply rotations in xyz order to our matrix
	viewMatrix *= viewMatrix.rotateX(radians(pitch) );
	viewMatrix *= viewMatrix.rotateY(radians(yaw) );
	viewMatrix *= viewMatrix.rotateZ(radians(roll) );

	// Return our final viewMatrix
	return viewMatrix;
}

/*
Returns a mat4 representing the projection matrix for this camera
*/
mat4 Camera::getProjectionMatrix(int width, int height) {
	// Make a fresh matrix
	mat4 projectionMatrix = mat4().identity();

	// Form the projection matrix and return 
	projectionMatrix = projectionMatrix.perspectiveProjection(radians(g_fovy), (float)width / (float)height, g_znear, g_zfar);
	return projectionMatrix;
}

void Camera::debug() {
	cout << "VIEW MATRIX: " << endl;
	cout << getViewMatrix() << endl;
	cout << "PROJECTION MATRIX: " << endl;
	cout << getProjectionMatrix(640, 480) << endl;
}