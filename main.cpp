/**
   OpenVR base by Morgan McGuire
   Terrain and light PoC by Stamate Cosmin
  
  Reference Frames:
      Object: The object being rendered relative to its own origin
      World:  Global reference frame
      Body:   Controlled by keyboard and mouse
      Head:   Controlled by tracking (or fixed relative to the body for non-VR)
      Camera: Fixed relative to the head. The camera is the eye.
 */

// Uncomment to add VR support
//#define _VR

////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_SWIZZLE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "minimalOpenGL.h"
#include "mesh_loader.h"
#include "raw_model.h"
#include "light_system.h"
#include "entity.h"
#include "world.h"

#ifdef _VR
#   include "minimalOpenVR.h"
#endif

GLFWwindow* window = nullptr;

#ifdef _VR
    vr::IVRSystem* hmd = nullptr;
#endif

#define BACKGROUND_COLOR glm::vec4(0.0f, 0.0f, 0.0f, 1)

int keys[1024] = { 0 }; //0 = init, 1 = pressed, 2 = released


// PROTOTYPES
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void getTime(float *previous_time, float *deltaTime, float *time);
void reloadShader(GLuint *shader);

int main(const int argc, const char* argv[]) {

    std::cout << "Spectral Slack\n\nW, A, S, D, Space, and C keys to translate\nMouse click and drag to rotate\nESC to quit\n\n";
    std::cout << std::fixed;

	//////////////////////////////////////////////////////////////////////
	// Instantiate values

    uint32_t framebufferWidth = 1280, framebufferHeight = 720;
#   ifdef _VR
        const int numEyes = 2;
        hmd = initOpenVR(framebufferWidth, framebufferHeight);
        assert(hmd);
#   else
        const int numEyes = 1;
#   endif

    const int windowHeight = 720;
    const int windowWidth = (framebufferWidth * windowHeight) / framebufferHeight;

    window = initOpenGL(windowWidth, windowHeight, "minimalOpenGL");
	glfwSetKeyCallback(window, key_callback);
        
    glm::vec3 bodyTranslation = glm::vec3();
    glm::vec3 bodyRotation;

	RawModelFactory::instantiateModelFactory();
	
	bool wireframe = false;
	glPolygonMode(GL_FRONT_AND_BACK, (wireframe ? GL_LINE : GL_FILL));

	bool lights_on = true;

	Camera* camera = new Camera();

	World* world = new World(glm::vec3(), MOUNTAIN_JAG, WORLD_MODE_FRACTAL);
	//world->setMode();

	LightSystem* light_system = new LightSystem(LIGHT_OMNI, camera);
	//light_system->switchFog();

	float time;
	float previous_time = glfwGetTime();
	float deltaTime;

	//////////////////////////////////////////////////////////////////////
	// Allocate the frame buffer. This code allocates one framebuffer per eye.
	// That requires more GPU memory, but is useful when performing temporal 
	// filtering or making render calls that can target both simultaneously.

	GLuint framebuffer[numEyes];
	glGenFramebuffers(numEyes, framebuffer);

	GLuint colorRenderTarget[numEyes], depthRenderTarget[numEyes];
	glGenTextures(numEyes, colorRenderTarget);
	glGenTextures(numEyes, depthRenderTarget);
	for (int eye = 0; eye < numEyes; ++eye) {
		glBindTexture(GL_TEXTURE_2D, colorRenderTarget[eye]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferWidth, framebufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		assert(glGetError() == GL_NONE);

		glBindTexture(GL_TEXTURE_2D, depthRenderTarget[eye]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, framebufferWidth, framebufferHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
		assert(glGetError() == GL_NONE);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer[eye]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorRenderTarget[eye], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthRenderTarget[eye], 0);
		assert(glGetError() == GL_NONE);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //////////////////////////////////////////////////////////////////////
    // Create the main shader
    GLuint shader = createShaderProgram(loadTextFile("min.vert"), loadTextFile("min.frag"));

    // Binding points for attributes and uniforms discovered from the shader
    const GLint positionAttribute   = glGetAttribLocation(shader,  "position");
    const GLint normalAttribute     = glGetAttribLocation(shader,  "normal");
    const GLint texCoordAttribute   = glGetAttribLocation(shader,  "texCoord"); 
    const GLint colorTextureUniform = glGetUniformLocation(shader, "colorTexture");

    const GLuint uniformBlockIndex = glGetUniformBlockIndex(shader, "Uniform");
    const GLuint uniformBindingPoint = 6;
    glUniformBlockBinding(shader, uniformBlockIndex, uniformBindingPoint);

    GLuint uniformBlock;
    glGenBuffers(1, &uniformBlock);

    {
        // Allocate space for the uniform block buffer
        GLint uniformBlockSize;
        glGetActiveUniformBlockiv(shader, uniformBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &uniformBlockSize);
        glBindBuffer(GL_UNIFORM_BUFFER, uniformBlock);
        glBufferData(GL_UNIFORM_BUFFER, uniformBlockSize, nullptr, GL_DYNAMIC_DRAW);
    }

	assert(glGetError() == GL_NONE);

    const GLchar* uniformName[] = {
        "Uniform.objectToWorldNormalMatrix",
        "Uniform.objectToWorldMatrix",
        "Uniform.modelViewProjectionMatrix",
        "Uniform.cameraPosition"};

    const int numBlockUniforms = sizeof(uniformName) / sizeof(uniformName[0]);
#   ifdef _DEBUG
    {
        GLint debugNumUniforms = 0;
        glGetProgramiv(shader, GL_ACTIVE_UNIFORMS, &debugNumUniforms);
        for (GLint i = 0; i < debugNumUniforms; ++i) {
            GLchar name[1024];
            GLsizei size = 0;
            GLenum type = GL_NONE;
            glGetActiveUniform(shader, i, sizeof(name), nullptr, &size, &type, name);
            std::cout << "Uniform #" << i << ": " << name << "\n";
        }
        assert(debugNumUniforms >= numBlockUniforms);
    }
#   endif

    // World uniform names to indices within the block
    GLuint uniformIndex[numBlockUniforms];
    glGetUniformIndices(shader, numBlockUniforms, uniformName, uniformIndex);
    assert(uniformIndex[0] < 10000);
	assert(glGetError() == GL_NONE);

    // World indices to byte offsets
    GLint  uniformOffset[numBlockUniforms];
    glGetActiveUniformsiv(shader, numBlockUniforms, uniformIndex, GL_UNIFORM_OFFSET, uniformOffset);
    assert(uniformOffset[0] >= 0);
	assert(glGetError() == GL_NONE);

    // Load a texture map
    GLuint colorTexture = GL_NONE;
    {
        int textureWidth, textureHeight, channels;
        std::vector<std::uint8_t> data;
        loadBMP("color.bmp", textureWidth, textureHeight, channels, data);

        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, textureWidth, textureHeight, 0, (channels == 3) ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
	assert(glGetError() == GL_NONE);

    GLuint trilinearSampler = GL_NONE;
    {
        glGenSamplers(1, &trilinearSampler);
        glSamplerParameteri(trilinearSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glSamplerParameteri(trilinearSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(trilinearSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(trilinearSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
	assert(glGetError() == GL_NONE);

#   ifdef _VR
        vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
#   endif

	//glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

	const float pi = glm::pi<float>();

	// MATRIX 4X4 DELARATIONS
	
	glm::mat4& bodyToWorldMatrix = glm::mat4(1.0f);
	glm::mat4& headToWorldMatrix = glm::mat4(1.0f);
	glm::mat4& objectToWorldMatrix = glm::mat4(1.0f);
	glm::mat4& modelViewProjectionMatrix = glm::mat4(1.0f);
	glm::mat4 eyeToHead[numEyes] = { glm::mat4(1.0f) };
	glm::mat4 projectionMatrix[numEyes] = { glm::mat4(1.0f) };
	glm::mat4 headToBodyMatrix = glm::mat4(1.0f);
	glm::mat4 model_matrix = glm::mat4(1.0f);
	glm::mat4& cameraToWorldMatrix = glm::mat4(1.0f);

	// STILL MAKING DECLARATIONS

	glm::mat3& objectToWorldNormalMatrix = glm::mat3(1.0f);
	glm::vec3& cameraPosition = glm::vec3(cameraToWorldMatrix[3]);
	const float nearPlaneZ = 0.1f;
	const float farPlaneZ = 15000.0f;
	const float verticalFieldOfView = glm::radians(45.0f);

	glUseProgram(shader);

	glUniform4f(glGetUniformLocation(shader, "background_color"),
		BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b,
		BACKGROUND_COLOR.a);

	//send top and bottom world color thresholds
	glUniform4f(glGetUniformLocation(shader, "color_top"),
		WORLD_TOP_COLOR.r, WORLD_TOP_COLOR.g, WORLD_TOP_COLOR.b, WORLD_TOP_COLOR.a);
	glUniform4f(glGetUniformLocation(shader, "color_bottom"),
		WORLD_BOTTOM_COLOR.r, WORLD_BOTTOM_COLOR.g, WORLD_BOTTOM_COLOR.b,
		WORLD_BOTTOM_COLOR.a);

	//send fog information
	glUniform1f(glGetUniformLocation(shader, "fog_start"), FOG_START_RADIUS);
	glUniform1f(glGetUniformLocation(shader, "fog_end"), FOG_END_RADIUS);
	glUniform4f(glGetUniformLocation(shader, "fog_color"),
		FOG_COLOR.x, FOG_COLOR.y, FOG_COLOR.z, FOG_COLOR.a);

	glUniform4f(glGetUniformLocation(shader, "ambiental_light"),
		LIGHT_AMBIENTAL.x, LIGHT_AMBIENTAL.y, LIGHT_AMBIENTAL.z,
		LIGHT_AMBIENTAL.a);

	glUniform3f(glGetUniformLocation(shader, "spotlight_direction"),
		LIGHT_SPOT_DIRECTION.x, LIGHT_SPOT_DIRECTION.y,
		LIGHT_SPOT_DIRECTION.z);
	
	/////////////////////////////////////////////////////////////////////
    // Main loop
    int timer = 0;
	float frameTimes[100];
	int totalFrames = 0;
	float averageFrame;

    while (! glfwWindowShouldClose(window)) {
        assert(glGetError() == GL_NONE);
		
		getTime(&previous_time, &deltaTime, &time); //WHAT YEAR IS IT
		frameTimes[totalFrames++%100] = time;
		if (totalFrames == 100) {
			averageFrame = 0;
			for (int i = 0; i < 100; i++) {
				averageFrame += frameTimes[i];
			}
			printf("Avg time per frame: %f\n", averageFrame / 100);
			totalFrames = 0;
		}



#       ifdef _VR
            getEyeTransformations(hmd, trackedDevicePose, nearPlaneZ, farPlaneZ, glm::value_ptr(headToBodyMatrix), glm::value_ptr(eyeToHead[0]), glm::value_ptr(eyeToHead[1]), glm::value_ptr(projectionMatrix[0]), glm::value_ptr(projectionMatrix[1]));
#       else
		projectionMatrix[0] = glm::perspective(verticalFieldOfView, float(framebufferWidth / framebufferHeight), nearPlaneZ, farPlaneZ);
#       endif

		
		cameraPosition = glm::vec3(cameraToWorldMatrix[3]);
		model_matrix = glm::mat4(1.0f);
		modelViewProjectionMatrix = glm::mat4(1.0f);

		bodyToWorldMatrix = //view
            glm::translate(bodyToWorldMatrix, bodyTranslation) *
            glm::rotate(bodyToWorldMatrix, bodyRotation.z, glm::vec3(0, 0, 1)) *
            glm::rotate(bodyToWorldMatrix, bodyRotation.y, glm::vec3(0, 1, 0)) *
			glm::rotate(bodyToWorldMatrix, bodyRotation.x, glm::vec3(1, 0, 0));

		headToWorldMatrix = bodyToWorldMatrix * headToBodyMatrix;

        for (int eye = 0; eye < numEyes; ++eye) {
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer[eye]);
            glViewport(0, 0, framebufferWidth, framebufferHeight);

            //glClearColor(0.1f, 0.2f, 0.3f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			cameraToWorldMatrix = headToWorldMatrix * eyeToHead[eye];

			// Set drawing mode to fill, for other elements than the world
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			//2nd shader sky drawer
#       ifdef _VR
			drawSky(framebufferWidth, framebufferHeight, glm::value_ptr(glm::inverse(cameraToWorldMatrix)), glm::value_ptr(projectionMatrix[eye]));
#		else
			drawSky(framebufferWidth, framebufferHeight, glm::value_ptr(cameraToWorldMatrix), glm::value_ptr(projectionMatrix[eye]));
#		endif
			
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glEnable(GL_CULL_FACE);
			glDepthMask(GL_TRUE);

			glUseProgram(shader);

			// uniform colorTexture - sampler binding
			const GLint colorTextureUnit = 0;
            glActiveTexture(GL_TEXTURE0 + colorTextureUnit);
            glBindTexture(GL_TEXTURE_2D, colorTexture);
            glBindSampler(colorTextureUnit, trilinearSampler);
            glUniform1i(colorTextureUniform, colorTextureUnit);

			glUniform1i(glGetUniformLocation(shader, "lights_on"),
				lights_on);

			cameraPosition = glm::vec3(cameraToWorldMatrix[3]);

			//reset some matrices to prevent recursive transformations
			model_matrix = glm::mat4(1.0f);
			modelViewProjectionMatrix = glm::mat4(1.0f);
			bodyToWorldMatrix = glm::mat4(1.0f);
			objectToWorldMatrix = glm::mat4(1.0f);

			light_system->render(shader, model_matrix, &objectToWorldMatrix, &projectionMatrix[eye], &cameraToWorldMatrix, &modelViewProjectionMatrix, &objectToWorldNormalMatrix, uniformBindingPoint, uniformBlock, uniformOffset);

			// Draw the world
			glPolygonMode(GL_FRONT_AND_BACK, (wireframe ? GL_LINE : GL_FILL));
			world->render(shader, model_matrix, cameraPosition, &objectToWorldMatrix, &projectionMatrix[eye], &cameraToWorldMatrix, &modelViewProjectionMatrix, &objectToWorldNormalMatrix, uniformBindingPoint, uniformBlock, uniformOffset);

#           ifdef _VR
            {
                vr::Texture_t tex = { reinterpret_cast<void*>(intptr_t(colorRenderTarget[eye])), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
                vr::VRCompositor()->Submit(vr::EVREye(eye), &tex);
            }
#           endif
			
        } // for each eye

        ////////////////////////////////////////////////////////////////////////
#       ifdef _VR
            // Tell the compositor to begin work immediately instead of waiting for the next WaitGetPoses() call
            vr::VRCompositor()->PostPresentHandoff();
#       endif

        // Mirror to the window
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_NONE);
        glViewport(0, 0, windowWidth, windowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBlitFramebuffer(0, 0, framebufferWidth, framebufferHeight, 0, 0, windowWidth, windowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, GL_NONE);

        // Display what has been drawn on the main window
        glfwSwapBuffers(window);

        // Check for events
        glfwPollEvents();

        // Handle events
		if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, 1);
		}
		// WASD keyboard movement
		const float cameraMoveSpeed = 500.0f * deltaTime;

		if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_W)) {
			bodyTranslation += glm::vec3(headToWorldMatrix * glm::vec4(0, 0, -cameraMoveSpeed, 0));
		}

		if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_S)) {
			bodyTranslation += glm::vec3(headToWorldMatrix * glm::vec4(0, 0, +cameraMoveSpeed, 0));
		}

		if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_A)) {
			bodyTranslation += glm::vec3(headToWorldMatrix * glm::vec4(-cameraMoveSpeed, 0, 0, 0));
		}

		if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_D)) {
			bodyTranslation += glm::vec3(headToWorldMatrix * glm::vec4(+cameraMoveSpeed, 0, 0, 0));
		}

		if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_SPACE)) { bodyTranslation.y += cameraMoveSpeed; }
		if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_C)) { bodyTranslation.y -= cameraMoveSpeed; }

		if (keys[GLFW_KEY_Q] == 1) { light_system->addLight(cameraPosition); }
		if (keys[GLFW_KEY_E] == 1) { light_system->switchType(); }
		if (keys[GLFW_KEY_G] == 1) { wireframe = !wireframe; }
		if (keys[GLFW_KEY_F] == 1) { light_system->switchFog(); }
		if (keys[GLFW_KEY_X] == 1) { reloadShader(&shader); }
		if (keys[GLFW_KEY_T] == 1) { light_system->switchCanMove(); }

		/*if (keys[GLFW_KEY_I] == 2) { light_system->setControl(ENTITY_CONTROL_FORWARD); }
		else { light_system->unsetControl(ENTITY_CONTROL_FORWARD); }

		if (keys[GLFW_KEY_J] == 2) { light_system->setControl(ENTITY_CONTROL_LEFT); }
		else { light_system->unsetControl(ENTITY_CONTROL_LEFT); }

		if (keys[GLFW_KEY_K] == 2) { light_system->setControl(ENTITY_CONTROL_BACKWARD); }
		else { light_system->unsetControl(ENTITY_CONTROL_BACKWARD); }

		if (keys[GLFW_KEY_L] == 2) { light_system->setControl(ENTITY_CONTROL_RIGHT); }
		else { light_system->unsetControl(ENTITY_CONTROL_RIGHT); }*/
        
		// Keep the camera above the ground
        //if (bodyTranslation.y < world->) { bodyTranslation.y = 0.01f; }

        static bool inDrag = false;
        const float cameraTurnSpeed = 0.005f;
		static double startX, startY;
		double currentX, currentY;

        if (GLFW_PRESS == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {

            glfwGetCursorPos(window, &currentX, &currentY);
            if (inDrag) {
                bodyRotation.y -= float(currentX - startX) * cameraTurnSpeed;
                bodyRotation.x -= float(currentY - startY) * cameraTurnSpeed;
            }
            inDrag = true; startX = currentX; startY = currentY;
        } else {
            inDrag = false;
        }

		light_system->move(deltaTime, cameraPosition, cameraMoveSpeed / 2.0f);

		//reset keys in use
		keys[GLFW_KEY_Q] = 0;
		keys[GLFW_KEY_E] = 0;
		keys[GLFW_KEY_G] = 0;
		keys[GLFW_KEY_F] = 0;
		keys[GLFW_KEY_X] = 0;
		keys[GLFW_KEY_T] = 0;
    }
	
#   ifdef _VR
        if (hmd != nullptr) {
            vr::VR_Shutdown();
        }
#   endif
	
	// Destructors
	glDeleteProgram(shader);
	camera->~Camera();
	light_system->~LightSystem();
	world->~World();
	RawModelFactory::destructModelFactory();

    // Close the GL context and release all resources
    glfwTerminate();

    return 0;
}

// Get time from last frame
void getTime(float *previous_time, float *deltaTime, float *time) {
	double current_time;

	current_time = glfwGetTime();
	*deltaTime = current_time - *previous_time;
	*previous_time = current_time;
	*time = (float)(*deltaTime) / 1000.0f;
}

// Currently broken shader reloader
void reloadShader(GLuint *shader) {
	GLsizei shaderCount = 0;
	GLuint shaders[] = { 0 };

	glGetAttachedShaders(*shader, 1, &shaderCount, shaders);

	for (int i = 0; i < shaderCount; i++) {
		glDetachShader(*shader, shaders[i]);
		glDeleteShader(shaders[i]);
	}
	glDeleteProgram(*shader);
	*shader = createShaderProgram(loadTextFile("min.vert"), loadTextFile("min.frag"));
	std::cout << "hello";
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	//cout << key << endl;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS) {
			if (keys[key] == 0)
			keys[key] = 1;
		}

		if (action == GLFW_RELEASE) {
			if (keys[key] == 1) {
				keys[key] = 2;
			}
		}
	}
}

#ifdef _WINDOWS
    int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw) {
        return main(0, nullptr);
    }
#endif
