// firstSDLapp.cpp : Defines the entry point for the console application.
//

// SDL
#include <SDL.h>

// Naivi GL
#include <nopengl.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  
#include <glm/ext.hpp>
#include <chrono>

// C++ Headers
#include <string>
#include <iostream>
#include <FreeImage.h>

// OpenGL / glew Headers
#define GL3_PROTOTYPES 1
#include <GL/glew.h>
#include <glm/matrix.hpp>

// Mod nap render includes
#include <material.h>
#include <modelresource.h>
#include <modelmeshcomponent.h>

// Nap includes
#include <nap/core.h>
#include <nap/resourcemanager.h>

// STD includes
#include <ctime>

//////////////////////////////////////////////////////////////////////////
// Globals
//////////////////////////////////////////////////////////////////////////

// Window Name
std::string		programName		= "Model Loading Test";
std::string		vertShaderName	= "shaders/shader.vert";
std::string		fragShaderName	= "shaders/shader.frag";

static const std::string testTextureName = "data/test.jpg";
static std::unique_ptr<opengl::Image> testTexture;
static const std::string pigTextureName = "data/pig_head.jpg";
static std::unique_ptr<opengl::Image> pigTexture;

// Vertex buffer that holds all the fbo's
opengl::VertexArrayObject	cubeObject;
opengl::FloatVertexBuffer	squarePositionBuffer;
opengl::FloatVertexBuffer	squareColorBuffer;
opengl::FloatVertexBuffer	squareUVBuffer;

// Vertex buffer that holds a triangle
opengl::VertexArrayObject	triangleObject;
opengl::VertexContainer		trianglePositions;
opengl::VertexContainer		triangleColors;
opengl::VertexContainer		triangleUvs;

// Shader uniform bind locations
int	projectionMatrixLocation(-1);
int	viewMatrixLocation(-1);
int	modelMatrixLocation(-1);
int	noiseLocation(-1);
int textureLocation(-1);

// Camera
opengl::Camera camera;

// vertex Shader indices
nap::Entity* model = nullptr;
int vertex_index(0), color_index(0), normal_index(0), uv_index(0);

// Our SDL_Window ( just like with SDL2 wihout OpenGL)
SDL_Window*					mainWindow;

// Our opengl context handle
SDL_GLContext				mainContext;

// Current texture to draw
unsigned int				currentIndex = 0;

// Window width / height on startup
unsigned int windowWidth(512);
unsigned int windowHeight(512);

// GLM
glm::mat4 viewMatrix;			// Store the view matrix
glm::mat4 modelMatrix;			// Store the model matrix

// Some utilities
void runGame();
void cleanup();
void createSquare();		
void createTriangle();		
bool loadImages();
void updateViewport(int width, int height);

/**
 * Creates a low level opengl triangle object
 */
void createTriangle(void)
{
	int face_point_count = 3;
	int face_count = 1;
	int point_count = face_point_count * face_count;

	int vert_size = 3;
	float vertices[] =
	{
		-1.0f,	-1.0f,	0.0f,
		0.0f,	1.0f,	0.0f,
		1.0f,	-1.0f,	0.0f
	};

	int color_size = 4;
	float colors[] =
	{
		1.0f,	0.0f,	0.0f,  1.0f,
		0.0f,	1.0f,	0.0f,  1.0f,
		0.0f,	0.0f,	1.0f , 1.0f
	};

	int uv_size = 3;
	float uvs[] =
	{
		1.0f,	1.0f, 1.0f,
		1.0f,	0.0f, 0.0f,
		0.0f,	0.0f, 0.0f
	};

	// Initialize and bind triangle array object
	triangleObject.init();

	trianglePositions.copyData(GL_FLOAT, vert_size, point_count, vertices);
	triangleObject.addVertexBuffer(vertex_index, *trianglePositions.getVertexBuffer());

	triangleColors.copyData(GL_FLOAT, color_size, point_count, colors);
	triangleObject.addVertexBuffer(color_index, *triangleColors.getVertexBuffer());

	triangleUvs.copyData(GL_FLOAT, uv_size, point_count, uvs);
	triangleObject.addVertexBuffer(uv_index, *triangleUvs.getVertexBuffer());
}


/**
createSquare is used to create the Vertex Array Object which will hold our square. We will
be hard coding in the vertices for the square, which will be done in this method.
*/
void createSquare(void)
{
	int face_point_count(6);
	int face_count(6);
	int point_count = face_point_count * face_count;

	int vert_size = 3;
	int vert_count = vert_size * point_count;
	float vertices[] =
	{
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,

		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,

		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,

		-1.0f,  1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,
	};

	int color_size = 4;
	int color_count = color_size * point_count;
	float colors[]
	{
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,

		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,

		0.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,

		0.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f, 1.0f,

		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,

		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f
	};

	int uv_size = 3;
	int uv_count = uv_size * point_count;
	float uvs[] =
	{
		0.0f, 0.0f, 0.0f,
		1.0f, 0.0f,	0.0f,
		1.0f, 1.0f,	1.0f,
		1.0f, 1.0f,	1.0f,
		0.0f, 1.0f,	1.0f,
		0.0f, 0.0f,	0.0f,

		0.0f, 0.0f,	0.0f,
		1.0f, 0.0f,	0.0f,
		1.0f, 1.0f,	1.0f,
		1.0f, 1.0f,	1.0f,
		0.0f, 1.0f,	1.0f,
		0.0f, 0.0f,	0.0f,

		1.0f, 0.0f,	0.0f,
		1.0f, 1.0f,	1.0f,
		0.0f, 1.0f,	1.0f,
		0.0f, 1.0f,	1.0f,
		0.0f, 0.0f,	0.0f,
		1.0f, 0.0f,	0.0f,

		1.0f, 0.0f,	0.0f,
		1.0f, 1.0f,	1.0f,
		0.0f, 1.0f,	1.0f,
		0.0f, 1.0f,	1.0f,
		0.0f, 0.0f,	0.0f,
		1.0f, 0.0f,	0.0f,

		0.0f, 1.0f,	1.0f,
		1.0f, 1.0f,	1.0f,
		1.0f, 0.0f,	0.0f,
		1.0f, 0.0f,	0.0f,
		0.0f, 0.0f,	0.0f,
		0.0f, 1.0f,	1.0f,

		0.0f, 1.0f,	1.0f,
		1.0f, 1.0f,	1.0f,
		1.0f, 0.0f,	0.0f,
		1.0f, 0.0f,	0.0f,
		0.0f, 0.0f,	0.0f,
		0.0f, 1.0f,	1.0f,
	};

	// Init vertex buffer (allocate)
	cubeObject.init();

	squarePositionBuffer.init();
	squarePositionBuffer.setData(vert_size, point_count, GL_STATIC_DRAW, vertices);
	cubeObject.addVertexBuffer(vertex_index, squarePositionBuffer);

	squareColorBuffer.init();
	squareColorBuffer.setData(color_size, point_count, GL_STATIC_DRAW, colors);
	cubeObject.addVertexBuffer(color_index, squareColorBuffer);

	squareUVBuffer.init();
	squareUVBuffer.setData(uv_size, point_count, GL_STATIC_DRAW, uvs);
	cubeObject.addVertexBuffer(uv_index, squareUVBuffer);
}


/**
* Loads a bitmap from disk
*/
bool loadImages()
{
	// Load blend image
	testTexture = std::make_unique<opengl::Image>(testTextureName);
	testTexture->setCompressed(true);
	if (!testTexture->load())
	{
		opengl::printMessage(opengl::MessageType::ERROR, "unable to load blend image: %s", testTextureName.c_str());
		return false;
	}

	pigTexture = std::make_unique<opengl::Image>(pigTextureName);
	pigTexture->setCompressed(true);
	if (!pigTexture->load())
	{
		opengl::printMessage(opengl::MessageType::ERROR, "unable to load pig texture: %s", pigTextureName.c_str());
		return false;
	}

	return true;
}

/**
* Updates gl viewport and projection matrix used for rendering
*/
void updateViewport(int width, int height)
{
	glViewport(0, 0, width, height);
	camera.setAspectRatio((float)width, (float)height);
}


/**
 * Initialize opengl context and create windows etc.
 */
bool initOpenGL()
{
	// Initialize OpenGL
	if (!opengl::initVideo())
		return false;

	// Set GL Attributes
	opengl::Attributes attrs;
	attrs.dubbleBuffer = true;
	attrs.versionMinor = 2;
	attrs.versionMajor = 3;
	opengl::setAttributes(attrs);

	// Create Window
	opengl::WindowSettings window_settings;
	window_settings.width = windowWidth;
	window_settings.height = windowHeight;
	window_settings.borderless = false;
	window_settings.resizable = true;
	window_settings.title = programName;

	// Print error if window could not be created
	mainWindow = opengl::createWindow(window_settings);
	if (mainWindow == nullptr)
		return false;

	// Create context
	mainContext = opengl::createContext(*mainWindow, true);
	if (mainContext == nullptr)
		return false;

	// Initialize glew
	opengl::init();

	// Enable multi sampling
	glEnable(GL_MULTISAMPLE);

	int Buffers(1), Samples(4);
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &Buffers);
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &Samples);

	return true;
}


/**
 * Initialize all the resources and instances used for drawing
 * slowly migrating all functionality to nap
 */
bool init(nap::Core& core)
{
	if (!initOpenGL())
	{
		nap::Logger::warn("unable to initialize opengl");
		return false;
	}

	// Load bitmap
	if (!loadImages())
	{
		nap::Logger::fatal("unable to load images");
		return false;
	}

	//////////////////////////////////////////////////////////////////////////

	// Create shader resource
	nap::ResourceManagerService* service = core.getOrCreateService<nap::ResourceManagerService>();
	service->setAssetRoot(".");
	nap::Resource* shader_resource = service->getResource(fragShaderName);

	// Load model resource
	nap::Resource* model_resource = service->getResource("data/pig_head_alpha_rotated.fbx");
	if (model_resource == nullptr)
	{
		nap::Logger::warn("unable to load pig head model resource");
		return false;
	}
	nap::ModelResource* pig_model = static_cast<nap::ModelResource*>(model_resource);

	// Create model entity
	model = &(core.getRoot().addEntity("model"));
	nap::ModelMeshComponent& mesh_component = model->addComponent<nap::ModelMeshComponent>("pig_head_mesh");
	
	//////////////////////////////////////////////////////////////////////////
	
	// Set shader resource on material
	nap::Material* material = mesh_component.getMaterial();
	assert(material != nullptr);
	material->shaderResource.setResource(*shader_resource);

	// Link model resource
	mesh_component.modelResource.setResource(*pig_model);

	//////////////////////////////////////////////////////////////////////////

	// Extract mesh
	const opengl::Mesh* mesh = pig_model->getMesh(0);
	if (mesh == nullptr)
	{
		nap::Logger::warn("unable to extract model mesh at index 0");
		return false;
	}

	// Get buffer indices for mesh (TODO: RESOLVE DYNAMICALLY)
	vertex_index = mesh->getVertexBufferIndex();
	color_index = mesh->getColorBufferIndex(0);
	normal_index = mesh->getNormalBufferIndex();
	uv_index = mesh->getUvBufferIndex(0);

	// Bind indices explicit to shader (TODO: RESOLVE DYNAMICALLY)
	// This tells what vertex buffer index belongs to what vertex shader input binding name
	opengl::Shader& shader = material->getResource()->getShader();
	shader.bindVertexAttribute(vertex_index, "in_Position");
	shader.bindVertexAttribute(color_index, "in_Color");
	shader.bindVertexAttribute(uv_index, "in_Uvs");

	// Get uniform bindings for vertex shader
	material->bind();
	projectionMatrixLocation =	glGetUniformLocation(shader.getId(), "projectionMatrix");	// Get the location of our projection matrix in the shader
	viewMatrixLocation =		glGetUniformLocation(shader.getId(), "viewMatrix");			// Get the location of our view matrix in the shader
	modelMatrixLocation =		glGetUniformLocation(shader.getId(), "modelMatrix");		// Get the location of our model matrix in the shader
	noiseLocation =				glGetUniformLocation(shader.getId(), "noiseValue");
	textureLocation =			glGetUniformLocation(shader.getId(), "myTextureSampler");
	
	material->unbind();

	//////////////////////////////////////////////////////////////////////////

	// View matrix
	viewMatrix = glm::lookAt
	(
		glm::vec3(0.0f, 0.0f, 4.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	// Set camera
	camera.setFieldOfView(45.0f);
	camera.setAspectRatio((float)windowWidth, (float)windowHeight);

	// Update gl viewport
	updateViewport(windowWidth, windowHeight);

	// Create Square Vertex Buffer Object
	createSquare();

	// Create triangle Vertex Buffer Object
	createTriangle();

	return true;
}


// Main loop
int main(int argc, char *argv[])
{
	// Create core
	nap::Core core;

	// Initialize render stuff
	if (!init(core))
		return -1;

	// Run Gam
	runGame();

	// Cleanup when done
	cleanup();

	return 0;
}


void runGame()
{
	// Run function
	bool loop = true;
	bool depth = true;

	// Get start
	auto t_start = std::chrono::high_resolution_clock::now();

	// Enable some gl specific stuff
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE_ARB);

	// Timer
	std::clock_t begin = std::clock();

	// Get mesh component
	nap::ModelMeshComponent* mesh_comp = model->getComponent<nap::ModelMeshComponent>();
	nap::Material* material = mesh_comp->getMaterial();
	assert(material != nullptr);

	// Loop
	while (loop)
	{
		SDL_Event event;
		if (SDL_PollEvent(&event))
		{
			// Check if we need to quit
			if (event.type == SDL_QUIT)
				loop = false;


			// Check if escape was pressed
			if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_d:
					depth = !depth;
					break;
				case SDLK_ESCAPE:
					loop = false;
					break;
				case SDLK_f:
				{
					SDL_SetWindowFullscreen(mainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
					break;
				}
				case SDLK_PERIOD:
				{
					currentIndex++;
					currentIndex = currentIndex < 3 ? currentIndex : 0;
					break;
				}
				default:
					break;
				}
			}

			if (event.type == SDL_WINDOWEVENT)
			{
				switch (event.window.event)
				{
				case SDL_WINDOWEVENT_RESIZED:
					// Update gl viewport
					updateViewport(event.window.data1, event.window.data2);
				default:
					break;
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////

		// get time passed
		auto t_now = std::chrono::high_resolution_clock::now();
		double time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();
		double time_angle = glm::radians(time * 360.0f);

		// Calculate model rotate angle
		double rotate_speed = 0.5f;
		double rotate_angle = time_angle * rotate_speed;

		// Calculate model pivot offset
		double pivot_distance = 2.0f;
		double pivot_speed = 0.25f;
		double pivot_offset = ((sin(time_angle * pivot_speed) + 1.0) / 2.0) * pivot_distance;

		// Clear window
		opengl::clearColor(0.0f, 0.0f, 0.0f, 1.0f);
		opengl::clearDepth();
		opengl::clearStencil();

		// Disable / Enable depth tests
		opengl::enableDepthTest(depth);

		// Bind Shader
		material->bind();

		// Parent matrix
		glm::mat4 parent_matrix;
		parent_matrix = glm::rotate(parent_matrix, (float)rotate_angle, glm::vec3(0.0f, 1.0f, 0.0f));

		// Model matrix
		glm::mat4 model_matrix;
		model_matrix = glm::translate(model_matrix, glm::vec3((float)pivot_offset, 0.0f, 0.0f));
		model_matrix = glm::rotate(model_matrix, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model_matrix = glm::scale(model_matrix, glm::vec3(0.5, 0.5, 0.5));
		glm::mat4 final_model_matrix = parent_matrix * model_matrix;

		// Send values
		glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &camera.getProjectionMatrix()[0][0]); // Send our projection matrix to the shader
		glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]); // Send our view matrix to the shader
		glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &(final_model_matrix[0][0]));

		// Set texture 1 for shader
		glActiveTexture(GL_TEXTURE0);

		// Bind correct texture and send to shader
		opengl::Image* img = currentIndex == 0 ? pigTexture.get() : testTexture.get();
		img->bind();
		glUniform1i(textureLocation, 0);

		// Unbind shader
		material->unbind();

		switch (currentIndex)
		{
		case 0:
		{
			mesh_comp->draw();
			break;
		case 1:
			cubeObject.bind();
			material->bind();
			cubeObject.draw(GL_TRIANGLES);
			material->unbind();
			cubeObject.unbind();
			break;
		}
		case 2:
			triangleObject.bind();
			material->bind();
			triangleObject.draw(GL_TRIANGLES);
			material->unbind();
			triangleObject.unbind();
			break;
		}

		// Swap front / back buffer
		opengl::swap(*mainWindow);
	}
}


void cleanup()
{
	model = nullptr;

	// Delete our OpengL context
	opengl::deleteContext(mainContext);

	// Destroy our window
	opengl::destroyWindow(*mainWindow);

	// Shutdown SDL 2
	opengl::shutdown();
}

