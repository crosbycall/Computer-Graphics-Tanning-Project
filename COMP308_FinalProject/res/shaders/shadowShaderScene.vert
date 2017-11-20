#version 120

uniform mat4 modelViewProjectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 lightModelViewProjectionMatrix;
uniform mat3 normalMatrix;
uniform mat4 lightMatrix;	//modelviewmatrix for the light
uniform vec3 inLightPosition;  //model-space

varying vec4 lightVertexPosition;
varying vec3 outColor;
varying vec3 position;
varying vec3 outNormal;
varying vec3 lightPosition;  //world-space

float mapRanges(float oldStart, float oldEnd, float newStart, float newEnd, float value) ;

void main()
{
	gl_Position = modelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0); //world space position

	position = vec3(modelViewMatrix * vec4(gl_Vertex.xyz, 1.0));
	outNormal = normalMatrix * gl_Normal;
	outColor= vec3(1.0, 1.0, 1.0);	
	lightPosition=vec3(lightMatrix*vec4(inLightPosition,1.0));
}

float mapRanges(float oldStart, float oldEnd, float newStart, float newEnd, float value) {
	float input_range = oldEnd - oldStart;
	float output_range = newEnd - newStart;

	return (value - oldStart) * output_range / input_range + newStart;
}
