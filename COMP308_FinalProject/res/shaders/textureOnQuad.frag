#version 120

// Constant across both shaders
uniform sampler2D texture0;
uniform float time;

// World space vertex / normal
varying vec4 wVertex;
varying vec3 wNormal;
varying vec2 vTextureCoord0;
void main() {
	vec3 textureColor = texture2D(texture0, vTextureCoord0).rgb;
	
	// Set the color to our sample from texture0
	gl_FragColor = vec4(textureColor, 1);
}