#version 120

struct Light{
	vec3 ambient;
	vec3 position;
	vec3 diffuse;
	vec3 specular;

	vec3 spotDirection;
	float spotCutoff;
	float spotExponent;
};

struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

// Constant across both shaders
uniform sampler2D texture0;
uniform float time;

uniform Material material;
uniform Light pointLight;
uniform Light directionalLight;

// World space vertex / normal
varying vec4 wVertex;
varying vec3 wNormal;
varying vec2 vTextureCoord0;
void main() {
	vec3 textureColor = texture2D(texture0, vTextureCoord0).rgb;
	//vec3 color = vec3(0, 1, 0);

	// Set the color to our sample from texture0
	gl_FragColor = vec4(textureColor, 1);
}