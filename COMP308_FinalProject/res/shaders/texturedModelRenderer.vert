#version 120

// Constant across both shaders
uniform sampler2D texture0;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat3 normalMatrix;
uniform mat4 lightModelViewProjectionMatrix;

// Values to pass to the fragment shader
varying vec2 vTextureCoord0;
varying vec4 lightVertexPosition;

// World space vertex / normal
varying vec4 wVertex;
varying vec3 wNormal;


void main() {
	// IMPORTANT tell OpenGL where the vertex is
	gl_Position = modelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1);

	lightVertexPosition = lightModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0); //vertex is same position as in shadowmap
	
	vTextureCoord0 = gl_MultiTexCoord0.xy;
	wVertex = modelMatrix * vec4(gl_Vertex.xyz, 1);
	wNormal = normalMatrix * gl_Normal.xyz;
}