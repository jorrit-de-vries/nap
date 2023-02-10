#version 450 core

// Extensions
#extension GL_GOOGLE_include_directive : enable

// Includes
#include "shadow.glslinc"
#include "blinnphong.glslinc"
#include "utils.glslinc"

// Specialization Constants
layout (constant_id = 0) const uint SHADOW_SAMPLE_COUNT = 8;

// Uniforms
uniform nap
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 modelMatrix;
	mat4 normalMatrix;
	vec3 cameraPosition;
} mvp;

uniform light
{
	DirectionalLight lights[8];
	uint count;
} lit;

uniform pointlight
{
	PointLight lights[8];
	uint count;
} point;

uniform UBO
{
	vec3	ambient;				//< Ambient
	vec3	diffuse;				//< Diffuse
	vec3	specular;				//< Specular
	vec2	fresnel;				//< Fresnel [scale, power]
	float	shininess;				//< Shininess
	float	alpha;					//< Alpha
} ubo;

// Fragment Input
in vec3 	passPosition;			//< Fragment position in world space
in vec3 	passNormal;				//< Fragment normal in world space
in vec3 	passUV0;				//< Texture UVs
in float 	passFresnel;			//< Fresnel term
in vec4 	passShadowCoords[8];	//< Shadow Coordinates

// Fragment Output
out vec4 out_Color;

// Shadow Texture Sampler
uniform sampler2DShadow shadowMaps[8];

// Constants
const float SHADOW_STRENGTH = 0.85;


void main()
{
	BlinnPhongMaterial mtl = { ubo.ambient, ubo.diffuse, ubo.specular, ubo.shininess };
	
	// Lights
	vec3 dir_color = computeLightDirectional(lit.lights, lit.count, mtl, mvp.cameraPosition, normalize(passNormal), passPosition);

	vec3 point_color = computeLightPoint(point.lights, point.count, mtl, mvp.cameraPosition, normalize(passNormal), passPosition);
	
	vec3 color = (dir_color + point_color) * 0.5;
	color = mix(color, vec3(1.0), passFresnel * pow(luminance(color), 0.25));

	// Shadows
	float shadow = computeShadow(shadowMaps, passShadowCoords, lit.count, min(SHADOW_SAMPLE_COUNT, 64)) * SHADOW_STRENGTH;
	color *= (1.0 - shadow);

	out_Color = vec4(color, ubo.alpha);
}
