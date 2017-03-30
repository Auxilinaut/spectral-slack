/**
   OpenVR base by Morgan McGuire: http://graphics.cs.williams.edu
  
  Reference Frames:
      Object: The object being rendered (the Shape in this example) relative to its own origin
      World:  Global reference frame
      Body:   Controlled by keyboard and mouse
      Head:   Controlled by tracking (or fixed relative to the body for non-VR)
      Camera: Fixed relative to the head. The camera is the eye.
 */

// Uncomment to add VR support
//#define _VR

////////////////////////////////////////////////////////////////////////////////

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "minimalOpenGL.h"
#include "mesh_loader.h"
#include "raw_model.h"
#include "light_system.h"
#include "movable.h"
#include "map.h"

#ifdef _VR
#   include "minimalOpenVR.h"
#endif

GLFWwindow* window = nullptr;

#ifdef _VR
    vr::IVRSystem* hmd = nullptr;
#endif

#define BACKGROUND_COLOR glm::vec4(0.886f, 0.941f, 0.953f, 1)

// Get time from last frame.
void getTime(int *previous_time, float *time) {
	int time_interval;
	int current_time;

	current_time = glfwGetTime();
	time_interval = current_time - *previous_time;
	*previous_time = current_time;
	*time = (float)time_interval / 1000.0f;
}

int main(const int argc, const char* argv[]) {
    std::cout << "Minimal OpenGL 4.1 Example by Morgan McGuire\n\nW, A, S, D, C, Z keys to translate\nMouse click and drag to rotate\nESC to quit\n\n";
    std::cout << std::fixed;

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
        
    glm::vec3 bodyTranslation(0.0f, 1.6f, 5.0f);
    glm::vec3 bodyRotation;

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

        glBindTexture(GL_TEXTURE_2D, depthRenderTarget[eye]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, framebufferWidth, framebufferHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer[eye]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorRenderTarget[eye], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D, depthRenderTarget[eye], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//////////////////////////////////////////////////////////////////////
	// Instantiate values

	RawModelFactory::instantiateModelFactory();
	
	bool wireframe = false;
	glPolygonMode(GL_FRONT_AND_BACK, (wireframe ? GL_LINE : GL_FILL));

	bool lights_on = true;

	Camera* camera = new Camera();

	LightSystem* light_system = new LightSystem(LIGHT_OMNI, camera);

	Map* map = new Map(glm::vec3(0.0f,0.0f,0.0f), FOG_END_RADIUS, MAP_MODE_BASE);
	map->setMode(MAP_MODE_FRACTAL);

	float time;
	int previous_time = glfwGetTime();

    //////////////////////////////////////////////////////////////////////
    // Create the main shader
    const GLuint shader = createShaderProgram(loadTextFile("min.vert"), loadTextFile("min.frag"));

    // Binding points for attributes and uniforms discovered from the shader
    const GLint positionAttribute   = glGetAttribLocation(shader,  "position");
    const GLint normalAttribute     = glGetAttribLocation(shader,  "normal");
    const GLint texCoordAttribute   = glGetAttribLocation(shader,  "texCoord"); 
    const GLint colorTextureUniform = glGetUniformLocation(shader, "colorTexture");

    const GLuint uniformBlockIndex = glGetUniformBlockIndex(shader, "Uniform");
    const GLuint uniformBindingPoint = 5;
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

    // Map uniform names to indices within the block
    GLuint uniformIndex[numBlockUniforms];
    glGetUniformIndices(shader, numBlockUniforms, uniformName, uniformIndex);
    assert(uniformIndex[0] < 10000);

    // Map indices to byte offsets
    GLint  uniformOffset[numBlockUniforms];
    glGetActiveUniformsiv(shader, numBlockUniforms, uniformIndex, GL_UNIFORM_OFFSET, uniformOffset);
    assert(uniformOffset[0] >= 0);

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

    GLuint trilinearSampler = GL_NONE;
    {
        glGenSamplers(1, &trilinearSampler);
        glSamplerParameteri(trilinearSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glSamplerParameteri(trilinearSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(trilinearSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(trilinearSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

#   ifdef _VR
        vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
#   endif

	const float pi = glm::pi<float>();

	// MATRIX 4X4'S
	
	

	glm::mat4 eyeToHead[numEyes] = { glm::mat4(1.0f) };
	glm::mat4 projectionMatrix[numEyes] = { glm::mat4(1.0f) };
	glm::mat4 headToBodyMatrix = glm::mat4(1.0f);
	
	/////////////////////////////////////////////////////////////////////
    // Main loop
    int timer = 0;
    while (! glfwWindowShouldClose(window)) {
        assert(glGetError() == GL_NONE);

        const float nearPlaneZ = 0.1f;
        const float farPlaneZ = 100.0f;
        const float verticalFieldOfView = 45.0f * pi / 180.0f;
		
		getTime(&previous_time, &time); //WHAT YEAR IS IT

#       ifdef _VR
            getEyeTransformations(hmd, trackedDevicePose, nearPlaneZ, farPlaneZ, headToBodyMatrix.data, eyeToHead[0].data, eyeToHead[1].data, projectionMatrix[0].data, projectionMatrix[1].data);
#       else
		projectionMatrix[0] = glm::perspective(verticalFieldOfView, float(framebufferWidth / framebufferHeight), -nearPlaneZ, farPlaneZ);
#       endif

        // printf("float nearPlaneZ = %f, farPlaneZ = %f; int width = %d, height = %d;\n", nearPlaneZ, farPlaneZ, framebufferWidth, framebufferHeight);

		glm::mat4& bodyToWorldMatrix = glm::mat4(1.0f);
		glm::mat4& headToWorldMatrix = glm::mat4(1.0f);
		glm::mat4& objectToWorldMatrix = glm::mat4(1.0f);


		bodyToWorldMatrix = //view
            glm::translate(bodyToWorldMatrix, bodyTranslation) *
            glm::rotate(bodyToWorldMatrix, bodyRotation.z, glm::vec3(0, 0, 1)) *
            glm::rotate(bodyToWorldMatrix, bodyRotation.y, glm::vec3(0, 1, 0)) *
			glm::rotate(bodyToWorldMatrix, bodyRotation.x, glm::vec3(1, 0, 0));

		headToWorldMatrix = bodyToWorldMatrix * headToBodyMatrix;

        for (int eye = 0; eye < numEyes; ++eye) {
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer[eye]);
            glViewport(0, 0, framebufferWidth, framebufferHeight);

            glClearColor(0.1f, 0.2f, 0.3f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//objectToWorldMatrix = glm::translate(objectToWorldMatrix,glm::vec3(0,.5,0)) * glm::rotate(objectToWorldMatrix, pi/3.0f, glm::vec3(0, 1, 0));
            const glm::mat3& objectToWorldNormalMatrix = glm::inverse(glm::transpose(glm::mat3(objectToWorldMatrix)));
            const glm::mat4& cameraToWorldMatrix       = headToWorldMatrix * eyeToHead[eye];

            const glm::vec3& light = glm::normalize(glm::vec3(1.0f, 0.5f, 0.2f));

			// Set drawing mode to fill, for other elements than the map.
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			//2nd shader floor and sky drawer
			drawSky(framebufferWidth, framebufferHeight, glm::value_ptr(cameraToWorldMatrix), glm::value_ptr(glm::inverse(projectionMatrix[eye])), &light.x);

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

			// Send global variables.
			glUniform4f(glGetUniformLocation(shader, "background_color"),
				BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b,
				BACKGROUND_COLOR.a);
			glUniform1i(glGetUniformLocation(shader, "lights_on"),
				lights_on);

            // Bind uniforms in the interface block
                glBindBufferBase(GL_UNIFORM_BUFFER, uniformBindingPoint, uniformBlock);

                GLubyte* ptr = (GLubyte*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
				const glm::vec3& cameraPosition = cameraToWorldMatrix[3];
				const glm::mat4& modelViewProjectionMatrix = glm::inverse(projectionMatrix[eye]) * cameraToWorldMatrix * objectToWorldMatrix;

                // mat3 is passed to openGL as if it was mat4 due to padding rules.
                for (int row = 0; row < 3; ++row) {
                    memcpy(ptr + uniformOffset[0] + sizeof(float) * 4 * row, glm::value_ptr(objectToWorldNormalMatrix) + row * 3, sizeof(float) * 3);
                }

                memcpy(ptr + uniformOffset[1], glm::value_ptr(objectToWorldMatrix), sizeof(objectToWorldMatrix));

				light_system->render(shader, objectToWorldMatrix, ptr, uniformOffset);

				// Draw the map
				glPolygonMode(GL_FRONT_AND_BACK, (wireframe ? GL_LINE : GL_FILL));
				map->render(shader, objectToWorldMatrix, cameraPosition, ptr, uniformOffset);

                
                memcpy(ptr + uniformOffset[2], glm::value_ptr(modelViewProjectionMatrix), sizeof(modelViewProjectionMatrix));
                
                memcpy(ptr + uniformOffset[3], &cameraPosition.x, sizeof(glm::vec4));

				glUnmapBuffer(GL_UNIFORM_BUFFER);

#           ifdef _VR
            {
                const vr::Texture_t tex = { reinterpret_cast<void*>(intptr_t(colorRenderTarget[eye])), vr::API_OpenGL, vr::ColorSpace_Gamma };
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
        const float cameraMoveSpeed = 0.01f;
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_W)) { bodyTranslation += glm::vec3(headToWorldMatrix * glm::vec4(0, 0, -cameraMoveSpeed, 0)); }
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_S)) { bodyTranslation += glm::vec3(headToWorldMatrix * glm::vec4(0, 0, +cameraMoveSpeed, 0)); }
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_A)) { bodyTranslation += glm::vec3(headToWorldMatrix * glm::vec4(-cameraMoveSpeed, 0, 0, 0)); }
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_D)) { bodyTranslation += glm::vec3(headToWorldMatrix * glm::vec4(+cameraMoveSpeed, 0, 0, 0)); }
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_C)) { bodyTranslation.y -= cameraMoveSpeed; }
        if ((GLFW_PRESS == glfwGetKey(window, GLFW_KEY_SPACE)) || (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_Z))) { bodyTranslation.y += cameraMoveSpeed; }

        // Keep the camera above the ground
        if (bodyTranslation.y < 0.01f) { bodyTranslation.y = 0.01f; }

        static bool inDrag = false;
        const float cameraTurnSpeed = 0.005f;
        if (GLFW_PRESS == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
            static double startX, startY;
            double currentX, currentY;

            glfwGetCursorPos(window, &currentX, &currentY);
            if (inDrag) {
                bodyRotation.y -= float(currentX - startX) * cameraTurnSpeed;
                bodyRotation.x -= float(currentY - startY) * cameraTurnSpeed;
            }
            inDrag = true; startX = currentX; startY = currentY;
        } else {
            inDrag = false;
        }

        ++timer;
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
	map->~Map();
	RawModelFactory::destructModelFactory();

    // Close the GL context and release all resources
    glfwTerminate();

    return 0;
}

#ifdef _WINDOWS
    int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw) {
        return main(0, nullptr);
    }
#endif
