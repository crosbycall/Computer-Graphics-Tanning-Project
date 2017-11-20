#version 120

// Matrices for transformations
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat3 normalMatrix;

// Values to pass to the fragment shader
varying vec2 vTextureCoord0;

// World space vertex / normal
varying vec4 wVertex;
varying vec3 wNormal;

void main() {
	// We are rendering a quad using default opengl coordinate system
	// So just pass the vertex on as is
	gl_Position = vec4(gl_Vertex.xyz, 1);

	// Pass on world space vertex / normal to the fragment shader
	wVertex = modelMatrix * vec4(gl_Vertex.xyz, 1);
	wNormal = normalMatrix * gl_Normal.xyz;

	// Get our UV coordinate for this vertex
	vTextureCoord0 = gl_MultiTexCoord0.xy;
}