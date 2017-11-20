#version 120

// World space vertex / normal
varying vec4 wVertex;
varying vec3 wNormal;
varying vec2 vTextureCoord0;

void main() {
	// Set the color as XYZ
	gl_FragColor = vec4(wVertex.x, wVertex.y, wVertex.z, 1);
}