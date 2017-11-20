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

// Constant across both shaders
uniform sampler2D texture0;
varying vec3 baseColor;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat3 normalMatrix;

uniform bool priman;

// Values to pass to the fragment shader
varying vec2 vTextureCoord0;

// World space vertex / normal
varying vec4 wVertex;
varying vec3 wNormal;

void main() {
	if(!priman){
		// IMPORTANT tell OpenGL where the vertex is
		//gl_Position = modelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1);
		gl_Position = projectionMatrix * viewMatrix * vec4(gl_Vertex.xyz, 1);
	
		baseColor = gl_Color.rgb;
		vTextureCoord0 = gl_MultiTexCoord0.xy;
		wVertex = modelMatrix * vec4(gl_Vertex.xyz, 1);
		wNormal = normalMatrix * gl_Normal.xyz;
	}
	else{
		// IMPORTANT tell OpenGL where the vertex is
		gl_Position = projectionMatrix * gl_ModelViewMatrix * vec4(gl_Vertex.xyz, 1);
	
		vTextureCoord0 = gl_MultiTexCoord0.xy;
		wVertex = modelMatrix * vec4(gl_Vertex.xyz, 1);
		wNormal = normalMatrix * gl_Normal.xyz;
	}
}