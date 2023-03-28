#version 450 core

// Uniforms
uniform nap
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 modelMatrix;
} mvp;

// Vertex Input
in vec3	in_Position;

// Vertex Output
out vec3 passPosition;				//< Vertex position in world space

void main()
{
	passPosition = in_Position;
	//passPosition.xy *= -1.0;

	// Negate translational component
	mat4 view_matrix = mvp.viewMatrix;
	view_matrix[3] = vec4(0.0, 0.0, 0.0, 1.0);

	// Model matrix may only have scaling
	gl_Position = mvp.projectionMatrix * view_matrix * mvp.modelMatrix * vec4(passPosition.xyz, 1.0);
}
