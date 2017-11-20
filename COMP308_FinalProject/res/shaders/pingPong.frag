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

// Our input ping pong texture
uniform sampler2D texture0;
uniform vec3 skinIncrement;
uniform vec3 endSkinColor;

uniform sampler2D shadowMap;

// Materials and lights
uniform Material material;
uniform Light tanLight;

// World space vertex / normal
varying vec4 wVertex;
varying vec3 wNormal;
varying vec2 vTextureCoord0;
varying vec4 lightVertexPosition;

vec3 getColorFromPointLight(Light pointLight, vec3 worldspace_position, vec3 worldspace_normal);
vec3 getColorFromDirectionalLight(Light dirLight, vec3 worldspace_position, vec3 worldspace_normal);
float getTanningValue(Light light, vec3 worldspace_position, vec3 worldspace_normal);
vec3 getClampedColor(vec3 toClamp);
float calculateShadowValue(Light dirLight, vec3 worldspace_position, vec3 worldspace_normal);

void main() {
	// Get the color from the input texture from the previous frame
	vec3 textureColor = texture2D(texture0, vTextureCoord0, 1).rgb;

	float tanVal = getTanningValue(tanLight, wVertex.xyz, wNormal);
	vec3 textureWithIncrement = textureColor + (skinIncrement * tanVal);
	vec3 finalColor = getClampedColor(textureWithIncrement);

	float shadowValue = calculateShadowValue(tanLight, wVertex.xyz, wNormal);

	if(shadowValue > 0) { //in light
		gl_FragColor = vec4(finalColor, 1);
	}
	else {//in shadow
		gl_FragColor = vec4(textureColor, 1);
	}
	
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

float getTanningValue(Light light, vec3 worldspace_position, vec3 worldspace_normal){
	vec3 light_viewspace_position = light.position.xyz;
	vec3 light_direction = -normalize(light_viewspace_position);
	float s_dot_n = max(dot(-light_direction, worldspace_normal), 0.0);
	return s_dot_n;
}

vec3 getClampedColor(vec3 toClamp){
	vec3 newColor = vec3(max(toClamp.x, endSkinColor.x), max(toClamp.y, endSkinColor.y), max(toClamp.z, endSkinColor.z) );
	return newColor;
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