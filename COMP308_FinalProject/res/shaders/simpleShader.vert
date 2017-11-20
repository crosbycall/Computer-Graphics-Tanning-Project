#version 120

uniform mat4 modelViewProjectionMatrix;

varying float depth;

void main()
{
	gl_Position = modelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);
	depth = gl_Position.z;
}
