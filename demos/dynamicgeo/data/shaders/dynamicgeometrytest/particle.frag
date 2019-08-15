#version 150 core

in vec4  pass_Color;
in vec3  pass_Uvs;
in float pass_PID;

out vec4 out_Color;

uniform sampler2D texture_input[2];

vec4 getTextureColor(int lookup)
{
	return texture(texture_input[lookup], uvs);
}

void main(void)
{
	// Get uv coordinates
	vec2 uvs = vec2(pass_Uvs.x, pass_Uvs.y);
	
	// Check which image to get
	// This is a hack to ensure this demo works on linux with the mesa opengl drivers
	// Otherwise: OpenGL ERROR: sampler arrays indexed with non-constant expressions are forbidden in GLSL 1.30 and later
	int tex_id = int(pass_PID+0.1) % 2;
	vec4 tex_color = getTextureColor(tex_id);
	if(tex_id == 0)
		tex_color = texture(texture_input[0], uvs);
	else
		tex_color = texture(texture_input[1], uvs);


	// Boost colors a bit
	float r = pow(tex_color.r,0.9);
	float g = pow(tex_color.g,0.9);
	float b = pow(tex_color.b,0.9);
	tex_color = vec4(r,g,b,tex_color.a);
	
	// Set output color
	out_Color = tex_color * pass_Color;
}