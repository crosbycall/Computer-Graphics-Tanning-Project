//---------------------------------------------------------------------------
//
// Copyright (c) 2015 Taehyun Rhee, Joshua Scott, Ben Allen
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
varying vec3 baseColor;

// World space vertex / normal
varying vec4 wVertex;
varying vec3 wNormal;
varying vec2 vTextureCoord0;

vec3 getColorFromPointLight(Light pointLight, vec3 worldspace_position, vec3 worldspace_normal);
vec3 getColorFromDirectionalLight(Light dirLight, vec3 worldspace_position, vec3 worldspace_normal);

void main() {
	vec3 textureColor = texture2D(texture0, vTextureCoord0).rgb;
	
	// IMPORTANT tell OpenGL what the final color of the fragment is (vec4)
	gl_FragColor = vec4(baseColor, 1);
	//gl_FragColor = vec4(textureColor, 1);
}

vec3 getColorFromPointLight(Light pointLight, vec3 worldspace_position, vec3 worldspace_normal){
	// Positions and directions
	vec3 light_world_position = pointLight.position.xyz;
	vec3 light_direction = normalize(light_world_position - worldspace_position);
	
	// Diffuse
	float s_dot_n = max(dot(light_direction, worldspace_normal), 0.0);
	vec3 diffuse = pointLight.diffuse.rgb * material.diffuse.rgb * s_dot_n;
	
	return diffuse;
}

vec3 getColorFromDirectionalLight(Light dirLight, vec3 worldspace_position, vec3 worldspace_normal){
	// Positions and directions
	vec3 light_viewspace_position = dirLight.position.xyz;
	vec3 light_direction = -normalize(light_viewspace_position);
	
	// Diffuse
	float s_dot_n = max(dot(-light_direction, worldspace_normal), 0.0);
	vec3 diffuse = dirLight.diffuse.rgb * material.diffuse.rgb * s_dot_n;
	
	return diffuse;
}