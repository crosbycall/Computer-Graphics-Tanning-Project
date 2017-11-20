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

float mapRanges(float oldStart, float oldEnd, float newStart, float newEnd, float value) ;

void main() {
	// Get our UV coordinate for this vertex
	vTextureCoord0 = gl_MultiTexCoord0.xy;

	float uMapped = mapRanges(0, 1, -1, 1, vTextureCoord0.x);
	float vMapped = mapRanges(0, 1, -1, 1, vTextureCoord0.y);

	// The mapped coordinates now represent where in the ColorBuffer we want to render
	// As long as z remains constant thats ok
	//gl_Position = vec4(uMapped, vMapped, 0.5, 1);
	gl_Position = vec4(gl_Vertex.xyz, 1);

	// Pass on world space vertex / normal to the fragment shader
	wVertex = modelMatrix * vec4(gl_Vertex.xyz, 1);
	wNormal = normalMatrix * gl_Normal.xyz;
}

/*
Maps an input from one value range, to another value range.
For example,  mapRanges(0, 100, 0, 1, 50) should return 0.5
*/
float mapRanges(float oldStart, float oldEnd, float newStart, float newEnd, float value) {
	float input_range = oldEnd - oldStart;
	float output_range = newEnd - newStart;

	return (value - oldStart) * output_range / input_range + newStart;
}