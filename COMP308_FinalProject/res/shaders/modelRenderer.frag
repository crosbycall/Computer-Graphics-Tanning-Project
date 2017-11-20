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

uniform Material material;
uniform Light tanLight;
uniform Light aLight;
uniform Light dLight;
uniform Light dLight2;

uniform sampler2D shadowMap;

// World space vertex / normal
varying vec4 wVertex;
varying vec3 wNormal;
varying vec2 vTextureCoord0;

varying vec4 lightVertexPosition;

vec3 getColorFromPointLight(Light pointLight, vec3 worldspace_position, vec3 worldspace_normal);
vec3 getColorFromDirectionalLight(Light dirLight, vec3 worldspace_position, vec3 worldspace_normal);
float calculateShadowValue(Light dirLight, vec3 worldspace_position, vec3 worldspace_normal);

void main() {
	float shadowValue = calculateShadowValue(tanLight, wVertex.xyz, wNormal);
	vec3 shadowColor = vec3(shadowValue, shadowValue, shadowValue); //either 1's or 0's

	// Ambient Light
	vec3 ambientLightColor = aLight.ambient.rgb * material.ambient.rgb;

	// Tan Light Color 
	vec3 tanLightColor = getColorFromDirectionalLight(tanLight, wVertex.xyz, wNormal);

	// Other Directional Light Colors
	vec3 dLightColor = getColorFromDirectionalLight(dLight, wVertex.xyz, wNormal);
	vec3 dLight2Color = getColorFromDirectionalLight(dLight2, wVertex.xyz, wNormal);
	vec3 directionalLightsColor = tanLightColor + dLightColor + dLight2Color;
	
	// Final Lighting Color
	vec3 finalLightingColor = (directionalLightsColor * shadowColor) + ambientLightColor;

	// Set gl_FragColor
	gl_FragColor = vec4(finalLightingColor, 1);
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

float calculateShadowValue(Light dirLight, vec3 worldspace_position, vec3 worldspace_normal) {
	float shadowValue = 0.0;
	vec4 lightVertexPosition2 = lightVertexPosition;
	lightVertexPosition2 /= lightVertexPosition2.w;
	lightVertexPosition2 += 0.01;

	vec3 light_viewspace_position = dirLight.position.xyz;
	vec3 light_direction = -normalize(light_viewspace_position);
	float dotP = max(dot(-light_direction, worldspace_normal), 0.0);

	if(dotP > 0) {
		for(float y = -0.0015; y <= 0.0015; y += 0.0005) {
			for(float x = -0.0015; x <= 0.0015; x += 0.0005) {
				if(texture2D(shadowMap, lightVertexPosition2.xy + vec2(x, y)).z >= lightVertexPosition2.z) {
					shadowValue += 1.0;
				}
			}
		}
	}
	shadowValue /= 64.0;

	return shadowValue;
}