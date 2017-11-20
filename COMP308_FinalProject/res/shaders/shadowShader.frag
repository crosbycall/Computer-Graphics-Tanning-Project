//#version 430
#version 120

uniform sampler2D shadowMap;
//uniform sampler2DShadow shadowMap;
uniform vec3 cameraPosition;

varying vec4 lightVertexPosition;
varying vec3 outNormal;
varying vec3 outColor;
varying vec3 lightPosition;
varying vec3 position;

//const vec3 ambientColor=vec3(0.5,0.5,0.5);
//const vec3 diffuseColor=vec3(0.3,0.3,0.3);
//const vec3 specularColor=vec3(0.5,0.5,0.5);
const vec3 ambientColor=vec3(0.05,0.05,0.05);
const vec3 diffuseColor=vec3(0.7,0.7,0.7);
const vec3 specularColor=vec3(1.0,1.0,1.0);

float calculateShadowValue();

void main()
{
	float shadowValue = 0.0;
	vec4 lightVertexPosition2 = lightVertexPosition;
	lightVertexPosition2 /= lightVertexPosition2.w;

	//for(float x=-0.001; x<=0.001; x+=0.0005)
	//	for(float y=-0.001;y<=0.001;y+=0.0005)
	//	{
	//		if(texture2D(shadowMap,lightVertexPosition2.xy+vec2(x,y)).z >= lightVertexPosition2.z)
	//			shadowValue += 1.0;	
	//	}
	//shadowValue /= 16.0;

	if(texture2D(shadowMap, lightVertexPosition2.xy).z >= lightVertexPosition2.z) {
		shadowValue = 1.0;
	}

	//shadowValue = calculateShadowValue();

	gl_FragColor = vec4(shadowValue, shadowValue, shadowValue, 1.0);

	//vec3 normal = normalize(outNormal);
	//vec3 surf2light = normalize(lightPosition - position);
	//vec3 surf2camera = normalize(-position);
	//vec3 reflection = -reflect(surf2camera, normal);
	//float diffuseContribution = max(0.0,dot(normal,surf2light));
	//float specularContribution = pow(max(0.0,dot(reflection,surf2light)),32.0);

	//gl_FragColor=vec4(ambientColor * outColor + (shadowValue + 0.05) * diffuseContribution * diffuseColor * outColor + (shadowValue < 0.5 ? vec3(0.0,0.0,0.0) : specularContribution * specularColor * shadowValue), 1.0);
}

//http://ogldev.atspace.co.uk/www/tutorial42/tutorial42.html
float calculateShadowValue() {
	float EPSILON = 0.005;
	vec4 lightVertexPosition2 = lightVertexPosition;
	lightVertexPosition2 /= lightVertexPosition2.w;
	vec2 UVCoords;
	UVCoords.x = lightVertexPosition2.x;
	UVCoords.y = lightVertexPosition2.y;
	float z = lightVertexPosition2.z;

	float offset = 1.0/1024;

	float shadowValue = 0.0;

	for(int y = -1; y <= 1; y++) {
		for(int x = -1; x <= 1; x++) {
			vec2 offSets = vec2(x * offset, y * offset);
			vec3 UVC = vec3(UVCoords + offSets, z + EPSILON);
			//shadowValue += texture2D(shadowMap, UVCoords + offSets, ).z;
			//shadowValue += texture(shadowMap, UVC);
		}
	}
	return (shadowValue / 18.0); // took out +0.5 at the start (bias has already been added)
}