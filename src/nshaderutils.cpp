// Local Includes
#include "nshaderutils.h"
#include "nglutils.h"

namespace opengl
{
	// Constructor
	ShaderInput::ShaderInput(GLuint shaderProgram, std::string& name, GLenum type, GLint location) :
		mName(name),
		mType(type),
		mLocation(location),
		mShaderProgram(shaderProgram)			{}


	// Validates part of the shader
	bool validateShader(GLuint shader)
	{
		const unsigned int BUFFER_SIZE = 512;
		char buffer[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		GLsizei length = 0;

		// If there's info to display do so
		glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer); // Ask OpenGL to give us the log associated with the shader
		if (length > 0)
		{
			printMessage(MessageType::ERROR, "shader compile error: %s", buffer);
			return false;
		}
		return true;
	}


	// Validates an entire shader program
	bool validateShaderProgram(GLuint program)
	{
		const unsigned int BUFFER_SIZE = 512;
		char buffer[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		GLsizei length = 0;

		glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer); // Ask OpenGL to give us the log associated with the program
		if (length > 0) // If we have any information to display
		{
			printMessage(MessageType::ERROR, "shader program: %d link error: %s", program, buffer);
			return false;
		}

		glValidateProgram(program); // Get OpenGL to try validating the program
		GLint status;
		glGetProgramiv(program, GL_VALIDATE_STATUS, &status); // Find out if the shader program validated correctly
		if (status == GL_FALSE) // If there was a problem validating
		{
			printMessage(MessageType::ERROR, "can't validate shader: %d", program);
			return false;
		}
		return true;
	}


	// Extracts all shader uniforms
	void extractShaderUniforms(GLuint program, std::vector<ShaderUniform>& outUniforms)
	{
		outUniforms.clear();

		GLint uniform_count;			// total number of attributes;
		GLint size;						// size of the variable
		GLenum type;					// type of the variable (float, vec3 or mat4, etc)
		const GLsizei bufSize = 256;	// maximum name length
		GLchar name[bufSize];			// variable name in GLSL
		GLsizei length;					// name length

		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniform_count);

		// Sample info shader program info
		for (auto i = 0; i < uniform_count; i++)
		{
			glGetActiveUniform(program, static_cast<GLint>(i), bufSize, &length, &size, &type, name);
			int location = glGetUniformLocation(program, name);
			if (location < 0)
			{
				printMessage(MessageType::ERROR, "unable to query uniform location: %s", name);
				continue;
			}

			// Add
			printMessage(MessageType::INFO, "Uniform: %d, type: %d, name: %s, location: %d", i, (unsigned int)type, name, location);
			outUniforms.emplace_back(ShaderUniform(program, std::string(name), type, location));
		}
	}


	// Extract all shader program attributes
	void extractShaderAttributes(GLuint program, std::vector<ShaderAttribute>& outAttributes)
	{
		outAttributes.clear();

		GLint attribute_count;			// total number of attributes;
		GLint size;						// size of the variable
		GLenum type;					// type of the variable (float, vec3 or mat4, etc)
		const GLsizei bufSize = 256;	// maximum name length
		GLchar name[bufSize];			// variable name in GLSL
		GLsizei length;					// name length

										// Get number of active attributes
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &attribute_count);

		// Sample info shader program info
		for (auto i = 0; i < attribute_count; i++)
		{
			glGetActiveAttrib(program, static_cast<GLint>(i), bufSize, &length, &size, &type, name);
			int location = glGetAttribLocation(program, name);
			if (location < 0)
			{
				printMessage(MessageType::ERROR, "unable to query attribute location: %s", name);
				continue;
			}

			// Add
			printMessage(MessageType::INFO, "Attribute: %d, type: %d, name: %s, location: %d", i, (unsigned int)type, name, location);
			outAttributes.emplace_back(ShaderAttribute(program, std::string(name), type, location));
		}
	}

}
